#pragma once

#include "studio_source_resampler.h"
#include "studio_source_timeline.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace foobar_vgm::source_audio {

// Allocation-free contiguous native-source history for enhanced reconstruction.
// Source ordinals are explicit: gaps, overlaps, overflow, and unproven startup
// history all invalidate the stream rather than being repaired heuristically.
template <std::size_t Capacity>
class studio_source_stream {
    static_assert(Capacity >= studio_source_resampler_kernel::tap_count,
        "enhanced stream capacity must hold at least one complete FIR window.");

public:
    void reset() noexcept {
        first_ordinal_ = 0;
        count_ = 0;
        head_ = 0;
        have_origin_ = false;
        invalid_ = false;
    }

    [[nodiscard]] bool valid() const noexcept { return !invalid_; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }

    [[nodiscard]] std::uint64_t first_ordinal() const noexcept {
        return have_origin_ ? first_ordinal_ : 0;
    }

    [[nodiscard]] std::uint64_t next_ordinal() const noexcept {
        if (!have_origin_)
            return 0;
        if (first_ordinal_ > std::numeric_limits<std::uint64_t>::max() - count_)
            return std::numeric_limits<std::uint64_t>::max();
        return first_ordinal_ + static_cast<std::uint64_t>(count_);
    }

    bool append(
        std::uint64_t first_source_ordinal,
        const studio_stereo_sample* source,
        std::size_t frame_count) noexcept {
        return append_converted(first_source_ordinal, source, frame_count);
    }

    // libvgm captures native chip samples in DEV_SMPL while the FIR uses double
    // internally. Convert once at the source-history boundary so live integer
    // capture can enter the same tested ordinal/ring contract without staging.
    template <typename SourceSample>
    bool append_converted(
        std::uint64_t first_source_ordinal,
        const SourceSample* source,
        std::size_t frame_count) noexcept {
        if (invalid_ || (frame_count != 0 && source == nullptr)) {
            invalid_ = true;
            return false;
        }
        if (frame_count == 0)
            return true;
        if (frame_count > Capacity - count_) {
            invalid_ = true;
            return false;
        }

        if (!have_origin_) {
            first_ordinal_ = first_source_ordinal;
            have_origin_ = true;
        } else if (first_source_ordinal != next_ordinal()) {
            invalid_ = true;
            return false;
        }

        for (std::size_t i = 0; i < frame_count; ++i) {
            const double left = static_cast<double>(source[i].left);
            const double right = static_cast<double>(source[i].right);
            if (!std::isfinite(left) || !std::isfinite(right)) {
                invalid_ = true;
                return false;
            }
            storage_[(head_ + count_ + i) % Capacity] = {left, right};
        }
        count_ += frame_count;
        return true;
    }

    bool discard_before(std::uint64_t source_ordinal) noexcept {
        if (invalid_)
            return false;
        if (!have_origin_ || count_ == 0)
            return true;
        if (source_ordinal <= first_ordinal_)
            return true;

        const std::uint64_t next = next_ordinal();
        if (source_ordinal > next) {
            invalid_ = true;
            return false;
        }
        if (source_ordinal == next) {
            head_ = 0;
            count_ = 0;
            first_ordinal_ = source_ordinal;
            return true;
        }

        const std::uint64_t drop64 = source_ordinal - first_ordinal_;
        if (drop64 > count_) {
            invalid_ = true;
            return false;
        }
        const std::size_t drop = static_cast<std::size_t>(drop64);
        head_ = (head_ + drop) % Capacity;
        count_ -= drop;
        first_ordinal_ = source_ordinal;
        return true;
    }

    [[nodiscard]] bool contains(const studio_source_window& window) const noexcept {
        if (invalid_ || !have_origin_ || !window.valid || window.first < 0
            || window.final < window.first)
            return false;
        const auto first = static_cast<std::uint64_t>(window.first);
        const auto final = static_cast<std::uint64_t>(window.final);
        if (first < first_ordinal_)
            return false;
        const std::uint64_t next = next_ordinal();
        return final < next;
    }

    // A fresh OPN2 starts from a proven silent FM state. For that one boundary,
    // negative source ordinals are therefore known zeros rather than invented
    // samples. This helper deliberately does not generalize to seeks: callers
    // must opt in only when the source's reset state proves the prefix.
    [[nodiscard]] bool contains_with_known_zero_prefix(
        const studio_source_window& window) const noexcept {
        if (invalid_ || !have_origin_ || !window.valid
            || window.final < window.first || window.final < 0)
            return false;
        if (first_ordinal_ != 0)
            return false;
        const auto final = static_cast<std::uint64_t>(window.final);
        return final < next_ordinal();
    }

    studio_resample_result reconstruct(
        const studio_source_resampler_kernel& kernel,
        studio_source_phase_position position) const noexcept {
        studio_resample_result invalid;
        const auto window = plan_studio_source_window(position);
        if (!contains(window))
            return invalid;

        std::array<studio_stereo_sample, studio_source_resampler_kernel::tap_count> taps{};
        const std::uint64_t first = static_cast<std::uint64_t>(window.first);
        for (std::size_t i = 0; i < taps.size(); ++i) {
            const auto* value = find(first + static_cast<std::uint64_t>(i));
            if (value == nullptr)
                return invalid;
            taps[i] = *value;
        }

        return reconstruct_taps(kernel, window, taps);
    }

    studio_resample_result reconstruct_with_known_zero_prefix(
        const studio_source_resampler_kernel& kernel,
        studio_source_phase_position position) const noexcept {
        studio_resample_result invalid;
        const auto window = plan_studio_source_window(position);
        if (!contains_with_known_zero_prefix(window))
            return invalid;

        std::array<studio_stereo_sample, studio_source_resampler_kernel::tap_count> taps{};
        for (std::size_t i = 0; i < taps.size(); ++i) {
            const std::ptrdiff_t source_ordinal =
                window.first + static_cast<std::ptrdiff_t>(i);
            if (source_ordinal < 0)
                continue;
            const auto* value = find(static_cast<std::uint64_t>(source_ordinal));
            if (value == nullptr)
                return invalid;
            taps[i] = *value;
        }

        return reconstruct_taps(kernel, window, taps);
    }

private:
    static studio_resample_result reconstruct_taps(
        const studio_source_resampler_kernel& kernel,
        const studio_source_window& window,
        const std::array<studio_stereo_sample,
            studio_source_resampler_kernel::tap_count>& taps) noexcept {
        const double local_position =
            static_cast<double>(studio_source_resampler_kernel::pre_roll)
            + static_cast<double>(window.phase)
                / static_cast<double>(studio_source_resampler_kernel::phase_count);
        return kernel.reconstruct(taps.data(), taps.size(), local_position);
    }

    [[nodiscard]] const studio_stereo_sample* find(
        std::uint64_t source_ordinal) const noexcept {
        if (invalid_ || !have_origin_ || source_ordinal < first_ordinal_)
            return nullptr;
        const std::uint64_t offset64 = source_ordinal - first_ordinal_;
        if (offset64 >= count_)
            return nullptr;
        const std::size_t offset = static_cast<std::size_t>(offset64);
        return &storage_[(head_ + offset) % Capacity];
    }

    std::array<studio_stereo_sample, Capacity> storage_{};
    std::uint64_t first_ordinal_ = 0;
    std::size_t count_ = 0;
    std::size_t head_ = 0;
    bool have_origin_ = false;
    bool invalid_ = false;
};

} // namespace foobar_vgm::source_audio
