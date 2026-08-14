#pragma once

#include "spc_runtime_capture.h"
#include "../../model/spatial_source.h"

#include <cstdint>

namespace gameaudio::spc {

constexpr float normalize_sdsp_route_gain(std::int8_t gain) noexcept {
    // S-DSP per-voice volumes are signed 8-bit values. Preserve polarity as
    // source truth; this is not a conventional pan law. Map the asymmetric
    // integer range into the closed normalized interval without discarding -128.
    return gain == -128 ? -1.0f : static_cast<float>(gain) / 127.0f;
}

constexpr std::uint64_t spc_spatial_source_id(
    std::uint8_t voice,
    std::uint32_t episode_generation) noexcept {
    return (static_cast<std::uint64_t>(voice) << 56u)
        | static_cast<std::uint64_t>(episode_generation);
}

inline vgmtooling::model::spatial_source_evidence make_spc_spatial_source(
    const spc_runtime_capture_record& record,
    std::uint32_t episode_generation) noexcept {
    vgmtooling::model::spatial_source_evidence source;
    source.source_id = spc_spatial_source_id(record.voice, episode_generation);
    source.generation = episode_generation;
    source.family = vgmtooling::model::spatial_source_family::spc;

    if (has_field(record.fields, spc_runtime_capture_field::voice)) {
        source.physical_slot_present = true;
        source.physical_slot = record.voice;
    }

    if (has_field(record.fields, spc_runtime_capture_field::route_gain_left)
        && has_field(record.fields, spc_runtime_capture_field::route_gain_right)) {
        source.stereo_route.present = true;
        source.stereo_route.left_gain = normalize_sdsp_route_gain(record.route_gain_left);
        source.stereo_route.right_gain = normalize_sdsp_route_gain(record.route_gain_right);
        source.stereo_route.authority =
            vgmtooling::model::spatial_evidence_authority::device_authored;
    }

    if (has_field(record.fields, spc_runtime_capture_field::echo_send_enabled)) {
        source.effect_send_known = true;
        source.effect_send_enabled = record.echo_send_enabled;
    }

    return source;
}

} // namespace gameaudio::spc
