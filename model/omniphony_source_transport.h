#pragma once

#include "realtime_spatial_mix_budget.h"
#include "spatial_source.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace vgmtooling::model {

// Binary transport mirror for Omniphony source_ffi ABI 0.4. These records are
// intentionally dumb C-layout values: Retro VGM Compiler owns musical/source
// semantics and the causal intervention budget; Omniphony owns presentation and
// binaural DSP. Keeping the mirror here avoids a Rust-header dependency.
constexpr std::uint32_t omniphony_source_abi_major_required = 0;
constexpr std::uint32_t omniphony_source_abi_minor_required = 4;

constexpr std::uint32_t omniphony_source_flag_persistent_part = 1u << 0u;
constexpr std::uint32_t omniphony_source_flag_native_stereo_route = 1u << 1u;
constexpr std::uint32_t omniphony_source_flag_authored_position = 1u << 2u;
constexpr std::uint32_t omniphony_source_flag_route_gain_preapplied = 1u << 3u;

constexpr std::uint32_t omniphony_source_lane_dry = 0;
constexpr std::uint32_t omniphony_source_lane_shared_wet = 1;
constexpr std::uint32_t omniphony_source_lane_reference_mix = 2;

struct omniphony_source_mix_budget_v1_transport {
    float depth_scale = 1.0f;
    float height_scale = 1.0f;
    float shared_wet_strength_scale = 1.0f;
    float shared_wet_extent_scale = 1.0f;
    float externalization_scale = 1.0f;
};

struct omniphony_source_evidence_v1_transport {
    std::uint32_t lane_kind = 0;
    std::uint32_t flags = 0;
    std::uint64_t source_id = 0;
    std::uint64_t persistent_part_id = 0;
    float left_gain = 0.0f;
    float right_gain = 0.0f;
    float authored_x = 0.0f;
    float authored_y = 0.0f;
    float authored_z = 0.0f;
    float foundation = 0.0f;
    float foreground = 0.0f;
    float diffuse = 0.0f;
    float width = 0.0f;
    float vertical_affinity = 0.0f;
    float confidence = 0.0f;
};

struct omniphony_source_evidence_event_v1_transport {
    std::uint32_t frame_offset = 0;
    std::uint32_t lane_index = 0;
    omniphony_source_evidence_v1_transport evidence{};
};

static_assert(std::is_standard_layout_v<omniphony_source_mix_budget_v1_transport>);
static_assert(sizeof(omniphony_source_mix_budget_v1_transport) == 20u);
static_assert(std::is_standard_layout_v<omniphony_source_evidence_v1_transport>);
static_assert(std::is_standard_layout_v<omniphony_source_evidence_event_v1_transport>);

// Dynamic hosts can resolve the ABI functions without importing Omniphony's
// implementation headers into the compiler model layer. The pointee is opaque,
// matching OmniphonySourceProcessor.
using omniphony_source_processor_handle = void;
using omniphony_source_abi_version_fn = std::uint32_t (*)();
using omniphony_source_set_mix_budget_fn = std::int32_t (*)(
    omniphony_source_processor_handle*,
    const omniphony_source_mix_budget_v1_transport*);
using omniphony_source_process_events_f32_fn = std::int32_t (*)(
    omniphony_source_processor_handle*,
    const float*,
    const omniphony_source_evidence_v1_transport*,
    std::size_t,
    const omniphony_source_evidence_event_v1_transport*,
    std::size_t,
    std::size_t,
    std::uint64_t,
    std::uint32_t,
    float*);

inline omniphony_source_mix_budget_v1_transport make_omniphony_source_mix_budget(
    const realtime_spatial_mix_budget& budget) noexcept
{
    return {
        budget.depth_scale,
        budget.height_scale,
        budget.shared_wet_strength,
        budget.shared_wet_extent,
        budget.added_externalization_scale,
    };
}

constexpr std::uint32_t omniphony_transport_lane_kind(
    spatial_audio_lane_kind kind) noexcept
{
    switch (kind) {
    case spatial_audio_lane_kind::dry_source:
        return omniphony_source_lane_dry;
    case spatial_audio_lane_kind::shared_effect_return:
        return omniphony_source_lane_shared_wet;
    case spatial_audio_lane_kind::reference_mix:
    default:
        return omniphony_source_lane_reference_mix;
    }
}

// Omniphony ABI carries one u64 runtime source token while Retro VGM Compiler
// keeps source_id and generation separately. This mixed token is presentation-
// only: it must never be used as a provenance identity or mapped back into the
// compiler. A generation change deliberately changes the renderer token.
constexpr std::uint64_t omniphony_transport_source_episode_id(
    std::uint64_t source_id,
    std::uint64_t generation) noexcept
{
    if (generation == 0)
        return source_id;

    std::uint64_t z = generation + 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30u)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27u)) * 0x94D049BB133111EBull;
    z ^= z >> 31u;
    return source_id ^ z;
}

inline bool make_omniphony_source_evidence(
    spatial_audio_lane_kind lane_kind,
    const spatial_source_evidence& source,
    bool force_route_gain_preapplied,
    omniphony_source_evidence_v1_transport& out,
    float persistent_part_min_confidence = 0.75f) noexcept
{
    if (!spatial_audio_lane_is_object_renderable(lane_kind))
        return false;

    out = {};
    out.lane_kind = omniphony_transport_lane_kind(lane_kind);
    out.source_id = omniphony_transport_source_episode_id(
        source.source_id,
        source.generation);

    if (source.persistent_part_present && source.persistent_part_id != 0 &&
        source.persistent_part_confidence >= persistent_part_min_confidence) {
        out.flags |= omniphony_source_flag_persistent_part;
        out.persistent_part_id = source.persistent_part_id;
    }

    if (source.stereo_route.present) {
        out.flags |= omniphony_source_flag_native_stereo_route;
        out.left_gain = source.stereo_route.left_gain;
        out.right_gain = source.stereo_route.right_gain;
    }

    if (source.authored_position_present) {
        out.flags |= omniphony_source_flag_authored_position;
        out.authored_x = source.authored_position[0];
        out.authored_y = source.authored_position[1];
        out.authored_z = source.authored_position[2];
    }

    // The source model is authoritative for this arithmetic fact, including
    // timed changes. The explicit boolean remains as a host-side override for
    // producers that already applied the exact route trajectory.
    if (source.stereo_route.gain_preapplied || force_route_gain_preapplied)
        out.flags |= omniphony_source_flag_route_gain_preapplied;

    out.foundation = clamp_unit_interval(source.presentation.foundation);
    out.foreground = clamp_unit_interval(source.presentation.foreground);
    out.diffuse = clamp_unit_interval(source.presentation.diffuse);
    out.width = clamp_unit_interval(source.presentation.width);
    out.vertical_affinity = clamp_unit_gain(source.presentation.vertical_affinity);
    out.confidence = clamp_unit_interval(source.presentation.confidence);
    return true;
}

template <std::size_t MaxLanes = 64, std::size_t MaxEvents = 256>
class omniphony_source_transport_storage {
public:
    void reset() noexcept {
        lane_count_ = 0;
        event_count_ = 0;
        frame_count_ = 0;
        valid_ = false;
    }

    bool build(
        const spatial_source_block_view& block,
        const std::uint8_t* force_route_gain_preapplied = nullptr,
        float persistent_part_min_confidence = 0.75f) noexcept
    {
        reset();
        if (block.lane_count > MaxLanes || block.evidence_event_count > MaxEvents ||
            (block.lane_count != 0 && block.lanes == nullptr) ||
            (block.evidence_event_count != 0 && block.evidence_events == nullptr))
            return false;

        std::size_t previous_offset = 0;
        bool have_previous = false;
        for (std::size_t lane_index = 0; lane_index < block.lane_count; ++lane_index) {
            const spatial_audio_lane_view& lane = block.lanes[lane_index];
            if (!spatial_audio_lane_is_object_renderable(lane.kind) || lane.mono_pcm == nullptr)
                return false;
            if (!make_omniphony_source_evidence(
                    lane.kind,
                    lane.evidence,
                    force_route_gain_preapplied != nullptr &&
                        force_route_gain_preapplied[lane_index] != 0,
                    lanes_[lane_index],
                    persistent_part_min_confidence))
                return false;
        }

        // Validate and materialize the complete event sequence before any PCM is
        // exposed to the renderer. Malformed timed evidence fails the block
        // transactionally before presentation state can advance.
        for (std::size_t event_index = 0; event_index < block.evidence_event_count; ++event_index) {
            const spatial_source_evidence_event& event = block.evidence_events[event_index];
            if (event.lane_index >= block.lane_count || event.frame_offset > block.frame_count ||
                event.frame_offset > static_cast<std::size_t>(UINT32_MAX) ||
                event.lane_index > static_cast<std::size_t>(UINT32_MAX) ||
                (have_previous && event.frame_offset < previous_offset))
                return false;

            previous_offset = event.frame_offset;
            have_previous = true;
            omniphony_source_evidence_event_v1_transport& destination = events_[event_index];
            destination.frame_offset = static_cast<std::uint32_t>(event.frame_offset);
            destination.lane_index = static_cast<std::uint32_t>(event.lane_index);
            if (!make_omniphony_source_evidence(
                    block.lanes[event.lane_index].kind,
                    event.evidence,
                    force_route_gain_preapplied != nullptr &&
                        force_route_gain_preapplied[event.lane_index] != 0,
                    destination.evidence,
                    persistent_part_min_confidence))
                return false;
        }

        lane_count_ = block.lane_count;
        event_count_ = block.evidence_event_count;
        frame_count_ = block.frame_count;
        valid_ = true;
        return true;
    }

    // Interleave the current planar source lanes into caller-owned scratch. A
    // missing source frame is not silently rewritten as digital zero: absence of
    // observed causal PCM invalidates the transport block instead.
    bool interleave_pcm(
        const spatial_source_block_view& block,
        float* output,
        std::size_t output_sample_capacity) const noexcept
    {
        if (!valid_ || block.lane_count != lane_count_ || block.frame_count != frame_count_ ||
            (frame_count_ != 0 && output == nullptr))
            return false;
        if (lane_count_ != 0 && frame_count_ > output_sample_capacity / lane_count_)
            return false;

        for (std::size_t frame = 0; frame < frame_count_; ++frame) {
            for (std::size_t lane_index = 0; lane_index < lane_count_; ++lane_index) {
                const spatial_audio_lane_view& lane = block.lanes[lane_index];
                if (lane.mono_pcm == nullptr ||
                    (lane.availability != nullptr && lane.availability[frame] == 0))
                    return false;
                output[frame * lane_count_ + lane_index] = lane.mono_pcm[frame];
            }
        }
        return true;
    }

    bool valid() const noexcept { return valid_; }
    std::size_t lane_count() const noexcept { return lane_count_; }
    std::size_t event_count() const noexcept { return event_count_; }
    std::size_t frame_count() const noexcept { return frame_count_; }
    const omniphony_source_evidence_v1_transport* lanes() const noexcept {
        return lanes_.data();
    }
    const omniphony_source_evidence_event_v1_transport* events() const noexcept {
        return events_.data();
    }

private:
    std::array<omniphony_source_evidence_v1_transport, MaxLanes> lanes_{};
    std::array<omniphony_source_evidence_event_v1_transport, MaxEvents> events_{};
    std::size_t lane_count_ = 0;
    std::size_t event_count_ = 0;
    std::size_t frame_count_ = 0;
    bool valid_ = false;
};

} // namespace vgmtooling::model
