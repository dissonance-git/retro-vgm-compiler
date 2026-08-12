#pragma once

#include "vgm_command_event.h"

#include <array>
#include <cstdint>

namespace gameaudio::vgm {

struct ym2612_operator_state {
    std::uint8_t detune = 0;
    std::uint8_t multiple = 0;
    std::uint8_t total_level = 0;
    std::uint8_t key_scale = 0;
    std::uint8_t attack_rate = 0;
    bool amplitude_modulation = false;
    std::uint8_t decay_rate = 0;
    std::uint8_t sustain_rate = 0;
    std::uint8_t sustain_level = 0;
    std::uint8_t release_rate = 0;
    std::uint8_t ssg_eg = 0;
};

struct ym2612_channel_state {
    bool key_on = false;
    std::uint8_t operator_key_mask = 0;

    std::uint16_t fnum = 0;
    std::uint8_t block = 0;

    bool pan_left = true;
    bool pan_right = true;
    std::uint8_t algorithm = 0;
    std::uint8_t feedback = 0;
    std::uint8_t ams = 0;
    std::uint8_t fms = 0;

    std::array<ym2612_operator_state, 4> operators{};
};

struct ym2612_state {
    std::array<ym2612_channel_state, 6> channels{};
    std::array<std::array<std::uint8_t, 256>, 2> registers{};

    // YM2612 frequency writes use shared high-byte latches. A4/AC update the
    // latch; A0/A8 commit it to a channel/operator frequency.
    std::uint8_t fnum_high_latch = 0;
    std::uint8_t ch3_fnum_high_latch = 0;
    std::array<std::uint16_t, 3> ch3_fnum{};
    std::array<std::uint8_t, 3> ch3_block{};

    bool lfo_enabled = false;
    std::uint8_t lfo_frequency = 0;
    std::uint8_t channel3_mode = 0;
    bool csm_enabled = false;

    bool dac_enabled = false;
    std::uint8_t last_dac_sample = 0x80;
    std::uint64_t direct_dac_write_count = 0;
    std::uint64_t stream_dac_step_count = 0;
    std::uint64_t resolved_stream_dac_write_count = 0;
};

struct sn76489_channel_state {
    std::uint16_t tone_period = 0;
    std::uint8_t attenuation = 0x0F;
};

struct sn76489_state {
    std::array<sn76489_channel_state, 4> channels{};
    std::uint8_t noise_control = 0;
    std::uint8_t stereo_mask = 0xFF;

    std::uint8_t latched_channel = 0;
    bool latched_volume = false;
};

// Realtime state derivable directly from Genesis/Mega Drive VGM commands.
// This intentionally stores only source truth. It does not infer instrument
// semantics, source importance, width, height, or enhancement permission.
class genesis_state {
public:
    void reset() noexcept;
    void observe(const command_event& event) noexcept;

    const ym2612_state& ym2612(std::size_t instance = 0) const noexcept { return ym2612_[instance & 1u]; }
    const sn76489_state& psg(std::size_t instance = 0) const noexcept { return psg_[instance & 1u]; }

    std::uint64_t observed_commands() const noexcept { return observed_commands_; }
    std::uint64_t ym2612_writes() const noexcept { return ym2612_writes_; }
    std::uint64_t psg_writes() const noexcept { return psg_writes_; }

private:
    static void write_ym2612(ym2612_state& state, std::uint8_t port, std::uint8_t reg, std::uint8_t data) noexcept;
    static void write_psg(sn76489_state& state, std::uint8_t data) noexcept;

    std::array<ym2612_state, 2> ym2612_{};
    std::array<sn76489_state, 2> psg_{};
    std::uint64_t observed_commands_ = 0;
    std::uint64_t ym2612_writes_ = 0;
    std::uint64_t psg_writes_ = 0;
};

} // namespace gameaudio::vgm
