#include "../../components/vgm/enhancement/qsound_spatial_source.h"

#include <cstdint>

using namespace gameaudio::vgm;
using vgmtooling::model::spatial_evidence_authority;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

constexpr bool near(float a, float b, float epsilon = 0.0001f) noexcept {
    return (a > b ? a - b : b - a) <= epsilon;
}

int main() {
    static_assert(qsound_pcm_source_count == 16);
    static_assert(qsound_adpcm_source_count == 3);
    static_assert(qsound_source_count == 19);

    // Normal QSound pan region: at the left edge, the direct component is
    // left-only while the filtered/wet component is routed to the other side.
    const auto hard_left = qsound_decode_four_way_route(0x110);
    CHECK(hard_left.decoded);
    CHECK(hard_left.mode == qsound_pan_mode::spatial);
    CHECK(near(hard_left.dry_left, 1.0f));
    CHECK(near(hard_left.dry_right, 0.0f));
    CHECK(near(hard_left.wet_left, 0.0f));
    CHECK(near(hard_left.wet_right, 1.0f));

    const auto center = qsound_decode_four_way_route(0x120);
    CHECK(center.decoded);
    CHECK(near(center.dry_left, 1.0f));
    CHECK(near(center.dry_right, 1.0f));
    CHECK(near(center.wet_left, 0.625f));
    CHECK(near(center.wet_right, 0.625f));

    const auto hard_right = qsound_decode_four_way_route(0x130);
    CHECK(hard_right.decoded);
    CHECK(near(hard_right.dry_left, 0.0f));
    CHECK(near(hard_right.dry_right, 1.0f));
    CHECK(near(hard_right.wet_left, 1.0f));
    CHECK(near(hard_right.wet_right, 0.0f));

    // The alternate 0x140..0x160 region is the recovered DSP's dry-only
    // panning table. It must never invent a wet contribution.
    const auto linear_left = qsound_decode_four_way_route(0x140);
    CHECK(linear_left.decoded);
    CHECK(linear_left.mode == qsound_pan_mode::linear);
    CHECK(linear_left.dry_left > 0.99f);
    CHECK(near(linear_left.dry_right, 0.0f));
    CHECK(near(linear_left.wet_left, 0.0f));
    CHECK(near(linear_left.wet_right, 0.0f));

    const auto linear_center = qsound_decode_four_way_route(0x150);
    CHECK(linear_center.decoded);
    CHECK(near(linear_center.dry_left, linear_center.dry_right));
    CHECK(linear_center.dry_left > 0.71f && linear_center.dry_left < 0.72f);
    CHECK(near(linear_center.wet_left, 0.0f));
    CHECK(near(linear_center.wet_right, 0.0f));

    // Do not copy libvgm's defensive pan-index clamp into source truth. Unknown
    // raw DSP addresses stay raw and must not acquire an inferred stereo route.
    const auto gap = qsound_decode_four_way_route(0x131);
    CHECK(!gap.decoded);
    CHECK(gap.mode == qsound_pan_mode::unknown);
    CHECK(gap.raw_pan == 0x131);

    const auto pcm = make_qsound_spatial_source(
        qsound_source_kind::pcm, 0, 15, 7, 0x110, static_cast<std::int16_t>(-0x1234));
    CHECK(pcm.evidence.physical_slot_present);
    CHECK(pcm.evidence.physical_slot == 15);
    CHECK(pcm.evidence.stereo_route.present);
    CHECK(pcm.evidence.stereo_route.authority == spatial_evidence_authority::device_authored);
    CHECK(near(pcm.evidence.stereo_route.left_gain, 1.0f));
    CHECK(near(pcm.evidence.stereo_route.right_gain, 0.0f));
    CHECK(pcm.echo_contribution_known);
    CHECK(pcm.echo_contribution_raw == static_cast<std::int16_t>(-0x1234));
    CHECK(pcm.evidence.effect_send_known);
    CHECK(pcm.evidence.effect_send_enabled);
    CHECK(!pcm.evidence.authored_position_present);

    const auto adpcm = make_qsound_spatial_source(
        qsound_source_kind::adpcm, 0, 2, 11, 0x130);
    CHECK(adpcm.evidence.physical_slot_present);
    CHECK(adpcm.evidence.physical_slot == 18);
    CHECK(!adpcm.echo_contribution_known);
    CHECK(!adpcm.evidence.effect_send_known);
    CHECK(!adpcm.evidence.authored_position_present);

    // PCM and ADPCM identities occupy distinct namespaces even at the same
    // local slot/generation.
    CHECK(qsound_source_id(qsound_source_kind::pcm, 0, 2, 11)
        != qsound_source_id(qsound_source_kind::adpcm, 0, 2, 11));

    // Invalid physical slots are preserved only as non-physical evidence.
    const auto invalid = make_qsound_spatial_source(
        qsound_source_kind::adpcm, 0, 3, 1, 0x120);
    CHECK(!invalid.evidence.physical_slot_present);
    CHECK(!invalid.echo_contribution_known);

    // Unknown pan state is not silently converted to a 3-D or even a decoded
    // stereo claim.
    const auto unknown = make_qsound_spatial_source(
        qsound_source_kind::pcm, 0, 0, 1, 0x171, 0);
    CHECK(!unknown.route.decoded);
    CHECK(!unknown.evidence.stereo_route.present);
    CHECK(!unknown.evidence.authored_position_present);
    CHECK(unknown.evidence.effect_send_known);
    CHECK(!unknown.evidence.effect_send_enabled);

    return 0;
}
