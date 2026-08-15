#pragma once

#include "qsound_block_capture.h"
#include "qsound_consumer_source_storage.h"
#include "qsound_control_state.h"

#include "../../../model/spatial_source.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Allocation-free adapter from QSound-specific consumer PCM/control evidence to
// the neutral spatial-source block contract. QSound register meanings end here:
// downstream renderers receive mono lanes plus timed spatial_source_evidence.
class qsound_spatial_source_bus_storage {
public:
    static constexpr std::size_t lane_count = qsound_source_count;
    static constexpr std::size_t event_capacity = qsound_block_capture::capacity;

    void reset() noexcept {
        event_count_ = 0;
        valid_ = false;
        final_state_ = {};
        view_ = {};
        for (auto& lane : lanes_)
            lane = {};
    }

    bool build(
        const qsound_consumer_source_storage& audio,
        const qsound_control_state& initial_state,
        const qsound_timed_source_control* writes,
        std::size_t write_count,
        bool controls_complete,
        std::size_t frame_count,
        std::uint8_t instance = 0,
        std::uint32_t episode_generation = 0) noexcept
    {
        reset();
        if (!audio.valid() || audio.frame_count() != frame_count || !controls_complete ||
            (write_count != 0 && writes == nullptr))
            return false;

        for (std::size_t slot = 0; slot < lane_count; ++slot) {
            const qsound_spatial_source source = initial_state.source(
                instance,
                static_cast<std::uint8_t>(slot),
                episode_generation);
            lanes_[slot].kind = vgmtooling::model::spatial_audio_lane_kind::dry_source;
            lanes_[slot].mono_pcm = audio.lane(slot);
            lanes_[slot].evidence = source.evidence;
            lanes_[slot].availability = audio.availability();
        }

        qsound_control_state state = initial_state;
        std::size_t previous_offset = 0;
        bool have_previous = false;
        for (std::size_t index = 0; index < write_count; ++index) {
            const qsound_timed_source_control& timed = writes[index];
            if (have_previous && timed.sample_offset < previous_offset)
                return false;
            previous_offset = timed.sample_offset;
            have_previous = true;

            if (!state.apply(timed.write) || timed.write.physical_slot >= lane_count)
                return false;

            // A write at or beyond the block end changes carry state for the
            // next block but does not describe a frame inside this block.
            if (timed.sample_offset >= frame_count)
                continue;

            if (event_count_ >= event_capacity)
                return false;

            const std::size_t lane_index = timed.write.physical_slot;
            const qsound_spatial_source source = state.source(
                instance,
                timed.write.physical_slot,
                episode_generation);
            events_[event_count_++] = vgmtooling::model::spatial_source_evidence_event{
                timed.sample_offset,
                lane_index,
                source.evidence,
            };
        }

        final_state_ = state;
        view_.lanes = lanes_.data();
        view_.lane_count = lane_count;
        view_.frame_count = frame_count;
        view_.evidence_events = events_.data();
        view_.evidence_event_count = event_count_;
        valid_ = true;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    const vgmtooling::model::spatial_source_block_view& view() const noexcept { return view_; }
    const qsound_control_state& final_state() const noexcept { return final_state_; }

private:
    std::array<vgmtooling::model::spatial_audio_lane_view, lane_count> lanes_{};
    std::array<vgmtooling::model::spatial_source_evidence_event, event_capacity> events_{};
    std::size_t event_count_ = 0;
    qsound_control_state final_state_{};
    vgmtooling::model::spatial_source_block_view view_{};
    bool valid_ = false;
};

} // namespace gameaudio::vgm
