#pragma once

#include "spatial_source.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace vgmtooling::model {

// Numeric presentation tendencies carried through the realtime DSP boundary.
// These are deliberately not coordinates. They are a smoothed, causal view of
// higher musical/perceptual evidence that a renderer such as Omniphony may map
// into its own presentation policy.
struct spatial_presentation_values {
    float foundation = 0.0f;
    float foreground = 0.0f;
    float diffuse = 0.0f;
    float width = 0.0f;
    float vertical_affinity = 0.0f;
    float confidence = 0.0f;
};

struct realtime_spatial_smoothing {
    // Fast enough to follow a new musical role without turning block-level
    // inference into audible zippering. Falling values intentionally relax
    // more slowly so uncertain role changes do not make the scene chatter.
    float rise_seconds = 0.040f;
    float fall_seconds = 0.180f;

    // Signed vertical affinity has no meaningful "rise" vs "fall" direction,
    // so it receives one symmetric motion time constant.
    float signed_motion_seconds = 0.120f;
};

// One constant-target span for one source lane. Omniphony can reproduce the
// same sample-domain smoothing recurrence without GMI precaching per-sample
// coordinates or an entire-song automation script.
//
// For unit-interval fields:
//   y[n+1] = target + (y[n] - target) * (target >= y[n] ? rise : fall)
//
// For vertical_affinity use signed_motion_coefficient.
struct realtime_spatial_control_span {
    std::size_t lane_index = 0;
    std::size_t frame_offset = 0;
    std::size_t frame_count = 0;

    spatial_source_evidence evidence{};
    spatial_presentation_values start{};
    spatial_presentation_values target{};

    float rise_coefficient = 0.0f;
    float fall_coefficient = 0.0f;
    float signed_motion_coefficient = 0.0f;
};

template <std::size_t MaxLanes, std::size_t MaxEvents>
class realtime_spatial_scene_dsp;

template <std::size_t MaxLanes = 64, std::size_t MaxEvents = 256>
class realtime_spatial_scene_block_storage {
public:
    static_assert(MaxLanes > 0, "realtime spatial scene needs at least one lane slot");
    static constexpr std::size_t max_spans = MaxLanes + MaxEvents;

    void reset() noexcept {
        span_count_ = 0;
        frame_count_ = 0;
        valid_ = false;
    }

    bool valid() const noexcept { return valid_; }
    std::size_t frame_count() const noexcept { return frame_count_; }
    std::size_t span_count() const noexcept { return span_count_; }
    const realtime_spatial_control_span* spans() const noexcept { return spans_.data(); }
    const realtime_spatial_control_span& span(std::size_t index) const noexcept {
        return spans_[index];
    }

private:
    template <std::size_t, std::size_t>
    friend class realtime_spatial_scene_dsp;

    bool push(const realtime_spatial_control_span& span) noexcept {
        if (span_count_ >= max_spans)
            return false;
        spans_[span_count_++] = span;
        return true;
    }

    void mark_valid(std::size_t frames) noexcept {
        frame_count_ = frames;
        valid_ = true;
    }

    std::array<realtime_spatial_control_span, max_spans> spans_{};
    std::size_t span_count_ = 0;
    std::size_t frame_count_ = 0;
    bool valid_ = false;
};

template <std::size_t MaxLanes, std::size_t MaxEvents>
class realtime_spatial_scene_dsp {
public:
    static_assert(MaxLanes > 0, "realtime spatial scene needs at least one lane slot");
    using block_storage = realtime_spatial_scene_block_storage<MaxLanes, MaxEvents>;

    realtime_spatial_scene_dsp() = default;
    explicit realtime_spatial_scene_dsp(realtime_spatial_smoothing smoothing) noexcept
        : smoothing_(smoothing) {}

    void reset() noexcept {
        lanes_ = {};
    }

    const realtime_spatial_smoothing& smoothing() const noexcept { return smoothing_; }

    bool set_smoothing(realtime_spatial_smoothing smoothing) noexcept {
        if (!valid_smoothing(smoothing))
            return false;
        smoothing_ = smoothing;
        return true;
    }

    // Build one causal control block from source evidence available for this
    // audio block. The function is allocation-free and atomic: malformed input
    // leaves the persistent DSP state untouched and marks the output invalid.
    bool process(
        const spatial_source_block_view& input,
        double sample_rate,
        block_storage& output) noexcept
    {
        output.reset();

        if (!valid_smoothing(smoothing_) || !std::isfinite(sample_rate) || sample_rate <= 0.0 ||
            input.lane_count > MaxLanes || input.evidence_event_count > MaxEvents ||
            (input.lane_count != 0 && input.lanes == nullptr) ||
            (input.evidence_event_count != 0 && input.evidence_events == nullptr))
            return false;

        const float rise = coefficient(smoothing_.rise_seconds, sample_rate);
        const float fall = coefficient(smoothing_.fall_seconds, sample_rate);
        const float signed_motion = coefficient(smoothing_.signed_motion_seconds, sample_rate);

        auto next = lanes_;
        std::array<std::size_t, MaxLanes> last_offset{};

        for (std::size_t lane_index = 0; lane_index < input.lane_count; ++lane_index) {
            lane_state& lane = next[lane_index];
            const spatial_source_evidence& evidence = input.lanes[lane_index].evidence;
            const spatial_presentation_values target = values_from(evidence.presentation);

            if (!same_identity(lane, evidence)) {
                lane.initialized = true;
                lane.source_id = evidence.source_id;
                lane.generation = evidence.generation;
                lane.smoothed = target;
            }

            lane.evidence = evidence;
            lane.target = target;
        }

        std::size_t previous_event_offset = 0;
        bool have_previous_event = false;
        for (std::size_t event_index = 0; event_index < input.evidence_event_count; ++event_index) {
            const spatial_source_evidence_event& event = input.evidence_events[event_index];
            if (event.lane_index >= input.lane_count || event.frame_offset > input.frame_count ||
                (have_previous_event && event.frame_offset < previous_event_offset))
                return false;

            have_previous_event = true;
            previous_event_offset = event.frame_offset;

            lane_state& lane = next[event.lane_index];
            if (!emit_and_advance(
                    event.lane_index,
                    last_offset[event.lane_index],
                    event.frame_offset,
                    lane,
                    rise,
                    fall,
                    signed_motion,
                    output))
                return false;

            last_offset[event.lane_index] = event.frame_offset;
            const spatial_presentation_values new_target = values_from(event.evidence.presentation);

            // A physical lane may be reused for a new bounded source episode.
            // Do not drag the old source's inferred presentation tendencies
            // across that identity boundary. Persistent-part relationships stay
            // available in evidence for a higher policy to use explicitly.
            if (!same_identity(lane, event.evidence)) {
                lane.initialized = true;
                lane.source_id = event.evidence.source_id;
                lane.generation = event.evidence.generation;
                lane.smoothed = new_target;
            }

            lane.evidence = event.evidence;
            lane.target = new_target;
        }

        for (std::size_t lane_index = 0; lane_index < input.lane_count; ++lane_index) {
            lane_state& lane = next[lane_index];
            if (!emit_and_advance(
                    lane_index,
                    last_offset[lane_index],
                    input.frame_count,
                    lane,
                    rise,
                    fall,
                    signed_motion,
                    output))
                return false;
        }

        // Lanes outside this block are not active transport lanes. Their stale
        // state cannot influence a later source because identity mismatch resets
        // the smoothing state before reuse.
        for (std::size_t lane_index = input.lane_count; lane_index < MaxLanes; ++lane_index)
            next[lane_index].initialized = false;

        lanes_ = next;
        output.mark_valid(input.frame_count);
        return true;
    }

private:
    struct lane_state {
        bool initialized = false;
        std::uint64_t source_id = 0;
        std::uint64_t generation = 0;
        spatial_source_evidence evidence{};
        spatial_presentation_values smoothed{};
        spatial_presentation_values target{};
    };

    static bool valid_smoothing(const realtime_spatial_smoothing& smoothing) noexcept {
        return std::isfinite(smoothing.rise_seconds) && smoothing.rise_seconds >= 0.0f &&
            std::isfinite(smoothing.fall_seconds) && smoothing.fall_seconds >= 0.0f &&
            std::isfinite(smoothing.signed_motion_seconds) && smoothing.signed_motion_seconds >= 0.0f;
    }

    static bool same_identity(const lane_state& lane, const spatial_source_evidence& evidence) noexcept {
        return lane.initialized && lane.source_id == evidence.source_id && lane.generation == evidence.generation;
    }

    static spatial_presentation_values values_from(const spatial_presentation_evidence& evidence) noexcept {
        return {
            clamp_unit_interval(evidence.foundation),
            clamp_unit_interval(evidence.foreground),
            clamp_unit_interval(evidence.diffuse),
            clamp_unit_interval(evidence.width),
            clamp_unit_gain(evidence.vertical_affinity),
            clamp_unit_interval(evidence.confidence),
        };
    }

    static float coefficient(float seconds, double sample_rate) noexcept {
        if (seconds <= 0.0f)
            return 0.0f;
        return static_cast<float>(std::exp(-1.0 / (static_cast<double>(seconds) * sample_rate)));
    }

    static float advance_unit(
        float current,
        float target,
        std::size_t frames,
        float rise,
        float fall) noexcept
    {
        if (frames == 0 || current == target)
            return current;
        const float per_sample = target >= current ? rise : fall;
        const double span_coefficient = std::pow(
            static_cast<double>(per_sample),
            static_cast<double>(frames));
        return target + static_cast<float>((current - target) * span_coefficient);
    }

    static float advance_signed(
        float current,
        float target,
        std::size_t frames,
        float motion) noexcept
    {
        if (frames == 0 || current == target)
            return current;
        const double span_coefficient = std::pow(
            static_cast<double>(motion),
            static_cast<double>(frames));
        return target + static_cast<float>((current - target) * span_coefficient);
    }

    static spatial_presentation_values advance_values(
        const spatial_presentation_values& current,
        const spatial_presentation_values& target,
        std::size_t frames,
        float rise,
        float fall,
        float signed_motion) noexcept
    {
        return {
            advance_unit(current.foundation, target.foundation, frames, rise, fall),
            advance_unit(current.foreground, target.foreground, frames, rise, fall),
            advance_unit(current.diffuse, target.diffuse, frames, rise, fall),
            advance_unit(current.width, target.width, frames, rise, fall),
            advance_signed(current.vertical_affinity, target.vertical_affinity, frames, signed_motion),
            advance_unit(current.confidence, target.confidence, frames, rise, fall),
        };
    }

    static bool emit_and_advance(
        std::size_t lane_index,
        std::size_t begin,
        std::size_t end,
        lane_state& lane,
        float rise,
        float fall,
        float signed_motion,
        block_storage& output) noexcept
    {
        if (end < begin)
            return false;
        if (end == begin)
            return true;

        const realtime_spatial_control_span span{
            lane_index,
            begin,
            end - begin,
            lane.evidence,
            lane.smoothed,
            lane.target,
            rise,
            fall,
            signed_motion,
        };
        if (!output.push(span))
            return false;

        lane.smoothed = advance_values(
            lane.smoothed,
            lane.target,
            end - begin,
            rise,
            fall,
            signed_motion);
        return true;
    }

    realtime_spatial_smoothing smoothing_{};
    std::array<lane_state, MaxLanes> lanes_{};
};

} // namespace vgmtooling::model
