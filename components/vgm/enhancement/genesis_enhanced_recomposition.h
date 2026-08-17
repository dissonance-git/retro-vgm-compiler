#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

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

enum class genesis_recomposition_family : std::uint8_t {
    ym2612_fm = 0,
    ym2612_dac,
    sn76489_psg,
    count,
};

constexpr std::size_t genesis_recomposition_family_count =
    static_cast<std::size_t>(genesis_recomposition_family::count);

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

struct genesis_recomposition_family_status {
    bool requested = false;
    bool applied = false;
    genesis_enhanced_recomposition_error error =
        genesis_enhanced_recomposition_error::none;
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
    using source_array = std::array<
        genesis_source_replacement_view,
        genesis_recomposition_source_count>;

    void reset() noexcept {
        frame_count_ = 0;
        valid_ = false;
        used_replacement_ = false;
        last_error_ = genesis_enhanced_recomposition_error::none;
        family_status_ = {};
    }

    // Strict scientific/control transaction. Any bad requested source rejects
    // the entire candidate block and restores the protected reference exactly.
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
        for (std::size_t source_index = 0;
             source_index < genesis_recomposition_source_count;
             ++source_index) {
            if (!sources[source_index].replace)
                continue;
            any_requested = true;
            const auto error = apply_one_source(
                sources[source_index],
                left_.data(),
                right_.data(),
                frame_count_);
            if (error != genesis_enhanced_recomposition_error::none)
                return fail_to_reference(reference_left, reference_right, error);
        }

        used_replacement_ = any_requested;
        valid_ = true;
        last_error_ = genesis_enhanced_recomposition_error::none;
        return true;
    }

    // Product-facing Genesis policy. FM, DAC, and PSG are separate evidence
    // families: a bad candidate in one family keeps that family's protected
    // reference contribution while independently proven families remain applied.
    // Global reference failure still rejects the whole block.
    bool build_independent_families(
        const float* reference_left,
        const float* reference_right,
        std::size_t frame_count,
        const source_array& sources) noexcept
    {
        reset();
        if (!begin_reference(reference_left, reference_right, frame_count))
            return false;

        for (std::size_t family_index = 0;
             family_index < genesis_recomposition_family_count;
             ++family_index) {
            const auto family = static_cast<genesis_recomposition_family>(family_index);
            const auto bounds = family_source_bounds(family);
            auto& status = family_status_[family_index];

            for (std::size_t frame = 0; frame < frame_count_; ++frame) {
                scratch_left_[frame] = left_[frame];
                scratch_right_[frame] = right_[frame];
            }

            for (std::size_t source_index = bounds.first;
                 source_index < bounds.second;
                 ++source_index) {
                const auto& source = sources[source_index];
                if (!source.replace)
                    continue;
                status.requested = true;
                const auto error = apply_one_source(
                    source,
                    scratch_left_.data(),
                    scratch_right_.data(),
                    frame_count_);
                if (error != genesis_enhanced_recomposition_error::none) {
                    status.error = error;
                    break;
                }
            }

            if (!status.requested ||
                status.error != genesis_enhanced_recomposition_error::none)
                continue;

            for (std::size_t frame = 0; frame < frame_count_; ++frame) {
                left_[frame] = scratch_left_[frame];
                right_[frame] = scratch_right_[frame];
            }
            status.applied = true;
            used_replacement_ = true;
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

    genesis_recomposition_family_status family_status(
        genesis_recomposition_family family) const noexcept
    {
        const auto index = static_cast<std::size_t>(family);
        return index < family_status_.size()
            ? family_status_[index]
            : genesis_recomposition_family_status{};
    }

    bool had_family_failure() const noexcept {
        if (!valid_)
            return false;
        for (const auto& status : family_status_) {
            if (status.requested && !status.applied &&
                status.error != genesis_enhanced_recomposition_error::none)
                return true;
        }
        return false;
    }

private:
    static std::pair<std::size_t, std::size_t> family_source_bounds(
        genesis_recomposition_family family) noexcept
    {
        switch (family) {
        case genesis_recomposition_family::ym2612_fm:
            return {
                static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1),
                static_cast<std::size_t>(genesis_recomposition_source::ym2612_dac),
            };
        case genesis_recomposition_family::ym2612_dac:
            return {
                static_cast<std::size_t>(genesis_recomposition_source::ym2612_dac),
                static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0),
            };
        case genesis_recomposition_family::sn76489_psg:
            return {
                static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0),
                static_cast<std::size_t>(genesis_recomposition_source::count),
            };
        case genesis_recomposition_family::count:
            break;
        }
        return {0, 0};
    }

    bool begin_reference(
        const float* reference_left,
        const float* reference_right,
        std::size_t frame_count) noexcept
    {
        if (reference_left == nullptr || reference_right == nullptr || frame_count == 0)
            return fail(genesis_enhanced_recomposition_error::invalid_reference);
        if (frame_count > MaxFrames)
            return fail(genesis_enhanced_recomposition_error::frame_count_too_large);

        frame_count_ = frame_count;
        if (!copy_reference(reference_left, reference_right, frame_count))
            return fail_to_reference(
                reference_left,
                reference_right,
                genesis_enhanced_recomposition_error::nonfinite_sample);
        return true;
    }

    static bool representable_float(double value) noexcept {
        if (!std::isfinite(value))
            return false;
        const double limit = static_cast<double>(std::numeric_limits<float>::max());
        return value >= -limit && value <= limit;
    }

    static genesis_enhanced_recomposition_error apply_one_source(
        const genesis_source_replacement_view& source,
        float* destination_left,
        float* destination_right,
        std::size_t frames) noexcept
    {
        if (!source.reference.present() || !source.reference.exact)
            return genesis_enhanced_recomposition_error::missing_exact_reference_source;
        if (!source.enhanced.present() || !source.enhanced.exact)
            return genesis_enhanced_recomposition_error::missing_enhanced_source;

        for (std::size_t frame = 0; frame < frames; ++frame) {
            const float ref_l = source.reference.left[frame];
            const float ref_r = source.reference.right[frame];
            const float enh_l = source.enhanced.left[frame];
            const float enh_r = source.enhanced.right[frame];
            if (!std::isfinite(ref_l) || !std::isfinite(ref_r) ||
                !std::isfinite(enh_l) || !std::isfinite(enh_r))
                return genesis_enhanced_recomposition_error::nonfinite_sample;

            const double out_l = static_cast<double>(destination_left[frame])
                + static_cast<double>(enh_l) - static_cast<double>(ref_l);
            const double out_r = static_cast<double>(destination_right[frame])
                + static_cast<double>(enh_r) - static_cast<double>(ref_r);
            if (!representable_float(out_l) || !representable_float(out_r))
                return genesis_enhanced_recomposition_error::nonfinite_sample;
            destination_left[frame] = static_cast<float>(out_l);
            destination_right[frame] = static_cast<float>(out_r);
        }
        return genesis_enhanced_recomposition_error::none;
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
        family_status_ = {};
        last_error_ = error;
        return false;
    }

    bool fail(genesis_enhanced_recomposition_error error) noexcept {
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
    std::array<genesis_recomposition_family_status,
               genesis_recomposition_family_count> family_status_{};
    std::size_t frame_count_ = 0;
    bool valid_ = false;
    bool used_replacement_ = false;
    genesis_enhanced_recomposition_error last_error_ =
        genesis_enhanced_recomposition_error::none;
};

} // namespace gameaudio::vgm
