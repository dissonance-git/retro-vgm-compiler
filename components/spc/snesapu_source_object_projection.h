#pragma once

#include "snesapu_source_transport_v2.h"
#include "../../model/spatial_source.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

enum class snesapu_source_object_projection_error : std::uint8_t {
    none = 0,
    invalid_source,
    invalid_segment,
    segment_outside_source,
    nonfinite_source,
    event_capacity_exceeded,
    invalid_event,
};

// Converts the mature SNESAPU SRCE v2 producer contract into the common source
// object contract without turning its sixteen route-control planes into audio.
//
// Dry voice amplitude receives the *sample-exact* magnitude of the producer's
// effective L/R coefficient trajectory. The signed L/R route retained as pose
// evidence is only a short-segment summary; marking gain_preapplied prevents a
// renderer from multiplying that summary into the PCM again.
//
// The two shared wet planes are already post-EVOL contributions and therefore
// travel unchanged as one linked environmental field (two mono side lanes).
template <std::size_t MaxFrames = snesapu_source_transport_v2::max_frames,
          std::size_t MaxEvents = 512>
class snesapu_source_object_projection_storage {
    static_assert(MaxFrames > 0, "MaxFrames must be non-zero");
    static_assert(MaxEvents > 0, "MaxEvents must be non-zero");

public:
    static constexpr std::size_t voice_count = snesapu_source_transport_v2::voice_count;
    static constexpr std::size_t lane_count = voice_count + 2u;

    void reset() noexcept {
        block_ = {};
        event_count_ = 0;
        valid_ = false;
        last_error_ = snesapu_source_object_projection_error::none;
    }

    bool build(
        const snesapu_source_transport_v2::view& source,
        std::size_t source_frame_offset,
        const vgmtooling::model::spatial_source_block_view& evidence_segment,
        std::uint32_t echo_generation) noexcept
    {
        reset();
        if (!source.valid())
            return fail(snesapu_source_object_projection_error::invalid_source);
        if (evidence_segment.lanes == nullptr || evidence_segment.lane_count != voice_count ||
            evidence_segment.frame_count == 0 || evidence_segment.frame_count > MaxFrames ||
            (evidence_segment.evidence_event_count != 0 && evidence_segment.evidence_events == nullptr))
            return fail(snesapu_source_object_projection_error::invalid_segment);
        if (source_frame_offset > source.frame_count() ||
            evidence_segment.frame_count > source.frame_count() - source_frame_offset)
            return fail(snesapu_source_object_projection_error::segment_outside_source);
        if (evidence_segment.evidence_event_count > MaxEvents)
            return fail(snesapu_source_object_projection_error::event_capacity_exceeded);

        const std::size_t frames = evidence_segment.frame_count;
        for (std::size_t voice = 0; voice < voice_count; ++voice) {
            if (evidence_segment.lanes[voice].kind !=
                    vgmtooling::model::spatial_audio_lane_kind::dry_source ||
                evidence_segment.lanes[voice].evidence.family !=
                    vgmtooling::model::spatial_source_family::spc)
                return fail(snesapu_source_object_projection_error::invalid_segment);

            const float* dry = source.dry_voice(voice);
            const float* gain_l = source.gain_left(voice);
            const float* gain_r = source.gain_right(voice);
            if (dry == nullptr || gain_l == nullptr || gain_r == nullptr)
                return fail(snesapu_source_object_projection_error::invalid_source);

            double square_sum_l = 0.0;
            double square_sum_r = 0.0;
            double signed_sum_l = 0.0;
            double signed_sum_r = 0.0;
            for (std::size_t frame = 0; frame < frames; ++frame) {
                const std::size_t source_frame = source_frame_offset + frame;
                const float x = dry[source_frame];
                const float gl = gain_l[source_frame];
                const float gr = gain_r[source_frame];
                if (!std::isfinite(x) || !std::isfinite(gl) || !std::isfinite(gr))
                    return fail(snesapu_source_object_projection_error::nonfinite_source);

                const double dl = static_cast<double>(gl);
                const double dr = static_cast<double>(gr);
                const double energy_gain = std::sqrt((dl * dl + dr * dr) * 0.5);
                const double routed = static_cast<double>(x) * energy_gain;
                if (!std::isfinite(routed))
                    return fail(snesapu_source_object_projection_error::nonfinite_source);
                routed_dry_[voice * MaxFrames + frame] = static_cast<float>(routed);

                square_sum_l += dl * dl;
                square_sum_r += dr * dr;
                signed_sum_l += dl;
                signed_sum_r += dr;
            }

            auto evidence = evidence_segment.lanes[voice].evidence;
            evidence.stereo_route.present = true;
            evidence.stereo_route.left_gain = signed_rms_route(
                square_sum_l,
                signed_sum_l,
                frames);
            evidence.stereo_route.right_gain = signed_rms_route(
                square_sum_r,
                signed_sum_r,
                frames);
            evidence.stereo_route.authority =
                vgmtooling::model::spatial_evidence_authority::device_authored;
            evidence.stereo_route.gain_preapplied = true;

            lanes_[voice] = {};
            lanes_[voice].kind = vgmtooling::model::spatial_audio_lane_kind::dry_source;
            lanes_[voice].mono_pcm = routed_dry_.data() + voice * MaxFrames;
            lanes_[voice].evidence = evidence;
        }

        const float* wet_l = source.echo_left();
        const float* wet_r = source.echo_right();
        if (wet_l == nullptr || wet_r == nullptr)
            return fail(snesapu_source_object_projection_error::invalid_source);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const std::size_t source_frame = source_frame_offset + frame;
            if (!std::isfinite(wet_l[source_frame]) || !std::isfinite(wet_r[source_frame]))
                return fail(snesapu_source_object_projection_error::nonfinite_source);
        }

        lanes_[8] = {};
        lanes_[8].kind = vgmtooling::model::spatial_audio_lane_kind::shared_effect_return;
        lanes_[8].mono_pcm = wet_l + source_frame_offset;
        lanes_[8].evidence = spc_source_bus::make_preapplied_echo_source(
            spc_source_bus::echo_side::left,
            echo_generation,
            127);

        lanes_[9] = {};
        lanes_[9].kind = vgmtooling::model::spatial_audio_lane_kind::shared_effect_return;
        lanes_[9].mono_pcm = wet_r + source_frame_offset;
        lanes_[9].evidence = spc_source_bus::make_preapplied_echo_source(
            spc_source_bus::echo_side::right,
            echo_generation,
            127);

        std::size_t previous_offset = 0;
        bool have_previous = false;
        for (std::size_t index = 0; index < evidence_segment.evidence_event_count; ++index) {
            const auto& incoming = evidence_segment.evidence_events[index];
            if (incoming.lane_index >= voice_count || incoming.frame_offset > frames ||
                (have_previous && incoming.frame_offset < previous_offset))
                return fail(snesapu_source_object_projection_error::invalid_event);

            auto& outgoing = events_[event_count_++];
            outgoing = incoming;
            outgoing.evidence.stereo_route.gain_preapplied = true;

            // For an in-segment event, replace a register/block approximation
            // with the exact route coefficient present at that source frame.
            // A terminal event has no causal sample in this segment, so it keeps
            // its incoming next-state route and is merely marked preapplied.
            if (incoming.frame_offset < frames) {
                const std::size_t source_frame = source_frame_offset + incoming.frame_offset;
                const float gl = source.gain_left(incoming.lane_index)[source_frame];
                const float gr = source.gain_right(incoming.lane_index)[source_frame];
                if (!std::isfinite(gl) || !std::isfinite(gr))
                    return fail(snesapu_source_object_projection_error::nonfinite_source);
                outgoing.evidence.stereo_route.present = true;
                outgoing.evidence.stereo_route.left_gain = gl;
                outgoing.evidence.stereo_route.right_gain = gr;
                outgoing.evidence.stereo_route.authority =
                    vgmtooling::model::spatial_evidence_authority::device_authored;
            }

            previous_offset = incoming.frame_offset;
            have_previous = true;
        }

        block_.lanes = lanes_.data();
        block_.lane_count = lane_count;
        block_.frame_count = frames;
        block_.evidence_events = event_count_ == 0 ? nullptr : events_.data();
        block_.evidence_event_count = event_count_;
        valid_ = true;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    const vgmtooling::model::spatial_source_block_view& block() const noexcept { return block_; }
    snesapu_source_object_projection_error last_error() const noexcept { return last_error_; }

private:
    static float signed_rms_route(
        double square_sum,
        double signed_sum,
        std::size_t frames) noexcept
    {
        if (frames == 0 || square_sum <= 0.0 || !std::isfinite(square_sum) ||
            !std::isfinite(signed_sum))
            return 0.0f;
        const double magnitude = std::sqrt(square_sum / static_cast<double>(frames));
        if (!std::isfinite(magnitude) || std::abs(signed_sum) < 1.0e-12)
            return 0.0f;
        return static_cast<float>(signed_sum < 0.0 ? -magnitude : magnitude);
    }

    bool fail(snesapu_source_object_projection_error error) noexcept {
        last_error_ = error;
        valid_ = false;
        block_ = {};
        event_count_ = 0;
        return false;
    }

    std::array<float, voice_count * MaxFrames> routed_dry_{};
    std::array<vgmtooling::model::spatial_audio_lane_view, lane_count> lanes_{};
    std::array<vgmtooling::model::spatial_source_evidence_event, MaxEvents> events_{};
    std::size_t event_count_ = 0;
    vgmtooling::model::spatial_source_block_view block_{};
    bool valid_ = false;
    snesapu_source_object_projection_error last_error_ =
        snesapu_source_object_projection_error::none;
};

} // namespace gameaudio::spc
