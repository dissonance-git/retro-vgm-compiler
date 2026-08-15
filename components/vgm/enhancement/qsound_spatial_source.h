#pragma once

#include "../../../model/spatial_source.h"

#include <array>
#include <cstdint>

namespace gameaudio::vgm {

enum class qsound_source_kind : std::uint8_t {
    pcm = 0,
    adpcm = 1,
};

enum class qsound_pan_mode : std::uint8_t {
    unknown = 0,
    spatial = 1,
    linear = 2,
};

struct qsound_four_way_route {
    std::uint16_t raw_pan = 0;
    qsound_pan_mode mode = qsound_pan_mode::unknown;
    bool decoded = false;
    float dry_left = 0.0f;
    float dry_right = 0.0f;
    float wet_left = 0.0f;
    float wet_right = 0.0f;
};

struct qsound_spatial_source {
    vgmtooling::model::spatial_source_evidence evidence{};
    qsound_four_way_route route{};

    // QSound PCM channels have a separate signed contribution to the shared
    // echo input. ADPCM channels do not have the corresponding register in the
    // decoded DL-1425 register map. Preserve the raw value rather than reducing
    // it to the generic boolean send flag.
    bool echo_contribution_known = false;
    std::int16_t echo_contribution_raw = 0;
};

constexpr std::size_t qsound_pcm_source_count = 16;
constexpr std::size_t qsound_adpcm_source_count = 3;
constexpr std::size_t qsound_source_count = qsound_pcm_source_count + qsound_adpcm_source_count;

// These tables are the signed coefficients recovered from the DL-1425 DSP
// program by the superctr QSound HLE used by libvgm/MAME. The mixer subtracts
// these values, so the effective source coefficient is -table/16384.
constexpr std::array<std::int16_t, 33> qsound_dry_mix_table = {
    -16384,-16384,-16384,-16384,-16384,-16384,-16384,-16384,
    -16384,-16384,-16384,-16384,-16384,-16384,-16384,-16384,
    -16384,-14746,-13107,-11633,-10486,-9175,-8520,-7209,
    -6226,-5226,-4588,-3768,-3277,-2703,-2130,-1802,0
};

constexpr std::array<std::int16_t, 33> qsound_wet_mix_table = {
    0,-1638,-1966,-2458,-2949,-3441,-4096,-4669,
    -4915,-5120,-5489,-6144,-7537,-8831,-9339,-9830,
    -10240,-10322,-10486,-10568,-10650,-11796,-12288,-12288,
    -12534,-12648,-12780,-12829,-12943,-13107,-13418,-14090,-16384
};

constexpr std::array<std::int16_t, 33> qsound_linear_mix_table = {
    -16379,-16338,-16257,-16135,-15973,-15772,-15531,-15251,
    -14934,-14580,-14189,-13763,-13303,-12810,-12284,-11729,
    -11729,-11144,-10531,-9893,-9229,-8543,-7836,-7109,
    -6364,-5604,-4829,-4043,-3246,-2442,-1631,-817,0
};

constexpr float qsound_effective_mix_gain(std::int16_t table_value) noexcept {
    return -static_cast<float>(table_value) / 16384.0f;
}

constexpr qsound_four_way_route qsound_decode_four_way_route(std::uint16_t raw_pan) noexcept {
    qsound_four_way_route out;
    out.raw_pan = raw_pan;

    // Normal QSound spatial table: DSP ROM 0x110..0x130.
    if (raw_pan >= 0x110u && raw_pan <= 0x130u) {
        const std::size_t i = static_cast<std::size_t>(raw_pan - 0x110u);
        out.mode = qsound_pan_mode::spatial;
        out.decoded = true;
        out.dry_left = qsound_effective_mix_gain(qsound_dry_mix_table[i]);
        out.dry_right = qsound_effective_mix_gain(qsound_dry_mix_table[32u - i]);
        out.wet_left = qsound_effective_mix_gain(qsound_wet_mix_table[i]);
        out.wet_right = qsound_effective_mix_gain(qsound_wet_mix_table[32u - i]);
        return out;
    }

    // Alternate DSP region described by the recovered program as linear
    // panning: 0x140..0x160. It contributes only to the dry path.
    if (raw_pan >= 0x140u && raw_pan <= 0x160u) {
        const std::size_t i = static_cast<std::size_t>(raw_pan - 0x140u);
        out.mode = qsound_pan_mode::linear;
        out.decoded = true;
        out.dry_left = qsound_effective_mix_gain(qsound_linear_mix_table[i]);
        out.dry_right = qsound_effective_mix_gain(qsound_linear_mix_table[32u - i]);
        return out;
    }

    // Preserve the raw word but do not imitate libvgm's defensive clamping for
    // unknown addresses. MAME LLE executes the real DSP ROM, so the honest
    // cross-implementation boundary is to leave these coefficients undecoded.
    return out;
}

constexpr bool qsound_valid_source_slot(qsound_source_kind kind, std::uint8_t slot) noexcept {
    return kind == qsound_source_kind::pcm
        ? slot < qsound_pcm_source_count
        : slot < qsound_adpcm_source_count;
}

constexpr std::uint32_t qsound_physical_slot(qsound_source_kind kind, std::uint8_t slot) noexcept {
    return kind == qsound_source_kind::pcm
        ? static_cast<std::uint32_t>(slot)
        : static_cast<std::uint32_t>(qsound_pcm_source_count + slot);
}

constexpr std::uint64_t qsound_source_id(
    qsound_source_kind kind,
    std::uint8_t instance,
    std::uint8_t slot,
    std::uint32_t episode_generation) noexcept {
    // 0x51 ('Q') is a local QSound namespace marker. This remains an
    // implementation-source identity, not a persistent musical-part identity.
    return (0x51ull << 56u)
        | (static_cast<std::uint64_t>(instance) << 48u)
        | (static_cast<std::uint64_t>(kind) << 47u)
        | (static_cast<std::uint64_t>(slot) << 40u)
        | static_cast<std::uint64_t>(episode_generation);
}

constexpr qsound_spatial_source make_qsound_spatial_source(
    qsound_source_kind kind,
    std::uint8_t instance,
    std::uint8_t slot,
    std::uint32_t episode_generation,
    std::uint16_t raw_pan,
    std::int16_t pcm_echo_contribution = 0) noexcept {
    qsound_spatial_source out;
    out.route = qsound_decode_four_way_route(raw_pan);

    auto& source = out.evidence;
    source.source_id = qsound_source_id(kind, instance, slot, episode_generation);
    source.generation = episode_generation;
    source.family = vgmtooling::model::spatial_source_family::vgm;
    source.physical_slot_present = qsound_valid_source_slot(kind, slot);
    if (source.physical_slot_present)
        source.physical_slot = qsound_physical_slot(kind, slot);

    if (out.route.decoded) {
        source.stereo_route.present = true;
        source.stereo_route.left_gain = vgmtooling::model::clamp_unit_gain(out.route.dry_left);
        source.stereo_route.right_gain = vgmtooling::model::clamp_unit_gain(out.route.dry_right);
        source.stereo_route.authority = vgmtooling::model::spatial_evidence_authority::device_authored;
    }

    if (kind == qsound_source_kind::pcm && source.physical_slot_present) {
        out.echo_contribution_known = true;
        out.echo_contribution_raw = pcm_echo_contribution;
        source.effect_send_known = true;
        source.effect_send_enabled = pcm_echo_contribution != 0;
    }

    return out;
}

} // namespace gameaudio::vgm
