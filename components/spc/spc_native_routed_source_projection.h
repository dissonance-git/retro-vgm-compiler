#pragma once

#include "../../model/spatial_source.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

enum class spc_native_routed_source_projection_error : std::uint8_t {
    none = 0,
    invalid_segment,
    missing_source_pcm,
    missing_exact_route,
    invalid_event,
    nonfinite_source,
    capacity_exceeded,
};

// Energy-preserving mono projection for the native 32 kHz S-DSP dry tap.
//
// The accurate DSP observer exposes one mono source per voice before signed
// VOLL/VOLR. The common source renderer must still retain that signed route as
// authored evidence, but the acoustic governor should measure the energy the
// route can actually contribute rather than treating a zero-routed voice as an
// audible competitor. This projection applies only the exact route magnitude:
//
//   mono *= sqrt((L^2 + R^2) / 2)
//
// Polarity and side remain in stereo_route. `gain_preapplied` is set so neither
// Omniphony nor another downstream consumer multiplies the route amplitude a
// second time. This is the same energy-equivalent projection already used by
// the SNESAPU SRCE-v2 source-object path.
template <std::size_t MaxFrames = 4096, std::size_t MaxEvents = 512>
class spc_native_routed_source_projection_storage {
public:
    static constexpr std::size_t voice_count = 8;

    void reset() noexcept {
        block_ = {};
        event_count_ = 0;
        valid_ = false;
        last_error_ = spc_native_routed_source_projection_error::none;
    }

    bool build(
        const vgmtooling::model::spatial_source_block_view& evidence_segment,
        const std::array<const float*, voice_count>& dry_voice) noexcept
    {
        using namespace vgmtooling::model;
        reset();
        if (evidence_segment.lanes == nullptr ||
            evidence_segment.lane_count != voice_count ||
            evidence_segment.frame_count == 0)
            return fail(spc_native_routed_source_projection_error::invalid_segment);
        if (evidence_segment.frame_count > MaxFrames ||
            evidence_segment.evidence_event_count > MaxEvents)
            return fail(spc_native_routed_source_projection_error::capacity_exceeded);
        if (evidence_segment.evidence_event_count != 0 &&
            evidence_segment.evidence_events == nullptr)
            return fail(spc_native_routed_source_projection_error::invalid_event);

        std::array<float, voice_count> route_left{};
        std::array<float, voice_count> route_right{};
        for (std::size_t voice = 0; voice < voice_count; ++voice) {
            const auto& incoming = evidence_segment.lanes[voice];
            if (incoming.kind != spatial_audio_lane_kind::dry_source)
                return fail(spc_native_routed_source_projection_error::invalid_segment);
            if (dry_voice[voice] == nullptr)
                return fail(spc_native_routed_source_projection_error::missing_source_pcm);
            if (!incoming.evidence.stereo_route.present)
                return fail(spc_native_routed_source_projection_error::missing_exact_route);
            if (!finite_route(incoming.evidence.stereo_route.left_gain) ||
                !finite_route(incoming.evidence.stereo_route.right_gain))
                return fail(spc_native_routed_source_projection_error::nonfinite_source);

            route_left[voice] = incoming.evidence.stereo_route.left_gain;
            route_right[voice] = incoming.evidence.stereo_route.right_gain;
            lanes_[voice] = incoming;
            lanes_[voice].mono_pcm = routed_[voice].data();
            lanes_[voice].evidence.stereo_route.gain_preapplied = true;
        }

        std::size_t previous_offset = 0;
        bool have_previous = false;
        for (std::size_t index = 0; index < evidence_segment.evidence_event_count; ++index) {
            const auto& incoming = evidence_segment.evidence_events[index];
            if (incoming.lane_index >= voice_count ||
                incoming.frame_offset > evidence_segment.frame_count ||
                (have_previous && incoming.frame_offset < previous_offset))
                return fail(spc_native_routed_source_projection_error::invalid_event);
            if (!incoming.evidence.stereo_route.present ||
                !finite_route(incoming.evidence.stereo_route.left_gain) ||
                !finite_route(incoming.evidence.stereo_route.right_gain))
                return fail(spc_native_routed_source_projection_error::missing_exact_route);

            auto& outgoing = events_[event_count_++];
            outgoing = incoming;
            outgoing.evidence.stereo_route.gain_preapplied = true;
            previous_offset = incoming.frame_offset;
            have_previous = true;
        }

        std::size_t event_index = 0;
        for (std::size_t frame = 0; frame < evidence_segment.frame_count; ++frame) {
            while (event_index < evidence_segment.evidence_event_count &&
                   evidence_segment.evidence_events[event_index].frame_offset == frame) {
                const auto& event = evidence_segment.evidence_events[event_index];
                route_left[event.lane_index] = event.evidence.stereo_route.left_gain;
                route_right[event.lane_index] = event.evidence.stereo_route.right_gain;
                ++event_index;
            }

            for (std::size_t voice = 0; voice < voice_count; ++voice) {
                const float source = dry_voice[voice][frame];
                if (!std::isfinite(source))
                    return fail(spc_native_routed_source_projection_error::nonfinite_source);
                const double left = static_cast<double>(route_left[voice]);
                const double right = static_cast<double>(route_right[voice]);
                const double gain = std::sqrt((left * left + right * right) * 0.5);
                const double routed = static_cast<double>(source) * gain;
                if (!std::isfinite(routed))
                    return fail(spc_native_routed_source_projection_error::nonfinite_source);
                routed_[voice][frame] = static_cast<float>(routed);
            }
        }

        // Terminal events update the next segment/window state but own no sample
        // in this segment. They are copied above and intentionally do not alter
        // routed PCM here.
        block_ = evidence_segment;
        block_.lanes = lanes_.data();
        block_.evidence_events = event_count_ == 0 ? nullptr : events_.data();
        block_.evidence_event_count = event_count_;
        valid_ = true;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    const vgmtooling::model::spatial_source_block_view& block() const noexcept {
        return block_;
    }
    spc_native_routed_source_projection_error last_error() const noexcept {
        return last_error_;
    }

private:
    static bool finite_route(float value) noexcept {
        return std::isfinite(value) && value >= -1.0f && value <= 1.0f;
    }

    bool fail(spc_native_routed_source_projection_error error) noexcept {
        last_error_ = error;
        valid_ = false;
        block_ = {};
        event_count_ = 0;
        return false;
    }

    std::array<std::array<float, MaxFrames>, voice_count> routed_{};
    std::array<vgmtooling::model::spatial_audio_lane_view, voice_count> lanes_{};
    std::array<vgmtooling::model::spatial_source_evidence_event, MaxEvents> events_{};
    std::size_t event_count_ = 0;
    vgmtooling::model::spatial_source_block_view block_{};
    bool valid_ = false;
    spc_native_routed_source_projection_error last_error_ =
        spc_native_routed_source_projection_error::none;
};

} // namespace gameaudio::spc
