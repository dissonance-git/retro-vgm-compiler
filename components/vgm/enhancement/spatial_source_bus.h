#pragma once

#include "source_family_recomposition.h"
#include "../../../model/spatial_source.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

enum class spatial_source_bus_error : std::uint8_t {
    none = 0,
    invalid_frames,
    no_exact_sources,
    inexact_source,
    invalid_evidence,
    nonfinite_source,
    incoherent_stereo_polarity,
};

// Presentation adapter for already-selected exact VGM source realizations.
// Source quality is resolved before this boundary; this adapter has no setting
// that can choose reference versus enhanced audio.
//
// The current contract supports isolated source lanes whose device-authored L/R
// routing is non-inverting. Their actual sample-wise stereo energy is projected
// to one causal mono trajectory, while the authored route remains attached as
// evidence with gain_preapplied=true so Omniphony cannot multiply native gain a
// second time. A future chip with proven signed/phase-inverting routing requires
// an explicit projection policy rather than silent reuse of this one.
template <std::size_t SourceCount, std::size_t MaxFrames = 8192>
class spatial_source_bus_storage {
    static_assert(SourceCount > 0, "spatial source count must be non-zero");
    static_assert(MaxFrames > 0, "MaxFrames must be non-zero");

public:
    static constexpr std::size_t source_capacity = SourceCount;
    using source_array = std::array<source_family_stereo_view, source_capacity>;
    using evidence_array =
        std::array<vgmtooling::model::spatial_source_evidence, source_capacity>;

    void reset() noexcept {
        block_ = {};
        lane_count_ = 0;
        frame_count_ = 0;
        valid_ = false;
        last_error_ = spatial_source_bus_error::none;
        canonical_index_.fill(source_capacity);
    }

    bool build(
        const source_array& selected_sources,
        const evidence_array& evidence,
        std::size_t frame_count) noexcept
    {
        reset();
        if (frame_count == 0 || frame_count > MaxFrames)
            return fail(spatial_source_bus_error::invalid_frames);

        frame_count_ = frame_count;
        for (std::size_t source_index = 0; source_index < source_capacity; ++source_index) {
            const auto& source = selected_sources[source_index];
            if (!source.present())
                continue;
            if (!source.exact)
                return fail(spatial_source_bus_error::inexact_source);

            auto source_evidence = evidence[source_index];
            if (source_evidence.family != vgmtooling::model::spatial_source_family::vgm
                || source_evidence.source_id == 0
                || !source_evidence.stereo_route.present)
                return fail(spatial_source_bus_error::invalid_evidence);

            const std::size_t lane_index = lane_count_;
            float* mono = mono_.data() + lane_index * MaxFrames;
            for (std::size_t frame = 0; frame < frame_count; ++frame) {
                const double left = static_cast<double>(source.left[frame]);
                const double right = static_cast<double>(source.right[frame]);
                if (!std::isfinite(left) || !std::isfinite(right))
                    return fail(spatial_source_bus_error::nonfinite_source);

                if ((left < 0.0 && right > 0.0) || (left > 0.0 && right < 0.0))
                    return fail(spatial_source_bus_error::incoherent_stereo_polarity);

                const double magnitude = std::sqrt((left * left + right * right) * 0.5);
                if (!std::isfinite(magnitude))
                    return fail(spatial_source_bus_error::nonfinite_source);
                const double sign_source = left != 0.0 ? left : right;
                const double projected = sign_source < 0.0 ? -magnitude : magnitude;
                if (!std::isfinite(projected))
                    return fail(spatial_source_bus_error::nonfinite_source);
                mono[frame] = static_cast<float>(projected);
            }

            source_evidence.stereo_route.gain_preapplied = true;
            lanes_[lane_index] = {};
            lanes_[lane_index].kind = vgmtooling::model::spatial_audio_lane_kind::dry_source;
            lanes_[lane_index].mono_pcm = mono;
            lanes_[lane_index].evidence = source_evidence;
            canonical_index_[lane_index] = source_index;
            ++lane_count_;
        }

        if (lane_count_ == 0)
            return fail(spatial_source_bus_error::no_exact_sources);

        block_.lanes = lanes_.data();
        block_.lane_count = lane_count_;
        block_.frame_count = frame_count_;
        block_.evidence_events = nullptr;
        block_.evidence_event_count = 0;
        valid_ = true;
        last_error_ = spatial_source_bus_error::none;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    std::size_t lane_count() const noexcept { return valid_ ? lane_count_ : 0; }
    std::size_t frame_count() const noexcept { return valid_ ? frame_count_ : 0; }
    spatial_source_bus_error last_error() const noexcept { return last_error_; }

    const vgmtooling::model::spatial_source_block_view& block() const noexcept {
        return block_;
    }

    std::size_t canonical_source_index(std::size_t lane_index) const noexcept {
        return valid_ && lane_index < lane_count_
            ? canonical_index_[lane_index]
            : source_capacity;
    }

private:
    bool fail(spatial_source_bus_error error) noexcept {
        block_ = {};
        lane_count_ = 0;
        frame_count_ = 0;
        valid_ = false;
        last_error_ = error;
        return false;
    }

    std::array<float, source_capacity * MaxFrames> mono_{};
    std::array<vgmtooling::model::spatial_audio_lane_view, source_capacity> lanes_{};
    std::array<std::size_t, source_capacity> canonical_index_{};
    vgmtooling::model::spatial_source_block_view block_{};
    std::size_t lane_count_ = 0;
    std::size_t frame_count_ = 0;
    bool valid_ = false;
    spatial_source_bus_error last_error_ = spatial_source_bus_error::none;
};

} // namespace gameaudio::vgm
