#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::vgm {

// Generic exact-source replacement contract shared by VGM device families.
// Sources remain implementation identities. Family grouping is supplied by the
// caller so chip-specific topology never leaks into this shared mechanism.
enum class source_family_recomposition_error : std::uint8_t {
    none = 0,
    invalid_reference,
    frame_count_too_large,
    missing_exact_reference_source,
    missing_enhanced_source,
    nonfinite_sample,
    invalid_family_map,
};

struct source_family_stereo_view {
    const float* left = nullptr;
    const float* right = nullptr;
    bool exact = false;

    bool present() const noexcept { return left != nullptr && right != nullptr; }
};

struct source_family_replacement_view {
    source_family_stereo_view reference{};
    source_family_stereo_view enhanced{};
    bool replace = false;
};

struct source_family_recomposition_status {
    bool requested = false;
    bool applied = false;
    source_family_recomposition_error error = source_family_recomposition_error::none;
};

// Recompose a protected stereo reference by replacing only exact source
// contributions whose enhanced realization has independently earned admission:
//
//   output = protected_reference + sum(enhanced_source - exact_reference_source)
//
// Strict build() is an all-or-nothing scientific/control transaction.
// build_independent_families() gives each evidence family its own transaction:
// one failed family remains reference while successful families stay applied.
//
// The mechanism intentionally knows nothing about chips, FM topology, panning,
// persistent musical parts, or Spatial presentation. Producers must align every
// source view to the same host-rate, stereo-route and final gain domain first.
template <std::size_t SourceCount,
          std::size_t FamilyCount,
          std::size_t MaxFrames = 4096>
class source_family_recomposition_storage {
    static_assert(SourceCount > 0, "SourceCount must be non-zero");
    static_assert(FamilyCount > 0, "FamilyCount must be non-zero");
    static_assert(MaxFrames > 0, "MaxFrames must be non-zero");

public:
    using source_array = std::array<source_family_replacement_view, SourceCount>;
    using family_map = std::array<std::size_t, SourceCount>;

    void reset() noexcept {
        frame_count_ = 0;
        valid_ = false;
        used_replacement_ = false;
        last_error_ = source_family_recomposition_error::none;
        family_status_ = {};
    }

    bool build(
        const float* reference_left,
        const float* reference_right,
        std::size_t frame_count,
        const source_array& sources) noexcept
    {
        reset();
        if (!begin_reference(reference_left, reference_right, frame_count))
            return false;

        bool any_requested = false;
        for (const auto& source : sources) {
            if (!source.replace)
                continue;
            any_requested = true;
            const auto error = apply_one_source(
                source, left_.data(), right_.data(), frame_count_);
            if (error != source_family_recomposition_error::none)
                return fail_to_reference(reference_left, reference_right, error);
        }

        used_replacement_ = any_requested;
        valid_ = true;
        last_error_ = source_family_recomposition_error::none;
        return true;
    }

    bool build_independent_families(
        const float* reference_left,
        const float* reference_right,
        std::size_t frame_count,
        const source_array& sources,
        const family_map& source_families) noexcept
    {
        reset();
        for (const auto family : source_families) {
            if (family >= FamilyCount)
                return fail(source_family_recomposition_error::invalid_family_map);
        }
        if (!begin_reference(reference_left, reference_right, frame_count))
            return false;

        for (std::size_t family = 0; family < FamilyCount; ++family) {
            for (std::size_t frame = 0; frame < frame_count_; ++frame) {
                scratch_left_[frame] = left_[frame];
                scratch_right_[frame] = right_[frame];
            }

            auto& status = family_status_[family];
            for (std::size_t source_index = 0; source_index < SourceCount; ++source_index) {
                if (source_families[source_index] != family)
                    continue;
                const auto& source = sources[source_index];
                if (!source.replace)
                    continue;
                status.requested = true;
                const auto error = apply_one_source(
                    source, scratch_left_.data(), scratch_right_.data(), frame_count_);
                if (error != source_family_recomposition_error::none) {
                    status.error = error;
                    break;
                }
            }

            if (!status.requested || status.error != source_family_recomposition_error::none)
                continue;

            for (std::size_t frame = 0; frame < frame_count_; ++frame) {
                left_[frame] = scratch_left_[frame];
                right_[frame] = scratch_right_[frame];
            }
            status.applied = true;
            used_replacement_ = true;
        }

        valid_ = true;
        last_error_ = source_family_recomposition_error::none;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    bool used_replacement() const noexcept { return valid_ && used_replacement_; }
    std::size_t frame_count() const noexcept { return frame_count_; }
    const float* left() const noexcept { return left_.data(); }
    const float* right() const noexcept { return right_.data(); }
    source_family_recomposition_error last_error() const noexcept { return last_error_; }

    source_family_recomposition_status family_status(std::size_t family) const noexcept {
        return family < FamilyCount
            ? family_status_[family]
            : source_family_recomposition_status{};
    }

    bool had_family_failure() const noexcept {
        if (!valid_)
            return false;
        for (const auto& status : family_status_) {
            if (status.requested && !status.applied &&
                status.error != source_family_recomposition_error::none)
                return true;
        }
        return false;
    }

private:
    bool begin_reference(
        const float* reference_left,
        const float* reference_right,
        std::size_t frame_count) noexcept
    {
        if (reference_left == nullptr || reference_right == nullptr || frame_count == 0)
            return fail(source_family_recomposition_error::invalid_reference);
        if (frame_count > MaxFrames)
            return fail(source_family_recomposition_error::frame_count_too_large);

        frame_count_ = frame_count;
        if (!copy_reference(reference_left, reference_right, frame_count))
            return fail_to_reference(
                reference_left,
                reference_right,
                source_family_recomposition_error::nonfinite_sample);
        return true;
    }

    static bool representable_float(double value) noexcept {
        if (!std::isfinite(value))
            return false;
        const double limit = static_cast<double>(std::numeric_limits<float>::max());
        return value >= -limit && value <= limit;
    }

    static source_family_recomposition_error apply_one_source(
        const source_family_replacement_view& source,
        float* destination_left,
        float* destination_right,
        std::size_t frames) noexcept
    {
        if (!source.reference.present() || !source.reference.exact)
            return source_family_recomposition_error::missing_exact_reference_source;
        if (!source.enhanced.present() || !source.enhanced.exact)
            return source_family_recomposition_error::missing_enhanced_source;

        for (std::size_t frame = 0; frame < frames; ++frame) {
            const float ref_l = source.reference.left[frame];
            const float ref_r = source.reference.right[frame];
            const float enh_l = source.enhanced.left[frame];
            const float enh_r = source.enhanced.right[frame];
            if (!std::isfinite(ref_l) || !std::isfinite(ref_r) ||
                !std::isfinite(enh_l) || !std::isfinite(enh_r))
                return source_family_recomposition_error::nonfinite_sample;

            const double out_l = static_cast<double>(destination_left[frame])
                + static_cast<double>(enh_l) - static_cast<double>(ref_l);
            const double out_r = static_cast<double>(destination_right[frame])
                + static_cast<double>(enh_r) - static_cast<double>(ref_r);
            if (!representable_float(out_l) || !representable_float(out_r))
                return source_family_recomposition_error::nonfinite_sample;
            destination_left[frame] = static_cast<float>(out_l);
            destination_right[frame] = static_cast<float>(out_r);
        }
        return source_family_recomposition_error::none;
    }

    bool copy_reference(
        const float* left,
        const float* right,
        std::size_t frames) noexcept
    {
        for (std::size_t frame = 0; frame < frames; ++frame) {
            if (!std::isfinite(left[frame]) || !std::isfinite(right[frame]))
                return false;
            left_[frame] = left[frame];
            right_[frame] = right[frame];
        }
        return true;
    }

    bool fail_to_reference(
        const float* reference_left,
        const float* reference_right,
        source_family_recomposition_error error) noexcept
    {
        for (std::size_t frame = 0; frame < frame_count_; ++frame) {
            left_[frame] = reference_left[frame];
            right_[frame] = reference_right[frame];
        }
        valid_ = false;
        used_replacement_ = false;
        family_status_ = {};
        last_error_ = error;
        return false;
    }

    bool fail(source_family_recomposition_error error) noexcept {
        valid_ = false;
        used_replacement_ = false;
        family_status_ = {};
        last_error_ = error;
        return false;
    }

    std::array<float, MaxFrames> left_{};
    std::array<float, MaxFrames> right_{};
    std::array<float, MaxFrames> scratch_left_{};
    std::array<float, MaxFrames> scratch_right_{};
    std::array<source_family_recomposition_status, FamilyCount> family_status_{};
    std::size_t frame_count_ = 0;
    bool valid_ = false;
    bool used_replacement_ = false;
    source_family_recomposition_error last_error_ = source_family_recomposition_error::none;
};

} // namespace gameaudio::vgm
