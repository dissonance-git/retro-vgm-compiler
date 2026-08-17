#pragma once

#include <cmath>
#include <cstdint>

namespace gameaudio::spc {

// SNESAPU interpolation identifiers from the pinned SPCPlay DSP.inc contract.
enum class snesapu_source_interpolation : std::uint8_t {
    none = 0,
    linear = 1,
    cubic = 2,
    gaussian = 3,
    sinc8 = 4,
    gaussian4_a = 5,
    gaussian4_b = 6,
    gaussian4_c = 7,
};

constexpr bool snesapu_source_interpolation_from_raw(
    std::uint32_t raw,
    snesapu_source_interpolation& interpolation) noexcept
{
    if (raw > static_cast<std::uint32_t>(snesapu_source_interpolation::gaussian4_c))
        return false;
    interpolation = static_cast<snesapu_source_interpolation>(raw);
    return true;
}

// Effective source-time offset of the historical pInter result relative to the
// pitch accumulator at MixSample. These values come from StartSrc's sIdx setup
// together with the sample positions consumed by NoneInt/LinearInt/Point4Int/
// Point8Int in the pinned SPCPlay source.
constexpr int snesapu_interpolation_phase_offset_samples(
    snesapu_source_interpolation interpolation) noexcept
{
    switch (interpolation) {
    case snesapu_source_interpolation::none:
        return 0;
    case snesapu_source_interpolation::linear:
    case snesapu_source_interpolation::sinc8:
        return -1;
    case snesapu_source_interpolation::cubic:
    case snesapu_source_interpolation::gaussian:
    case snesapu_source_interpolation::gaussian4_a:
    case snesapu_source_interpolation::gaussian4_b:
    case snesapu_source_interpolation::gaussian4_c:
        return 1;
    }
    return 0;
}

// UpdateSrc reads only the low whole-sample byte of mRate plus its 16-bit
// fraction. Current S-DSP pitch limits keep valid rates inside this range.
constexpr bool snesapu_source_rate_representable(std::uint32_t q16_16_rate) noexcept {
    return (q16_16_rate & 0xff000000u) == 0u;
}

struct snesapu_game_loop_span {
    bool present = false;
    double start_sample = 0.0;
    double end_sample = 0.0; // exclusive first-pass endpoint

    bool valid() const noexcept {
        if (!present)
            return true;
        return std::isfinite(start_sample)
            && std::isfinite(end_sample)
            && start_sample >= 0.0
            && end_sample > start_sample;
    }
};

struct snesapu_source_trajectory_projection {
    double accumulator_sample_position = 0.0;
    double effective_sample_position = 0.0;
    double canonical_game_sample_position = 0.0;
    std::uint64_t loop_cycle = 0;
    bool before_key_on = false;
    bool valid = false;
};

// Parallel logical source phase for the SNESAPU hot loop. It deliberately does
// not derive time from sIdx/bCur: those are decode-window implementation state
// and refill early to expose interpolation history/lookahead. The pitch
// recurrence itself is simpler and invariant across buffer refills.
class snesapu_source_trajectory_tracker {
public:
    void key_on(snesapu_source_interpolation interpolation) noexcept {
        phase_q16_16_ = 0;
        interpolation_ = interpolation;
        active_ = true;
    }

    void stop() noexcept { active_ = false; }

    void set_interpolation(snesapu_source_interpolation interpolation) noexcept {
        interpolation_ = interpolation;
    }

    bool advance(std::uint32_t m_rate_q16_16) noexcept {
        if (!active_ || !snesapu_source_rate_representable(m_rate_q16_16))
            return false;
        phase_q16_16_ += static_cast<std::uint64_t>(m_rate_q16_16);
        return true;
    }

    snesapu_source_trajectory_projection project(
        const snesapu_game_loop_span& loop = {}) const noexcept
    {
        snesapu_source_trajectory_projection out;
        if (!active_ || !loop.valid())
            return out;

        constexpr double q16 = 65536.0;
        out.accumulator_sample_position = static_cast<double>(phase_q16_16_) / q16;
        out.effective_sample_position = out.accumulator_sample_position
            + static_cast<double>(snesapu_interpolation_phase_offset_samples(interpolation_));
        out.canonical_game_sample_position = out.effective_sample_position;
        out.before_key_on = out.effective_sample_position < 0.0;

        if (loop.present && out.effective_sample_position >= loop.end_sample) {
            const double span = loop.end_sample - loop.start_sample;
            const double after_first_end = out.effective_sample_position - loop.end_sample;
            const double cycle_index = std::floor(after_first_end / span);
            const double cycle_offset = after_first_end - cycle_index * span;
            out.canonical_game_sample_position = loop.start_sample + cycle_offset;
            out.loop_cycle = 1u + static_cast<std::uint64_t>(cycle_index);
        }

        out.valid = std::isfinite(out.accumulator_sample_position)
            && std::isfinite(out.effective_sample_position)
            && std::isfinite(out.canonical_game_sample_position);
        return out;
    }

    std::uint64_t raw_phase_q16_16() const noexcept { return phase_q16_16_; }
    bool active() const noexcept { return active_; }
    snesapu_source_interpolation interpolation() const noexcept { return interpolation_; }

private:
    std::uint64_t phase_q16_16_ = 0;
    snesapu_source_interpolation interpolation_ = snesapu_source_interpolation::gaussian;
    bool active_ = false;
};

} // namespace gameaudio::spc
