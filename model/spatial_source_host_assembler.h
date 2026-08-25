#pragma once

#include "spatial_source.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace vgmtooling::model {

enum class spatial_source_host_assembler_error : std::uint8_t {
    none = 0,
    invalid_block,
    capacity_exceeded,
    layout_change_requires_drain,
    unordered_evidence,
    evidence_identity_violation,
};

constexpr bool same_spatial_source_identity(
    const spatial_source_evidence& a,
    const spatial_source_evidence& b) noexcept
{
    return a.source_id == b.source_id &&
        a.generation == b.generation &&
        a.family == b.family;
}

constexpr bool same_stereo_route_evidence(
    const stereo_route_evidence& a,
    const stereo_route_evidence& b) noexcept
{
    return a.present == b.present &&
        a.left_gain == b.left_gain &&
        a.right_gain == b.right_gain &&
        a.authority == b.authority &&
        a.gain_preapplied == b.gain_preapplied;
}

constexpr bool same_spatial_presentation_evidence(
    const spatial_presentation_evidence& a,
    const spatial_presentation_evidence& b) noexcept
{
    return a.foundation == b.foundation &&
        a.foreground == b.foreground &&
        a.diffuse == b.diffuse &&
        a.width == b.width &&
        a.vertical_affinity == b.vertical_affinity &&
        a.confidence == b.confidence &&
        a.authority == b.authority;
}

constexpr bool same_spatial_source_evidence(
    const spatial_source_evidence& a,
    const spatial_source_evidence& b) noexcept
{
    return same_spatial_source_identity(a, b) &&
        a.physical_slot_present == b.physical_slot_present &&
        a.physical_slot == b.physical_slot &&
        same_stereo_route_evidence(a.stereo_route, b.stereo_route) &&
        a.effect_send_known == b.effect_send_known &&
        a.effect_send_enabled == b.effect_send_enabled &&
        a.persistent_part_present == b.persistent_part_present &&
        a.persistent_part_id == b.persistent_part_id &&
        a.persistent_part_confidence == b.persistent_part_confidence &&
        same_spatial_presentation_evidence(a.presentation, b.presentation) &&
        a.authored_position_present == b.authored_position_present &&
        a.authored_position[0] == b.authored_position[0] &&
        a.authored_position[1] == b.authored_position[1] &&
        a.authored_position[2] == b.authored_position[2];
}

// Bounded, allocation-free adapter between source-native render blocks and a
// host such as foobar2000 that may consume arbitrary frame counts. It owns only
// the transient transport window: source identity/evidence remains authoritative
// in the producer model and is copied without reinterpretation.
template <std::size_t MaxLanes = 64,
          std::size_t CapacityFrames = 4096,
          std::size_t MaxEvents = 512>
class spatial_source_host_assembler {
    static_assert(MaxLanes > 0, "MaxLanes must be non-zero");
    static_assert(CapacityFrames > 0, "CapacityFrames must be non-zero");
    static_assert(MaxEvents > 0, "MaxEvents must be non-zero");

public:
    void reset() noexcept {
        configured_ = false;
        lane_count_ = 0;
        buffer_head_index_ = 0;
        read_position_ = 0;
        write_position_ = 0;
        buffered_frames_ = 0;
        event_count_ = 0;
        output_event_count_ = 0;
        last_error_ = spatial_source_host_assembler_error::none;
        last_pull_identity_limited_ = false;
    }

    bool push(const spatial_source_block_view& block) noexcept {
        last_error_ = spatial_source_host_assembler_error::none;

        if (!valid_block_shape(block))
            return fail(spatial_source_host_assembler_error::invalid_block);
        if (block.lane_count > MaxLanes)
            return fail(spatial_source_host_assembler_error::invalid_block);
        if (block.frame_count > CapacityFrames - buffered_frames_)
            return fail(spatial_source_host_assembler_error::capacity_exceeded);

        if (configured_) {
            if (block.lane_count != lane_count_)
                return fail(spatial_source_host_assembler_error::layout_change_requires_drain);
            for (std::size_t lane = 0; lane < lane_count_; ++lane) {
                if (block.lanes[lane].kind != lane_kinds_[lane])
                    return fail(spatial_source_host_assembler_error::layout_change_requires_drain);
            }
        }

        std::array<spatial_source_evidence, MaxLanes> prospective_tail{};
        std::size_t additional_events = block.evidence_event_count;
        if (configured_) {
            prospective_tail = tail_evidence_;
            for (std::size_t lane = 0; lane < lane_count_; ++lane) {
                if (!same_spatial_source_evidence(prospective_tail[lane], block.lanes[lane].evidence)) {
                    ++additional_events;
                    prospective_tail[lane] = block.lanes[lane].evidence;
                }
            }
        } else {
            for (std::size_t lane = 0; lane < block.lane_count; ++lane)
                prospective_tail[lane] = block.lanes[lane].evidence;
        }

        if (additional_events > MaxEvents - event_count_)
            return fail(spatial_source_host_assembler_error::capacity_exceeded);

        std::size_t previous_offset = 0;
        bool have_previous_offset = false;
        for (std::size_t index = 0; index < block.evidence_event_count; ++index) {
            const auto& event = block.evidence_events[index];
            if (event.lane_index >= block.lane_count ||
                event.frame_offset > block.frame_count)
                return fail(spatial_source_host_assembler_error::invalid_block);
            if (have_previous_offset && event.frame_offset < previous_offset)
                return fail(spatial_source_host_assembler_error::unordered_evidence);
            if (!same_spatial_source_identity(
                    prospective_tail[event.lane_index], event.evidence))
                return fail(spatial_source_host_assembler_error::evidence_identity_violation);
            previous_offset = event.frame_offset;
            have_previous_offset = true;
            prospective_tail[event.lane_index] = event.evidence;
        }

        const std::uint64_t block_start = write_position_;
        if (block.frame_count > std::numeric_limits<std::uint64_t>::max() - block_start)
            return fail(spatial_source_host_assembler_error::capacity_exceeded);

        if (!configured_) {
            configured_ = true;
            lane_count_ = block.lane_count;
            for (std::size_t lane = 0; lane < lane_count_; ++lane) {
                lane_kinds_[lane] = block.lanes[lane].kind;
                head_evidence_[lane] = block.lanes[lane].evidence;
                tail_evidence_[lane] = block.lanes[lane].evidence;
            }
        } else {
            for (std::size_t lane = 0; lane < lane_count_; ++lane) {
                const auto& incoming = block.lanes[lane].evidence;
                if (!same_spatial_source_evidence(tail_evidence_[lane], incoming)) {
                    append_event(
                        block_start,
                        lane,
                        incoming,
                        !same_spatial_source_identity(tail_evidence_[lane], incoming));
                    tail_evidence_[lane] = incoming;
                }
            }
        }

        for (std::size_t frame = 0; frame < block.frame_count; ++frame) {
            const std::size_t destination =
                (buffer_head_index_ + buffered_frames_ + frame) % CapacityFrames;
            for (std::size_t lane = 0; lane < block.lane_count; ++lane) {
                const auto& source_lane = block.lanes[lane];
                const bool available = source_lane.mono_pcm != nullptr &&
                    (source_lane.availability == nullptr || source_lane.availability[frame] != 0);
                const std::size_t storage_index = lane * CapacityFrames + destination;
                if (storage_index >= pcm_.size() || storage_index >= availability_.size())
                    return fail(spatial_source_host_assembler_error::invalid_block);
                pcm_[storage_index] = available ? source_lane.mono_pcm[frame] : 0.0f;
                availability_[storage_index] = available ? 1u : 0u;
            }
        }

        for (std::size_t index = 0; index < block.evidence_event_count; ++index) {
            const auto& event = block.evidence_events[index];
            append_event(
                block_start + static_cast<std::uint64_t>(event.frame_offset),
                event.lane_index,
                event.evidence,
                false);
            tail_evidence_[event.lane_index] = event.evidence;
        }

        write_position_ += static_cast<std::uint64_t>(block.frame_count);
        buffered_frames_ += block.frame_count;
        return true;
    }

    // Consumes up to max_frames. A source identity/generation transition is a
    // hard output boundary: it is never smuggled through an evidence event whose
    // contract describes state changes within one source episode.
    spatial_source_block_view pull(std::size_t max_frames) noexcept {
        output_event_count_ = 0;
        last_pull_identity_limited_ = false;
        if (!configured_ || buffered_frames_ == 0 || max_frames == 0)
            return {};

        normalize_head_events();
        std::size_t frame_count = std::min(max_frames, buffered_frames_);
        const std::uint64_t requested_end = read_position_ + static_cast<std::uint64_t>(frame_count);
        for (std::size_t index = 0; index < event_count_; ++index) {
            const queued_event& event = events_[index];
            if (event.identity_break && event.absolute_frame > read_position_ &&
                event.absolute_frame < requested_end) {
                frame_count = static_cast<std::size_t>(event.absolute_frame - read_position_);
                last_pull_identity_limited_ = true;
                break;
            }
        }

        const std::uint64_t output_end = read_position_ + static_cast<std::uint64_t>(frame_count);
        for (std::size_t lane = 0; lane < lane_count_; ++lane) {
            for (std::size_t frame = 0; frame < frame_count; ++frame) {
                const std::size_t source = (buffer_head_index_ + frame) % CapacityFrames;
                output_pcm_[lane * CapacityFrames + frame] = pcm_[lane * CapacityFrames + source];
                output_availability_[lane * CapacityFrames + frame] =
                    availability_[lane * CapacityFrames + source];
            }
            output_lanes_[lane].kind = lane_kinds_[lane];
            output_lanes_[lane].mono_pcm = output_pcm_.data() + lane * CapacityFrames;
            output_lanes_[lane].evidence = head_evidence_[lane];
            output_lanes_[lane].availability = output_availability_.data() + lane * CapacityFrames;
        }

        for (std::size_t index = 0; index < event_count_; ++index) {
            const queued_event& event = events_[index];
            if (event.absolute_frame <= read_position_)
                continue;
            if (event.absolute_frame >= output_end)
                break;
            spatial_source_evidence_event& output = output_events_[output_event_count_++];
            output.frame_offset = static_cast<std::size_t>(event.absolute_frame - read_position_);
            output.lane_index = event.lane_index;
            output.evidence = event.evidence;
        }

        spatial_source_block_view result;
        result.lanes = output_lanes_.data();
        result.lane_count = lane_count_;
        result.frame_count = frame_count;
        result.evidence_events = output_event_count_ == 0 ? nullptr : output_events_.data();
        result.evidence_event_count = output_event_count_;

        buffer_head_index_ = (buffer_head_index_ + frame_count) % CapacityFrames;
        buffered_frames_ -= frame_count;
        read_position_ = output_end;
        normalize_head_events();

        if (buffered_frames_ == 0) {
            configured_ = false;
            lane_count_ = 0;
            read_position_ = 0;
            write_position_ = 0;
            buffer_head_index_ = 0;
            event_count_ = 0;
        }
        return result;
    }

    std::size_t buffered_frames() const noexcept { return buffered_frames_; }
    std::size_t lane_count() const noexcept { return lane_count_; }
    std::size_t queued_event_count() const noexcept { return event_count_; }
    spatial_source_host_assembler_error last_error() const noexcept { return last_error_; }
    bool last_pull_identity_limited() const noexcept { return last_pull_identity_limited_; }

private:
    struct queued_event {
        std::uint64_t absolute_frame = 0;
        std::size_t lane_index = 0;
        spatial_source_evidence evidence{};
        bool identity_break = false;
    };

    bool valid_block_shape(const spatial_source_block_view& block) const noexcept {
        if (block.lane_count == 0 || block.lane_count > MaxLanes)
            return false;
        if (block.lanes == nullptr)
            return false;
        if (block.evidence_event_count != 0 && block.evidence_events == nullptr)
            return false;
        return true;
    }

    bool fail(spatial_source_host_assembler_error error) noexcept {
        last_error_ = error;
        return false;
    }

    void append_event(
        std::uint64_t absolute_frame,
        std::size_t lane_index,
        const spatial_source_evidence& evidence,
        bool identity_break) noexcept
    {
        queued_event& destination = events_[event_count_++];
        destination.absolute_frame = absolute_frame;
        destination.lane_index = lane_index;
        destination.evidence = evidence;
        destination.identity_break = identity_break;
    }

    void normalize_head_events() noexcept {
        std::size_t consumed = 0;
        while (consumed < event_count_ && events_[consumed].absolute_frame <= read_position_) {
            const queued_event& event = events_[consumed];
            if (event.lane_index < lane_count_)
                head_evidence_[event.lane_index] = event.evidence;
            ++consumed;
        }
        if (consumed == 0)
            return;
        for (std::size_t index = consumed; index < event_count_; ++index)
            events_[index - consumed] = events_[index];
        event_count_ -= consumed;
    }

    bool configured_ = false;
    std::size_t lane_count_ = 0;
    std::size_t buffer_head_index_ = 0;
    std::size_t buffered_frames_ = 0;
    std::uint64_t read_position_ = 0;
    std::uint64_t write_position_ = 0;

    std::array<spatial_audio_lane_kind, MaxLanes> lane_kinds_{};
    std::array<spatial_source_evidence, MaxLanes> head_evidence_{};
    std::array<spatial_source_evidence, MaxLanes> tail_evidence_{};
    std::array<float, MaxLanes * CapacityFrames> pcm_{};
    std::array<std::uint8_t, MaxLanes * CapacityFrames> availability_{};
    std::array<queued_event, MaxEvents> events_{};
    std::size_t event_count_ = 0;

    std::array<spatial_audio_lane_view, MaxLanes> output_lanes_{};
    std::array<float, MaxLanes * CapacityFrames> output_pcm_{};
    std::array<std::uint8_t, MaxLanes * CapacityFrames> output_availability_{};
    std::array<spatial_source_evidence_event, MaxEvents> output_events_{};
    std::size_t output_event_count_ = 0;

    spatial_source_host_assembler_error last_error_ = spatial_source_host_assembler_error::none;
    bool last_pull_identity_limited_ = false;
};

} // namespace vgmtooling::model
