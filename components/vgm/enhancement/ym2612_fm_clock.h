#pragma once

#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

// YM2612 FM produces one complete six-channel synthesis sample every 144 input
// master-clock cycles (6-cycle operator clock prescale * 24 operator slots).
inline constexpr std::uint32_t ym2612_fm_cycles_per_sample = 144;

struct ym2612_fm_clock_config {
    std::uint32_t chip_clock_hz = 0;
    std::uint32_t source_tick_rate_hz = 0;

    [[nodiscard]] constexpr bool valid() const noexcept {
        if (chip_clock_hz == 0 || source_tick_rate_hz == 0)
            return false;
        constexpr std::uint64_t max = std::numeric_limits<std::uint64_t>::max();
        const std::uint64_t denominator = static_cast<std::uint64_t>(source_tick_rate_hz) * ym2612_fm_cycles_per_sample;
        return denominator <= max / chip_clock_hz;
    }
};

// Exact rational bridge from source VGM ticks to the YM2612 native FM sample
// lattice. No floating-point time accumulator is used, so long playback cannot
// develop clock drift from repeated rounding.
class ym2612_fm_clock {
public:
    bool configure(const ym2612_fm_clock_config& config) noexcept {
        if (!config.valid()) {
            config_ = {};
            return false;
        }
        config_ = config;
        denominator_ = static_cast<std::uint64_t>(config.source_tick_rate_hz) * ym2612_fm_cycles_per_sample;
        return true;
    }

    [[nodiscard]] bool configured() const noexcept { return config_.valid(); }
    [[nodiscard]] std::uint32_t chip_clock_hz() const noexcept { return config_.chip_clock_hz; }
    [[nodiscard]] std::uint32_t source_tick_rate_hz() const noexcept { return config_.source_tick_rate_hz; }

    // Return the first native FM sample index whose time is >= tick/tick_rate.
    // A write at tick zero therefore applies before native sample zero.
    [[nodiscard]] std::uint64_t native_sample_at_or_after_tick(std::uint64_t tick) const noexcept {
        if (!configured())
            return 0;
        const auto result = mul_div(tick, config_.chip_clock_hz, denominator_);
        if (result.quotient == std::numeric_limits<std::uint64_t>::max())
            return result.quotient;
        return result.remainder == 0 ? result.quotient : result.quotient + 1;
    }

private:
    struct div_result {
        std::uint64_t quotient = 0;
        std::uint64_t remainder = 0;
    };

    static constexpr div_result mul_div(std::uint64_t value, std::uint64_t multiplier, std::uint64_t divisor) noexcept {
        if (divisor == 0)
            return {0, 0};
        const std::uint64_t whole = value / divisor;
        const std::uint64_t fraction = value % divisor;
        constexpr std::uint64_t max = std::numeric_limits<std::uint64_t>::max();
        if (whole != 0 && multiplier > max / whole)
            return {max, 0};
        const std::uint64_t whole_product = whole * multiplier;
        // configure() guarantees divisor * multiplier fits, so fraction *
        // multiplier also fits because fraction < divisor.
        const std::uint64_t fraction_product = fraction * multiplier;
        const std::uint64_t fraction_quotient = fraction_product / divisor;
        const std::uint64_t remainder = fraction_product % divisor;
        if (fraction_quotient > max - whole_product)
            return {max, 0};
        return {whole_product + fraction_quotient, remainder};
    }

    ym2612_fm_clock_config config_{};
    std::uint64_t denominator_ = 0;
};

} // namespace gameaudio::vgm
