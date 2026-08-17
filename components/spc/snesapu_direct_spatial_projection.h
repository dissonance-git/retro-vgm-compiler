#pragma once

#include "snesapu_source_object_projection.h"
#include "spc_spatial_source.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

// Conservative direct bridge for the editable SNESAPU child transport. Physical
// S-DSP voices are the only initial identity claim. The shared realtime musical
// frontend learns role hypotheses from completed PCM for later blocks; this
// layer does not resurrect the historical block-end semantic classifier.
template <std::size_t MaxFrames = snesapu_source_transport_v2::max_frames,
          std::size_t MaxEvents = 512>
class snesapu_direct_spatial_projection_storage {
public:
    using projection_type =
        snesapu_source_object_projection_storage<MaxFrames, MaxEvents>;

    void reset(std::uint32_t session_generation = 1u) noexcept {
        projection_.reset();
        generation_ = session_generation == 0u ? 1u : session_generation;
        valid_ = false;
    }

    bool build(const snesapu_source_transport_v2::view& source) noexcept {
        valid_ = false;
        if (!source.valid() || source.frame_count() == 0 || source.frame_count() > MaxFrames)
            return false;

        for (std::size_t voice = 0; voice < 8u; ++voice) {
            lanes_[voice] = {};
            lanes_[voice].kind = vgmtooling::model::spatial_audio_lane_kind::dry_source;
            auto& evidence = lanes_[voice].evidence;
            evidence.family = vgmtooling::model::spatial_source_family::spc;
            evidence.source_id = spc_spatial_source_id(
                static_cast<std::uint8_t>(voice), generation_);
            evidence.generation = generation_;
            evidence.physical_slot_present = true;
            evidence.physical_slot = static_cast<std::uint32_t>(voice);
            // The exact per-sample signed route arrives in SRCE gain planes.
            // snesapu_source_object_projection replaces this empty summary with
            // its signed-RMS route evidence and marks the gain preapplied.
        }

        evidence_segment_.lanes = lanes_.data();
        evidence_segment_.lane_count = lanes_.size();
        evidence_segment_.frame_count = source.frame_count();
        evidence_segment_.evidence_events = nullptr;
        evidence_segment_.evidence_event_count = 0;

        if (!projection_.build(source, 0u, evidence_segment_, generation_))
            return false;
        valid_ = true;
        return true;
    }

    bool valid() const noexcept { return valid_ && projection_.valid(); }
    const vgmtooling::model::spatial_source_block_view& block() const noexcept {
        return projection_.block();
    }
    snesapu_source_object_projection_error last_error() const noexcept {
        return projection_.last_error();
    }
    std::uint32_t generation() const noexcept { return generation_; }

private:
    projection_type projection_{};
    std::array<vgmtooling::model::spatial_audio_lane_view, 8> lanes_{};
    vgmtooling::model::spatial_source_block_view evidence_segment_{};
    std::uint32_t generation_ = 1u;
    bool valid_ = false;
};

} // namespace gameaudio::spc
