#pragma once

#include "studio_source_resampler.h"
#include "studio_source_timeline.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace foobar_vgm::source_audio {

// Allocation-free contiguous native-source history for Studio reconstruction.
// Source ordinals are explicit: gaps, overlaps, overflow, and invented startup
// history all invalidate the stream rather than being repaired heuristically.
template <std::size_t Capacity>
class studio_source_stream {
    static_assert(Capacity >= studio_source_resampler_kernel::tap_count,
        "Studio stream capacity must hold at least one complete FIR window.");

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
            if (!std::isfinite(source[i].left) || !std::isfinite(source[i].right)) {
                invalid_ = true;
                return false;
            }
            storage_[(head_ + count_ + i) % Capacity] = source[i];
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

        const double local_position =
            static_cast<double>(studio_source_resampler_kernel::pre_roll)
            + static_cast<double>(window.phase)
                / static_cast<double>(studio_source_resampler_kernel::phase_count);
        return kernel.reconstruct(taps.data(), taps.size(), local_position);
    }

private:
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
