#pragma once

#include "ym2151_enhanced_recomposition.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

enum class ym2151_native_source_witness_error : std::uint8_t {
    none = 0,
    mix_mismatch,
};

// One native-rate observation from the guarded libvgm MAME YM2151 tap. Values
// stay integer here so exact source accounting can be proven before any source
// resampling, float conversion or host-rate scheduling is introduced.
struct ym2151_native_source_witness {
    std::array<std::int32_t, ym2151_recomposition_source_count> left{};
    std::array<std::int32_t, ym2151_recomposition_source_count> right{};
    std::int32_t mix_left = 0;
    std::int32_t mix_right = 0;
};

class ym2151_native_source_witness_validator {
public:
    void reset() noexcept {
        valid_ = true;
        sample_count_ = 0;
        last_error_ = ym2151_native_source_witness_error::none;
    }

    bool observe(const ym2151_native_source_witness& witness) noexcept {
        if (!valid_)
            return false;

        std::int64_t sum_left = 0;
        std::int64_t sum_right = 0;
        for (std::size_t channel = 0; channel < ym2151_recomposition_source_count; ++channel) {
            sum_left += static_cast<std::int64_t>(witness.left[channel]);
            sum_right += static_cast<std::int64_t>(witness.right[channel]);
        }

        if (sum_left != static_cast<std::int64_t>(witness.mix_left)
            || sum_right != static_cast<std::int64_t>(witness.mix_right))
        {
            valid_ = false;
            last_error_ = ym2151_native_source_witness_error::mix_mismatch;
            return false;
        }

        if (sample_count_ != std::numeric_limits<std::uint64_t>::max())
            ++sample_count_;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    std::uint64_t sample_count() const noexcept { return sample_count_; }
    ym2151_native_source_witness_error last_error() const noexcept {
        return last_error_;
    }

private:
    bool valid_ = true;
    std::uint64_t sample_count_ = 0;
    ym2151_native_source_witness_error last_error_ =
        ym2151_native_source_witness_error::none;
};

} // namespace gameaudio::vgm
