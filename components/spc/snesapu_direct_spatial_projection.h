#pragma once

#include "snesapu_source_transport_v2.h"
#include "spc_spatial_source.h"
#include "spc_source_bus.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

// Conservative direct bridge for the editable SNESAPU child transport. Physical
// S-DSP voices are the only initial identity claim. The shared realtime musical
// frontend learns role hypotheses from completed PCM for later blocks; this
// layer does not resurrect the historical block-end semantic classifier.
//
// Because this bridge owns the complete SRCE gain trajectories, it can preserve
// route changes sample-for-sample. Gain magnitude is already applied to the
// routed dry PCM; timed route events carry only authored pose/polarity evidence
// and are always marked gain_preapplied.
template <std::size_t MaxFrames = snesapu_source_transport_v2::max_frames>
class snesapu_direct_spatial_projection_storage {
public:
    using transport = snesapu_source_transport_v2;
    static constexpr std::size_t voice_count = transport::voice_count;
    static constexpr std::size_t lane_count = voice_count + 2u;
    static constexpr std::size_t max_route_events = voice_count * (MaxFrames - 1u);

    static_assert(MaxFrames > 0, "direct SPC projection requires frames");

    void reset(std::uint32_t session_generation = 1u) noexcept {
        block_ = {};
        event_count_ = 0;
        generation_ = session_generation == 0u ? 1u : session_generation;
        valid_ = false;
    }

    bool build(const transport::view& source) noexcept {
        block_ = {};
        event_count_ = 0;
        valid_ = false;
        if (!source.valid() || source.frame_count() == 0 || source.frame_count() > MaxFrames)
            return false;

        const std::size_t frames = source.frame_count();
        for (std::size_t voice = 0; voice < voice_count; ++voice) {
            const float* dry = source.dry_voice(voice);
            const float* gain_l = source.gain_left(voice);
            const float* gain_r = source.gain_right(voice);
            if (dry == nullptr || gain_l == nullptr || gain_r == nullptr)
                return false;

            for (std::size_t frame = 0; frame < frames; ++frame) {
                const float x = dry[frame];
                const float gl = gain_l[frame];
                const float gr = gain_r[frame];
                if (!std::isfinite(x) || !std::isfinite(gl) || !std::isfinite(gr))
                    return false;
                const double dl = static_cast<double>(gl);
                const double dr = static_cast<double>(gr);
                const double energy_gain = std::sqrt((dl * dl + dr * dr) * 0.5);
                const double routed = static_cast<double>(x) * energy_gain;
                if (!std::isfinite(routed))
                    return false;
                routed_dry_[voice * MaxFrames + frame] = static_cast<float>(routed);
            }

            lanes_[voice] = {};
            lanes_[voice].kind = vgmtooling::model::spatial_audio_lane_kind::dry_source;
            lanes_[voice].mono_pcm = routed_dry_.data() + voice * MaxFrames;
            auto& initial = lanes_[voice].evidence;
            initial.family = vgmtooling::model::spatial_source_family::spc;
            initial.source_id = spc_spatial_source_id(
                static_cast<std::uint8_t>(voice), generation_);
            initial.generation = generation_;
            initial.physical_slot_present = true;
            initial.physical_slot = static_cast<std::uint32_t>(voice);
            set_exact_route(initial, gain_l[0], gain_r[0]);
        }

        // Global event ordering is frame-major, then physical voice. Emit only
        // actual coefficient changes; constant authored routing costs nothing.
        for (std::size_t frame = 1; frame < frames; ++frame) {
            for (std::size_t voice = 0; voice < voice_count; ++voice) {
                const float* gain_l = source.gain_left(voice);
                const float* gain_r = source.gain_right(voice);
                const float gl = gain_l[frame];
                const float gr = gain_r[frame];
                if (!std::isfinite(gl) || !std::isfinite(gr))
                    return false;
                if (gl == gain_l[frame - 1] && gr == gain_r[frame - 1])
                    continue;
                if (event_count_ >= events_.size())
                    return false;

                auto evidence = lanes_[voice].evidence;
                set_exact_route(evidence, gl, gr);
                events_[event_count_++] = {frame, voice, evidence};
            }
        }

        const float* wet_l = source.echo_left();
        const float* wet_r = source.echo_right();
        if (wet_l == nullptr || wet_r == nullptr)
            return false;
        for (std::size_t frame = 0; frame < frames; ++frame) {
            if (!std::isfinite(wet_l[frame]) || !std::isfinite(wet_r[frame]))
                return false;
        }

        lanes_[8] = {};
        lanes_[8].kind = vgmtooling::model::spatial_audio_lane_kind::shared_effect_return;
        lanes_[8].mono_pcm = wet_l;
        lanes_[8].evidence = spc_source_bus::make_post_evol_echo_source(
            spc_source_bus::echo_side::left,
            generation_);

        lanes_[9] = {};
        lanes_[9].kind = vgmtooling::model::spatial_audio_lane_kind::shared_effect_return;
        lanes_[9].mono_pcm = wet_r;
        lanes_[9].evidence = spc_source_bus::make_post_evol_echo_source(
            spc_source_bus::echo_side::right,
            generation_);

        block_.lanes = lanes_.data();
        block_.lane_count = lane_count;
        block_.frame_count = frames;
        block_.evidence_events = event_count_ == 0 ? nullptr : events_.data();
        block_.evidence_event_count = event_count_;
        valid_ = true;
        return true;
    }

    bool valid() const noexcept { return valid_; }
    const vgmtooling::model::spatial_source_block_view& block() const noexcept {
        return block_;
    }
    std::size_t event_count() const noexcept { return valid_ ? event_count_ : 0u; }
    std::uint32_t generation() const noexcept { return generation_; }

private:
    static void set_exact_route(
        vgmtooling::model::spatial_source_evidence& evidence,
        float left,
        float right) noexcept
    {
        evidence.stereo_route.present = true;
        evidence.stereo_route.left_gain = left;
        evidence.stereo_route.right_gain = right;
        evidence.stereo_route.authority =
            vgmtooling::model::spatial_evidence_authority::device_authored;
        evidence.stereo_route.gain_preapplied = true;
    }

    std::array<float, voice_count * MaxFrames> routed_dry_{};
    std::array<vgmtooling::model::spatial_audio_lane_view, lane_count> lanes_{};
    std::array<vgmtooling::model::spatial_source_evidence_event, max_route_events> events_{};
    std::size_t event_count_ = 0;
    vgmtooling::model::spatial_source_block_view block_{};
    std::uint32_t generation_ = 1u;
    bool valid_ = false;
};

} // namespace gameaudio::spc
