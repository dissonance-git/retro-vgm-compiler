#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Canonical Genesis source order used by the source-aware foobar/libvgm seam.
// These are implementation sources, not persistent musical parts.
enum class genesis_recomposition_source : std::uint8_t {
    ym2612_fm1 = 0,
    ym2612_fm2,
    ym2612_fm3,
    ym2612_fm4,
    ym2612_fm5,
    ym2612_fm6,
    ym2612_dac,
    sn76489_tone0,
    sn76489_tone1,
    sn76489_tone2,
    sn76489_noise,
    count,
};

constexpr std::size_t genesis_recomposition_source_count =
    static_cast<std::size_t>(genesis_recomposition_source::count);

enum class genesis_enhanced_recomposition_error : std::uint8_t {
    none = 0,
    invalid_reference,
    frame_count_too_large,
    missing_exact_reference_source,
    missing_enhanced_source,
    nonfinite_sample,
};

struct genesis_stereo_source_view {
    const float* left = nullptr;
    const float* right = nullptr;

    // Exact means this lane is the contribution already contained in the
    // protected reference stereo mix at the same output frame and gain domain.
    // A merely similar/emulated source is not sufficient for subtraction.
    bool exact = false;

    bool present() const noexcept { return left != nullptr && right != nullptr; }
};

struct genesis_source_replacement_view {
    genesis_stereo_source_view reference{};
    genesis_stereo_source_view enhanced{};
    bool replace = false;
};

// Source-domain replacement without requiring the entire mix to be
// re-synthesized. For every explicitly supported source:
//
//   enhanced_mix = protected_reference + (enhanced_source - exact_reference_source)
//
// Unsupported sources, effects, other chips, and unknown residuals remain
// bit-for-bit in the protected reference path. This is the audible bridge from
// sidecar synthesis to enhanced playback without double-rendering a source.
//
// All inputs must already share the same host-rate, stereo-route, and final
// song/fade gain domain. Resampling or gain alignment belongs at the producer
// boundary, where its provenance can be verified.
template <std::size_t MaxFrames = 4096>
class genesis_enhanced_recomposition_storage {
    static_assert(MaxFrames > 0, "MaxFrames must be non-zero");

public:
    void reset() noexcept {
        frame_count_ = 0;
        valid_ = false;
        used_replacement_ = false;
        last_error_ = genesis_enhanced_recomposition_error::none;
    }

    bool build(
        const float* reference_left,
        const float* reference_right,
        std::size_t frame_count,
        const std::array<genesis_source_replacement_view,
                         genesis_recomposition_source_count>& sources) noexcept
    {
        reset();
        if (reference_left == nullptr || reference_right == nullptr || frame_count == 0)
            return fail(genesis_enhanced_recomposition_error::invalid_reference);
        if (frame_count > MaxFrames)
            return fail(genesis_enhanced_recomposition_error::frame_count_too_large);

        frame_count_ = frame_count;
        // Always seed the destination with the protected reference. If any
        // replacement evidence fails later, restore this reference before
        // returning false so an audio caller can fall back without silence.
        if (!copy_reference(reference_left, reference_right, frame_count))
            return fail_to_reference(
                reference_left,
                reference_right,
                genesis_enhanced_recomposition_error::nonfinite_sample);

        for (const auto& source : sources) {
            if (!source.replace)
                continue;
            used_replacement_ = true;

            if (!source.reference.present() || !source.reference.exact)
                return fail_to_reference(
                    reference_left,
                    reference_right,
                    genesis_enhanced_recomposition_error::missing_exact_reference_source);
            if (!source.enhanced.present() || !source.enhanced.exact)
                return fail_to_reference(
                    reference_left,
                    reference_right,
                    genesis_enhanced_recomposition_error::missing_enhanced_source);

            for (std::size_t frame = 0; frame < frame_count; ++frame) {
                const float ref_l = source.reference.left[frame];
                const float ref_r = source.reference.right[frame];
                const float enh_l = source.enhanced.left[frame];
                const float enh_r = source.enhanced.right[frame];
                if (!std::isfinite(ref_l) || !std::isfinite(ref_r) ||
                    !std::isfinite(enh_l) || !std::isfinite(enh_r))
                    return fail_to_reference(
                        reference_left,
                        reference_right,
                        genesis_enhanced_recomposition_error::nonfinite_sample);

                const double out_l = static_cast<double>(left_[frame])
                    + static_cast<double>(enh_l) - static_cast<double>(ref_l);
                const double out_r = static_cast<double>(right_[frame])
                    + static_cast<double>(enh_r) - static_cast<double>(ref_r);
                if (!std::isfinite(out_l) || !std::isfinite(out_r))
                    return fail_to_reference(
                        reference_left,
                        reference_right,
                        genesis_enhanced_recomposition_error::nonfinite_sample);
                left_[frame] = static_cast<float>(out_l);
                right_[frame] = static_cast<float>(out_r);
            }
        }

        valid_ = true;
        last_error_ = genesis_enhanced_recomposition_error::none;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    bool used_replacement() const noexcept { return valid_ && used_replacement_; }
    std::size_t frame_count() const noexcept { return frame_count_; }
    const float* left() const noexcept { return left_.data(); }
    const float* right() const noexcept { return right_.data(); }
    genesis_enhanced_recomposition_error last_error() const noexcept {
        return last_error_;
    }

private:
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
        genesis_enhanced_recomposition_error error) noexcept
    {
        // The reference was validated and copied before replacements began.
        // Re-copying undoes any earlier source deltas from this candidate block.
        for (std::size_t frame = 0; frame < frame_count_; ++frame) {
            left_[frame] = reference_left[frame];
            right_[frame] = reference_right[frame];
        }
        valid_ = false;
        used_replacement_ = false;
        last_error_ = error;
        return false;
    }

    bool fail(genesis_enhanced_recomposition_error error) noexcept {
        valid_ = false;
        used_replacement_ = false;
        last_error_ = error;
        return false;
    }

    std::array<float, MaxFrames> left_{};
    std::array<float, MaxFrames> right_{};
    std::size_t frame_count_ = 0;
    bool valid_ = false;
    bool used_replacement_ = false;
    genesis_enhanced_recomposition_error last_error_ =
        genesis_enhanced_recomposition_error::none;
};

} // namespace gameaudio::vgm
