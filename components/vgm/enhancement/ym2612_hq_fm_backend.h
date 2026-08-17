#pragma once

#include "ym2612_fm_backend.h"
#include "ym2612_hq_fm_profile.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Experimental-but-audible high-ceiling FM backend. It intentionally preserves
// the six physical YM2612 FM channels and the source four-operator graph while
// replacing the low-resolution sine/amplitude/channel arithmetic with floating
// point synthesis at a bounded oversampled rate.
//
// `semantic_coverage_complete()` is the admission fence. Until a source uses
// only semantics implemented here, the foobar shell must retain the exact Nuked
// reference lane for that source rather than guessing through an unsupported
// register feature.
class ym2612_hq_fm_backend final : public ym2612_fm_backend {
public:
    explicit ym2612_hq_fm_backend(ym2612_hq_fm_profile profile = {}) noexcept;

    bool configure(const ym2612_fm_backend_config& config) noexcept override;
    void reset() noexcept override;
    [[nodiscard]] std::size_t latency_frames() const noexcept override { return 0; }

    void render_timed(
        const ym2612_timed_write* writes,
        std::size_t write_count,
        float* const outputs[channel_count],
        std::size_t frames) noexcept override;

    [[nodiscard]] bool semantic_coverage_complete() const noexcept {
        return semantic_coverage_complete_;
    }
    [[nodiscard]] bool configured() const noexcept { return configured_; }
    [[nodiscard]] const ym2612_hq_fm_profile& profile() const noexcept { return profile_; }

private:
    enum class envelope_stage : std::uint8_t {
        off = 0,
        attack,
        decay,
        sustain,
        release,
    };

    struct operator_state {
        std::uint8_t detune = 0;
        std::uint8_t multiple = 0;
        std::uint8_t total_level = 0x7f;
        std::uint8_t key_scale = 0;
        std::uint8_t attack_rate = 0;
        bool amplitude_modulation = false;
        std::uint8_t decay_rate = 0;
        std::uint8_t sustain_rate = 0;
        std::uint8_t sustain_level = 0x0f;
        std::uint8_t release_rate = 0;
        std::uint8_t ssg_eg = 0;

        double phase_cycles = 0.0;
        double envelope = 0.0;
        envelope_stage stage = envelope_stage::off;
        bool keyed = false;
    };

    struct channel_state {
        std::uint16_t fnum = 0;
        std::uint8_t block = 0;
        std::uint8_t algorithm = 0;
        std::uint8_t feedback = 0;
        bool pan_left = true;
        bool pan_right = true;
        std::uint8_t ams = 0;
        std::uint8_t fms = 0;
        std::array<operator_state, 4> operators{};

        double op1_history0 = 0.0;
        double op1_history1 = 0.0;
        double op2_memory = 0.0;
        double last_operator_output = 0.0;
    };

    void apply_write(const ym2612_timed_write& write) noexcept;
    void apply_key_write(std::uint8_t data) noexcept;
    double render_channel(std::size_t channel_index, double sample_rate_hz) noexcept;
    double render_operator(
        channel_state& channel,
        std::size_t operator_index,
        double modulation_phase_units,
        double sample_rate_hz) noexcept;
    double operator_frequency_hz(
        const channel_state& channel,
        const operator_state& op) const noexcept;
    void advance_envelope(operator_state& op, double sample_rate_hz) noexcept;

    static double rate_time_seconds(std::uint8_t rate, double slow_seconds, double fast_seconds) noexcept;
    static double total_level_gain(std::uint8_t total_level) noexcept;
    static double sustain_gain(std::uint8_t sustain_level) noexcept;
    static std::size_t channel_for_register(std::uint8_t port, std::uint8_t reg) noexcept;
    static std::size_t operator_for_register(std::uint8_t reg) noexcept;

    ym2612_hq_fm_profile profile_{};
    ym2612_fm_backend_config config_{};
    std::array<channel_state, channel_count> channels_{};
    std::array<std::uint8_t, 2> fnum_high_latch_{{0, 0}};
    std::uint8_t channel3_mode_ = 0;
    bool dac_enabled_ = false;
    bool lfo_enabled_ = false;
    std::uint8_t lfo_frequency_ = 0;
    std::uint64_t internal_sample_cursor_ = 0;
    bool configured_ = false;
    bool semantic_coverage_complete_ = true;
};

} // namespace gameaudio::vgm
