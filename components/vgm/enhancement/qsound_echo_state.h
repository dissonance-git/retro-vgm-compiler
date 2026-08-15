#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

enum class qsound_echo_mode : std::uint8_t {
    mode1 = 0,
    mode2 = 1,
};

enum class qsound_echo_step_status : std::uint8_t {
    exact = 0,
    historical_int32_overflow_domain = 1,
    end_position_conversion_domain = 2,
};

struct qsound_echo_step_result {
    qsound_echo_step_status status = qsound_echo_step_status::exact;
    std::int16_t output = 0;
};

// Project-owned model of the recovered superctr shared echo transition only.
// It intentionally stops before QSound pan tables, FIR filters and output delay
// lines. A live parity claim additionally requires an earned initial state;
// reset() is correct only at an actual QSound DSP initialization boundary.
class qsound_echo_state {
public:
    static constexpr std::size_t delay_capacity = 1024;

    explicit qsound_echo_state(qsound_echo_mode mode = qsound_echo_mode::mode1) noexcept {
        reset(mode);
    }

    void reset(qsound_echo_mode mode) noexcept {
        mode_ = mode;
        feedback_ = 0;
        end_position_ = static_cast<std::uint16_t>(base_end_position(mode_) + 6u);
        last_sample_ = 0;
        delay_line_.fill(0);
        delay_position_ = 0;
    }

    void set_feedback(std::uint16_t raw) noexcept {
        feedback_ = signed16(raw);
    }

    void set_end_position(std::uint16_t raw) noexcept {
        end_position_ = raw;
    }

    qsound_echo_mode mode() const noexcept { return mode_; }
    std::int16_t feedback() const noexcept { return feedback_; }
    std::uint16_t end_position() const noexcept { return end_position_; }
    std::int16_t last_sample() const noexcept { return last_sample_; }
    std::size_t delay_position() const noexcept { return delay_position_; }

    qsound_echo_step_result step(std::int32_t input) noexcept {
        std::size_t length = 0;
        const qsound_echo_step_status length_status = current_length(length);
        if (length_status != qsound_echo_step_status::exact)
            return {length_status, 0};

        const std::int32_t line_sample = delay_line_[delay_position_];
        const std::int32_t previous = last_sample_;
        const std::int32_t averaged = arithmetic_shift_right(
            static_cast<std::int64_t>(line_sample) + static_cast<std::int64_t>(previous), 1);

        const std::int64_t feedback_product = static_cast<std::int64_t>(averaged)
            * static_cast<std::int64_t>(feedback_);
        const std::int64_t feedback_scaled = feedback_product * 4;
        constexpr std::int64_t int32_min = std::numeric_limits<std::int32_t>::min();
        constexpr std::int64_t int32_max = std::numeric_limits<std::int32_t>::max();
        if (feedback_scaled < int32_min || feedback_scaled > int32_max)
            return {qsound_echo_step_status::historical_int32_overflow_domain, 0};

        const std::int64_t new_sample = static_cast<std::int64_t>(input) + feedback_scaled;
        if (new_sample < int32_min || new_sample > int32_max)
            return {qsound_echo_step_status::historical_int32_overflow_domain, 0};

        const std::int32_t stored = arithmetic_shift_right(new_sample, 16);
        std::size_t next_position = delay_position_ + 1u;
        if (next_position >= length)
            next_position = 0;

        // Commit only after the complete transition is inside the declared
        // arithmetic/control domain. Failed certification cannot partially
        // advance an otherwise usable project-side echo state.
        last_sample_ = static_cast<std::int16_t>(line_sample);
        delay_line_[delay_position_] = static_cast<std::int16_t>(stored);
        delay_position_ = next_position;

        return {
            qsound_echo_step_status::exact,
            static_cast<std::int16_t>(averaged),
        };
    }

private:
    static constexpr std::uint16_t base_end_position(qsound_echo_mode mode) noexcept {
        return mode == qsound_echo_mode::mode2 ? 0x053cu : 0x0554u;
    }

    static constexpr std::int16_t signed16(std::uint16_t raw) noexcept {
        return raw <= 0x7fffu
            ? static_cast<std::int16_t>(raw)
            : static_cast<std::int16_t>(static_cast<std::int32_t>(raw) - 0x10000);
    }

    static std::int32_t arithmetic_shift_right(std::int64_t value, unsigned bits) noexcept {
        const std::int64_t divisor = static_cast<std::int64_t>(1) << bits;
        std::int64_t quotient = value / divisor;
        if (value < 0 && (value % divisor) != 0)
            --quotient;
        return static_cast<std::int32_t>(quotient);
    }

    qsound_echo_step_status current_length(std::size_t& out) const noexcept {
        const std::int32_t difference = static_cast<std::int32_t>(end_position_)
            - static_cast<std::int32_t>(base_end_position(mode_));

        // The pinned C core first assigns this difference into INT16 before
        // clamping it to [0, 1024]. Values outside signed-16 range depend on the
        // implementation-defined narrowing conversion, so do not certify them.
        if (difference < std::numeric_limits<std::int16_t>::min() ||
            difference > std::numeric_limits<std::int16_t>::max())
            return qsound_echo_step_status::end_position_conversion_domain;

        if (difference <= 0)
            out = 0;
        else if (difference >= static_cast<std::int32_t>(delay_capacity))
            out = delay_capacity;
        else
            out = static_cast<std::size_t>(difference);
        return qsound_echo_step_status::exact;
    }

    qsound_echo_mode mode_ = qsound_echo_mode::mode1;
    std::int16_t feedback_ = 0;
    std::uint16_t end_position_ = 0;
    std::int16_t last_sample_ = 0;
    std::array<std::int16_t, delay_capacity> delay_line_{};
    std::size_t delay_position_ = 0;
};

} // namespace gameaudio::vgm
