#pragma once

#include "spc_runtime_capture.h"
#include "../../model/spatial_source.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gameaudio::spc {

enum class spc_runtime_spatial_adapter_error : std::uint8_t {
    none = 0,
    invalid_window,
    capture_overflow,
    continuity_lost_requires_reset,
    execution_reset_requires_reset,
    unordered_capture,
    tick_rate_mismatch,
    event_outside_window,
    frame_mapping_overflow,
    generation_overflow,
    capacity_exceeded,
};

struct spc_runtime_spatial_capture_view {
    const spc_runtime_capture_record* records = nullptr;
    std::size_t count = 0;
    bool overflowed = false;
};

struct spc_runtime_spatial_voice_state {
    std::uint64_t generation = 0;
    bool route_left_known = false;
    bool route_right_known = false;
    std::int8_t route_gain_left = 0;
    std::int8_t route_gain_right = 0;
    bool echo_send_known = false;
    bool echo_send_enabled = false;
};

struct spc_runtime_spatial_state {
    std::array<spc_runtime_spatial_voice_state, 8> voices{};

    void reset() noexcept { voices = {}; }
};

struct spc_runtime_spatial_segment_view {
    std::size_t reference_frame_offset = 0;
    vgmtooling::model::spatial_source_block_view sources{};
};

constexpr float spc_signed_route_gain(std::int8_t value) noexcept {
    return static_cast<float>(value) / 128.0f;
}

inline vgmtooling::model::spatial_source_evidence make_spc_spatial_evidence(
    std::size_t voice,
    const spc_runtime_spatial_voice_state& state) noexcept
{
    using namespace vgmtooling::model;

    spatial_source_evidence evidence;
    evidence.family = spatial_source_family::spc;
    // Source identity is the physical S-DSP voice slot plus an episode
    // generation. SRCN/sample identity is deliberately not promoted to source
    // identity: one physical voice episode may legally change source index.
    evidence.source_id = static_cast<std::uint64_t>(voice + 1u);
    evidence.generation = state.generation;
    evidence.physical_slot_present = true;
    evidence.physical_slot = static_cast<std::uint32_t>(voice);

    if (state.route_left_known && state.route_right_known) {
        evidence.stereo_route.present = true;
        evidence.stereo_route.left_gain = spc_signed_route_gain(state.route_gain_left);
        evidence.stereo_route.right_gain = spc_signed_route_gain(state.route_gain_right);
        evidence.stereo_route.authority = spatial_evidence_authority::device_authored;
    }

    evidence.effect_send_known = state.echo_send_known;
    evidence.effect_send_enabled = state.echo_send_enabled;
    return evidence;
}

inline bool map_spc_tick_to_reference_frame(
    std::int64_t tick,
    std::int64_t window_start_tick,
    std::uint64_t tick_rate,
    std::uint64_t sample_rate,
    std::uint64_t& output) noexcept
{
    if (tick_rate == 0 || sample_rate == 0 || tick < window_start_tick ||
        tick < 0 || window_start_tick < 0)
        return false;

    const std::uint64_t delta = static_cast<std::uint64_t>(tick - window_start_tick);
    const std::uint64_t whole = delta / tick_rate;
    const std::uint64_t remainder = delta % tick_rate;

    if (whole > std::numeric_limits<std::uint64_t>::max() / sample_rate)
        return false;
    const std::uint64_t whole_frames = whole * sample_rate;

    if (remainder != 0 && sample_rate > std::numeric_limits<std::uint64_t>::max() / remainder)
        return false;
    const std::uint64_t fractional_frames = (remainder * sample_rate) / tick_rate;
    if (whole_frames > std::numeric_limits<std::uint64_t>::max() - fractional_frames)
        return false;

    output = whole_frames + fractional_frames;
    return true;
}

// Converts exact instrumented S-DSP runtime observations into the shared host
// source contract. The adapter intentionally emits evidence-only lanes: until a
// validated per-voice dry tap exists, mono_pcm remains null and downstream host
// transport must preserve that unavailability instead of inventing silence.
//
// A key-on changes the physical voice episode generation and therefore creates a
// hard segment boundary. Pan/echo state changes within one episode remain timed
// evidence events. The shared S-DSP echo return is never attributed to a voice
// merely because that voice has echo-send enabled.
template <std::size_t MaxSegments = 64, std::size_t MaxEvidenceEvents = 512>
class spc_runtime_spatial_adapter {
    static_assert(MaxSegments > 0, "MaxSegments must be non-zero");
    static_assert(MaxEvidenceEvents > 0, "MaxEvidenceEvents must be non-zero");

public:
    void reset() noexcept {
        state_.reset();
        segment_count_ = 0;
        event_count_ = 0;
        expected_trace_index_valid_ = false;
        expected_trace_index_ = 0;
        last_error_ = spc_runtime_spatial_adapter_error::none;
    }

    bool build_window(
        const spc_runtime_spatial_capture_view& capture,
        std::int64_t window_start_tick,
        std::uint64_t tick_rate,
        std::uint64_t sample_rate,
        std::size_t reference_frame_count) noexcept
    {
        last_error_ = spc_runtime_spatial_adapter_error::none;
        segment_count_ = 0;
        event_count_ = 0;

        if (reference_frame_count == 0 || tick_rate == 0 || sample_rate == 0 ||
            window_start_tick < 0 || (capture.count != 0 && capture.records == nullptr))
            return fail(spc_runtime_spatial_adapter_error::invalid_window);
        if (capture.overflowed)
            return fail(spc_runtime_spatial_adapter_error::capture_overflow);

        // First pass is transactional validation and exact capacity accounting.
        // Persistent state is not touched until the whole capture window proves
        // contiguous and representable.
        spc_runtime_spatial_state simulated_state = state_;
        std::size_t simulated_segments = 1;
        std::size_t simulated_events = 0;
        std::size_t simulated_segment_start = 0;
        std::uint64_t previous_frame = 0;
        std::int64_t previous_tick = window_start_tick;
        bool have_previous = false;
        std::uint64_t expected_trace = expected_trace_index_;
        bool expected_trace_valid = expected_trace_index_valid_;

        for (std::size_t index = 0; index < capture.count; ++index) {
            const auto& record = capture.records[index];
            if (record.kind == spc_voice_runtime_event_kind::continuation_lost)
                return fail(spc_runtime_spatial_adapter_error::continuity_lost_requires_reset);
            if (record.kind == spc_voice_runtime_event_kind::execution_reset)
                return fail(spc_runtime_spatial_adapter_error::execution_reset_requires_reset);
            if (record.tick_rate != tick_rate)
                return fail(spc_runtime_spatial_adapter_error::tick_rate_mismatch);
            if (have_previous && record.tick < previous_tick)
                return fail(spc_runtime_spatial_adapter_error::unordered_capture);
            if (expected_trace_valid && record.trace_index != expected_trace)
                return fail(spc_runtime_spatial_adapter_error::continuity_lost_requires_reset);
            expected_trace = record.trace_index + 1u;
            expected_trace_valid = true;

            std::uint64_t mapped_frame_u64 = 0;
            if (!map_spc_tick_to_reference_frame(
                    record.tick,
                    window_start_tick,
                    tick_rate,
                    sample_rate,
                    mapped_frame_u64))
                return fail(spc_runtime_spatial_adapter_error::frame_mapping_overflow);
            if (mapped_frame_u64 > reference_frame_count)
                return fail(spc_runtime_spatial_adapter_error::event_outside_window);
            if (have_previous && mapped_frame_u64 < previous_frame)
                return fail(spc_runtime_spatial_adapter_error::unordered_capture);

            const std::size_t mapped_frame = static_cast<std::size_t>(mapped_frame_u64);
            if (record.kind == spc_voice_runtime_event_kind::key_on_accepted) {
                if (!has_valid_voice(record))
                    return fail(spc_runtime_spatial_adapter_error::invalid_window);
                auto& voice = simulated_state.voices[record.voice];
                if (voice.generation == std::numeric_limits<std::uint64_t>::max())
                    return fail(spc_runtime_spatial_adapter_error::generation_overflow);
                if (mapped_frame > simulated_segment_start && mapped_frame < reference_frame_count)
                    ++simulated_segments;
                simulated_segment_start = mapped_frame;
                ++voice.generation;
            }

            if (record_changes_spatial_state(record)) {
                if (!has_valid_voice(record))
                    return fail(spc_runtime_spatial_adapter_error::invalid_window);
                const bool changed = apply_record_to_voice_state(
                    record,
                    simulated_state.voices[record.voice]);
                if (changed && record.kind != spc_voice_runtime_event_kind::key_on_accepted &&
                    mapped_frame > simulated_segment_start && mapped_frame < reference_frame_count)
                    ++simulated_events;
            }

            previous_tick = record.tick;
            previous_frame = mapped_frame_u64;
            have_previous = true;
        }

        if (simulated_segments > MaxSegments || simulated_events > MaxEvidenceEvents)
            return fail(spc_runtime_spatial_adapter_error::capacity_exceeded);

        std::size_t segment_start = 0;
        std::size_t segment_event_start = 0;
        snapshot_segment_lanes(state_);

        std::size_t index = 0;
        while (index < capture.count) {
            std::uint64_t mapped_frame_u64 = 0;
            (void)map_spc_tick_to_reference_frame(
                capture.records[index].tick,
                window_start_tick,
                tick_rate,
                sample_rate,
                mapped_frame_u64);
            const std::size_t mapped_frame = static_cast<std::size_t>(mapped_frame_u64);

            std::size_t group_end = index;
            bool identity_break = false;
            while (group_end < capture.count) {
                std::uint64_t candidate_frame = 0;
                (void)map_spc_tick_to_reference_frame(
                    capture.records[group_end].tick,
                    window_start_tick,
                    tick_rate,
                    sample_rate,
                    candidate_frame);
                if (candidate_frame != mapped_frame_u64)
                    break;
                if (capture.records[group_end].kind == spc_voice_runtime_event_kind::key_on_accepted)
                    identity_break = true;
                ++group_end;
            }

            if (identity_break && mapped_frame > segment_start) {
                finalize_segment(segment_start, mapped_frame, segment_event_start);
                segment_start = mapped_frame;
                segment_event_start = event_count_;
            }

            for (std::size_t current = index; current < group_end; ++current) {
                const auto& record = capture.records[current];
                if (record.kind == spc_voice_runtime_event_kind::key_on_accepted)
                    ++state_.voices[record.voice].generation;

                bool changed = false;
                if (record_changes_spatial_state(record))
                    changed = apply_record_to_voice_state(record, state_.voices[record.voice]);
                if (record.kind == spc_voice_runtime_event_kind::key_on_accepted)
                    changed = true;

                if (!changed)
                    continue;

                // A state observation exactly at the end boundary belongs to the
                // persistent state entering the next window. It must not rewrite
                // this segment's frame-zero evidence or appear as an in-block
                // event for audio frames that precede it.
                if (mapped_frame == reference_frame_count)
                    continue;

                const auto evidence = make_spc_spatial_evidence(record.voice, state_.voices[record.voice]);
                if (mapped_frame == segment_start || identity_break) {
                    current_segment_lanes_[record.voice].evidence = evidence;
                } else {
                    auto& destination = events_[event_count_++];
                    destination.frame_offset = mapped_frame - segment_start;
                    destination.lane_index = record.voice;
                    destination.evidence = evidence;
                }
            }

            if (identity_break)
                snapshot_segment_lanes(state_);

            index = group_end;
        }

        if (segment_start < reference_frame_count)
            finalize_segment(segment_start, reference_frame_count, segment_event_start);

        if (capture.count != 0) {
            expected_trace_index_ = capture.records[capture.count - 1].trace_index + 1u;
            expected_trace_index_valid_ = true;
        }
        return true;
    }

    const spc_runtime_spatial_segment_view* segments() const noexcept {
        return segments_.data();
    }
    std::size_t segment_count() const noexcept { return segment_count_; }
    const spc_runtime_spatial_state& state() const noexcept { return state_; }
    spc_runtime_spatial_adapter_error last_error() const noexcept { return last_error_; }

private:
    static bool has_valid_voice(const spc_runtime_capture_record& record) noexcept {
        return has_field(record.fields, spc_runtime_capture_field::voice) && record.voice < 8;
    }

    static bool record_changes_spatial_state(const spc_runtime_capture_record& record) noexcept {
        return has_field(record.fields, spc_runtime_capture_field::route_gain_left) ||
            has_field(record.fields, spc_runtime_capture_field::route_gain_right) ||
            has_field(record.fields, spc_runtime_capture_field::echo_send_enabled);
    }

    static bool apply_record_to_voice_state(
        const spc_runtime_capture_record& record,
        spc_runtime_spatial_voice_state& state) noexcept
    {
        bool changed = false;
        if (has_field(record.fields, spc_runtime_capture_field::route_gain_left)) {
            changed = changed || !state.route_left_known || state.route_gain_left != record.route_gain_left;
            state.route_left_known = true;
            state.route_gain_left = record.route_gain_left;
        }
        if (has_field(record.fields, spc_runtime_capture_field::route_gain_right)) {
            changed = changed || !state.route_right_known || state.route_gain_right != record.route_gain_right;
            state.route_right_known = true;
            state.route_gain_right = record.route_gain_right;
        }
        if (has_field(record.fields, spc_runtime_capture_field::echo_send_enabled)) {
            changed = changed || !state.echo_send_known || state.echo_send_enabled != record.echo_send_enabled;
            state.echo_send_known = true;
            state.echo_send_enabled = record.echo_send_enabled;
        }
        return changed;
    }

    void snapshot_segment_lanes(const spc_runtime_spatial_state& state) noexcept {
        for (std::size_t voice = 0; voice < current_segment_lanes_.size(); ++voice) {
            current_segment_lanes_[voice] = {};
            current_segment_lanes_[voice].kind = vgmtooling::model::spatial_audio_lane_kind::dry_source;
            current_segment_lanes_[voice].mono_pcm = nullptr;
            current_segment_lanes_[voice].availability = nullptr;
            current_segment_lanes_[voice].evidence = make_spc_spatial_evidence(voice, state.voices[voice]);
        }
    }

    void finalize_segment(
        std::size_t start,
        std::size_t end,
        std::size_t event_start) noexcept
    {
        if (end <= start)
            return;

        const std::size_t lane_base = segment_count_ * 8u;
        for (std::size_t voice = 0; voice < 8; ++voice)
            lane_storage_[lane_base + voice] = current_segment_lanes_[voice];

        auto& segment = segments_[segment_count_++];
        segment.reference_frame_offset = start;
        segment.sources.lanes = lane_storage_.data() + lane_base;
        segment.sources.lane_count = 8;
        segment.sources.frame_count = end - start;
        segment.sources.evidence_events = event_count_ == event_start
            ? nullptr
            : events_.data() + event_start;
        segment.sources.evidence_event_count = event_count_ - event_start;
    }

    bool fail(spc_runtime_spatial_adapter_error error) noexcept {
        last_error_ = error;
        segment_count_ = 0;
        event_count_ = 0;
        return false;
    }

    spc_runtime_spatial_state state_{};
    std::array<vgmtooling::model::spatial_audio_lane_view, 8> current_segment_lanes_{};
    std::array<vgmtooling::model::spatial_audio_lane_view, MaxSegments * 8u> lane_storage_{};
    std::array<vgmtooling::model::spatial_source_evidence_event, MaxEvidenceEvents> events_{};
    std::array<spc_runtime_spatial_segment_view, MaxSegments> segments_{};
    std::size_t segment_count_ = 0;
    std::size_t event_count_ = 0;
    bool expected_trace_index_valid_ = false;
    std::uint64_t expected_trace_index_ = 0;
    spc_runtime_spatial_adapter_error last_error_ = spc_runtime_spatial_adapter_error::none;
};

} // namespace gameaudio::spc
