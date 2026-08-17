#pragma once

#include "realtime_musical_role_tracker.h"
#include "realtime_musical_spatial_projection.h"

#include <array>
#include <cmath>
#include <cstddef>

namespace vgmtooling::model {

// Past-only semantic state paired with one current source lane. Audio ownership
// remains in spatial_source_block_view; this handoff is a bounded sidecar.
struct realtime_musical_spatial_lane_state {
    spatial_source_evidence evidence{};
    bool roles_available = false;
    realtime_musical_role_hypotheses roles{};
};

// A timed source-evidence event may cross an identity boundary inside the audio
// block. The handoff performs a role-memory lookup at the same exact boundary so
// a renderer can reset or resume semantic state without treating a physical lane
// as the musical identity.
struct realtime_musical_spatial_role_event {
    std::size_t frame_offset = 0;
    std::size_t lane_index = 0;
    spatial_source_evidence evidence{};
    bool roles_available = false;
    realtime_musical_role_hypotheses roles{};
};

template <std::size_t MaxLanes = 64, std::size_t MaxEvents = 256>
class realtime_musical_spatial_handoff_storage {
public:
    static_assert(MaxLanes > 0, "realtime handoff requires at least one lane slot");

    void reset() noexcept {
        lane_count_ = 0;
        event_count_ = 0;
        frame_count_ = 0;
        history_seconds_ = 0.0;
        projected_view_ = {};
        valid_ = false;
    }

    bool valid() const noexcept { return valid_; }
    std::size_t lane_count() const noexcept { return lane_count_; }
    std::size_t event_count() const noexcept { return event_count_; }
    std::size_t frame_count() const noexcept { return frame_count_; }
    double history_seconds() const noexcept { return history_seconds_; }

    const realtime_musical_spatial_lane_state& lane(std::size_t index) const noexcept {
        return lanes_[index];
    }

    const realtime_musical_spatial_role_event& event(std::size_t index) const noexcept {
        return events_[index];
    }

    // A renderer-ready view over the same current PCM pointers with only the
    // presentation/evidence sidecar replaced by past-only projected state. The
    // storage owns the lane/event records, not the PCM itself. Its lifetime is
    // therefore bounded by both this handoff object and the caller's input PCM.
    const spatial_source_block_view& projected_view() const noexcept {
        return projected_view_;
    }

private:
    template <std::size_t, std::size_t, std::size_t>
    friend class realtime_musical_spatial_frontend;

    std::array<realtime_musical_spatial_lane_state, MaxLanes> lanes_{};
    std::array<realtime_musical_spatial_role_event, MaxEvents> events_{};
    std::array<spatial_audio_lane_view, MaxLanes> projected_lanes_{};
    std::array<spatial_source_evidence_event, MaxEvents> projected_events_{};
    spatial_source_block_view projected_view_{};
    std::size_t lane_count_ = 0;
    std::size_t event_count_ = 0;
    std::size_t frame_count_ = 0;
    double history_seconds_ = 0.0;
    bool valid_ = false;
};

template <
    std::size_t MaxLanes = 64,
    std::size_t MaxEvents = 256,
    std::size_t RoleCapacity = 128>
class realtime_musical_spatial_frontend {
public:
    using handoff_storage = realtime_musical_spatial_handoff_storage<MaxLanes, MaxEvents>;

    void reset() noexcept {
        observer_.reset();
        tracker_.reset();
    }

    // Phase 1: prepare the semantic sidecar for the block that is about to be
    // rendered. This function consults only source evidence available at the
    // current frame boundary and role memory learned from already-completed
    // audio. It never inspects current-block PCM.
    bool prepare_block(
        const spatial_source_block_view& input,
        handoff_storage& output) const noexcept
    {
        output.reset();
        if (input.lane_count > MaxLanes || input.evidence_event_count > MaxEvents ||
            (input.lane_count != 0 && input.lanes == nullptr) ||
            (input.evidence_event_count != 0 && input.evidence_events == nullptr))
            return false;

        std::size_t previous_event_offset = 0;
        bool have_previous_event = false;
        for (std::size_t event_index = 0; event_index < input.evidence_event_count; ++event_index) {
            const spatial_source_evidence_event& event = input.evidence_events[event_index];
            if (event.lane_index >= input.lane_count || event.frame_offset > input.frame_count ||
                (have_previous_event && event.frame_offset < previous_event_offset))
                return false;
            have_previous_event = true;
            previous_event_offset = event.frame_offset;
        }

        for (std::size_t lane_index = 0; lane_index < input.lane_count; ++lane_index) {
            const spatial_source_evidence& raw_evidence = input.lanes[lane_index].evidence;
            realtime_musical_spatial_lane_state& destination = output.lanes_[lane_index];
            destination.roles_available = tracker_.lookup(
                raw_evidence,
                destination.roles);
            destination.evidence = project_realtime_musical_spatial_evidence(
                raw_evidence,
                destination.roles_available,
                destination.roles);

            output.projected_lanes_[lane_index] = input.lanes[lane_index];
            output.projected_lanes_[lane_index].evidence = destination.evidence;
        }

        for (std::size_t event_index = 0; event_index < input.evidence_event_count; ++event_index) {
            const spatial_source_evidence_event& source_event = input.evidence_events[event_index];
            realtime_musical_spatial_role_event& destination = output.events_[event_index];
            destination.frame_offset = source_event.frame_offset;
            destination.lane_index = source_event.lane_index;
            destination.roles_available = tracker_.lookup(
                source_event.evidence,
                destination.roles);
            destination.evidence = project_realtime_musical_spatial_evidence(
                source_event.evidence,
                destination.roles_available,
                destination.roles);

            output.projected_events_[event_index] = spatial_source_evidence_event{
                source_event.frame_offset,
                source_event.lane_index,
                destination.evidence,
            };
        }

        output.projected_view_.lanes = output.projected_lanes_.data();
        output.projected_view_.lane_count = input.lane_count;
        output.projected_view_.frame_count = input.frame_count;
        output.projected_view_.evidence_events = output.projected_events_.data();
        output.projected_view_.evidence_event_count = input.evidence_event_count;
        output.lane_count_ = input.lane_count;
        output.event_count_ = input.evidence_event_count;
        output.frame_count_ = input.frame_count;
        output.history_seconds_ = tracker_.stream_seconds();
        output.valid_ = true;
        return true;
    }

    // Phase 2: call only after the audio represented by input has passed the
    // renderer. The just-completed RAW PCM/evidence may then update observations
    // and role memory for later blocks. Never feed handoff.projected_view() into
    // this phase: that would let prior semantic guesses become new evidence and
    // create a self-reinforcing feedback loop.
    //
    // Stream time advances even if observation fails, because the audio clock
    // itself still moved. A malformed/ambiguous analysis block therefore loses
    // learning rather than freezing musical time or fabricating evidence.
    bool complete_block(
        const spatial_source_block_view& input,
        double sample_rate) noexcept
    {
        if (!std::isfinite(sample_rate) || sample_rate <= 0.0)
            return false;
        if (!tracker_.advance_block(input.frame_count, sample_rate))
            return false;

        // Omniphony accepts events at frame_offset == frame_count as a
        // zero-length terminal transition and expects that state to become the
        // next block's initial evidence. There is no completed PCM under those
        // events, so they must not be paired with the just-finished block's
        // acoustic observation or role proposal. Since prepare_block already
        // validates nondecreasing offsets, terminal events form a trailing run.
        std::size_t learning_event_count = input.evidence_event_count;
        if (input.evidence_events != nullptr) {
            while (learning_event_count != 0 &&
                   input.evidence_events[learning_event_count - 1].frame_offset == input.frame_count) {
                --learning_event_count;
            }
        }

        spatial_source_block_view learning_view = input;
        learning_view.evidence_event_count = learning_event_count;
        if (!observer_.process(learning_view, sample_rate))
            return false;

        std::array<spatial_source_evidence, MaxLanes> final_evidence{};
        for (std::size_t lane_index = 0; lane_index < input.lane_count; ++lane_index)
            final_evidence[lane_index] = input.lanes[lane_index].evidence;
        for (std::size_t event_index = 0; event_index < learning_event_count; ++event_index) {
            const spatial_source_evidence_event& event = input.evidence_events[event_index];
            final_evidence[event.lane_index] = event.evidence;
        }

        for (std::size_t lane_index = 0; lane_index < input.lane_count; ++lane_index) {
            const realtime_musical_role_hypotheses hypotheses = proposer_.propose(
                final_evidence[lane_index],
                observer_.source(lane_index),
                observer_.scene());
            tracker_.observe(final_evidence[lane_index], hypotheses);
        }

        return true;
    }

    const realtime_musical_spatial_observer<MaxLanes, MaxEvents>& observer() const noexcept {
        return observer_;
    }

    const realtime_musical_role_tracker<RoleCapacity>& tracker() const noexcept {
        return tracker_;
    }

private:
    realtime_musical_spatial_observer<MaxLanes, MaxEvents> observer_{};
    realtime_musical_role_proposer proposer_{};
    realtime_musical_role_tracker<RoleCapacity> tracker_{};
};

} // namespace vgmtooling::model
