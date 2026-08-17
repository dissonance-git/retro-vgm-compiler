#include "model/spatial_source.h"
#include "model/spatial_playback_options.h"
#include "components/vgm/enhancement/genesis_spatial_source.h"
#include "components/spc/spc_spatial_source.h"
#include "components/spc/spc_source_bus.h"

#include <cassert>
#include <cmath>

using vgmtooling::model::spatial_source_family;

int main() {
    const auto vgm_caps = vgmtooling::model::spatial_capabilities_for(spatial_source_family::vgm);
    assert(vgm_caps.stable_source_evidence);
    assert(vgm_caps.authored_stereo_route);
    assert(vgm_caps.readiness == vgmtooling::model::spatial_source_readiness::isolated_audio_partial);
    assert(vgm_caps.isolated_dry_pcm);
    assert(vgm_caps.protected_reference_mix);
    assert(!vgm_caps.shared_effect_return);
    assert(!vgm_caps.exact_linear_recomposition);
    assert(!vgm_caps.authored_3d_position);

    const auto spc_caps = vgmtooling::model::spatial_capabilities_for(spatial_source_family::spc);
    assert(spc_caps.stable_source_evidence);
    assert(spc_caps.authored_stereo_route);
    assert(spc_caps.effect_send_state);
    assert(spc_caps.protected_reference_mix);
    assert(spc_caps.readiness == vgmtooling::model::spatial_source_readiness::isolated_audio_partial);
    assert(spc_caps.isolated_dry_pcm);
    assert(spc_caps.shared_effect_return);
    assert(!spc_caps.exact_linear_recomposition);
    assert(!spc_caps.authored_3d_position);

    for (const auto family : {
             spatial_source_family::psf1,
             spatial_source_family::gsf,
             spatial_source_family::usf,
             spatial_source_family::twosf,
             spatial_source_family::ncsf,
         }) {
        const auto caps = vgmtooling::model::spatial_capabilities_for(family);
        assert(caps.readiness == vgmtooling::model::spatial_source_readiness::unavailable);
        assert(!caps.stable_source_evidence);
        assert(!caps.isolated_dry_pcm);
        assert(!caps.shared_effect_return);
        assert(!caps.exact_linear_recomposition);
        assert(!caps.protected_reference_mix);
        assert(!caps.authored_3d_position);
    }

    // The protected reference is an authority/control path, not an object to be
    // spatialized alongside the isolated sources that produced it.
    assert(vgmtooling::model::spatial_audio_lane_is_object_renderable(
        vgmtooling::model::spatial_audio_lane_kind::dry_source));
    assert(vgmtooling::model::spatial_audio_lane_is_object_renderable(
        vgmtooling::model::spatial_audio_lane_kind::shared_effect_return));
    assert(!vgmtooling::model::spatial_audio_lane_is_object_renderable(
        vgmtooling::model::spatial_audio_lane_kind::reference_mix));

    // Both foobar components expose enhancement and spatialization as separate
    // user choices. Toggling one must not silently toggle the other.
    vgmtooling::model::spatial_playback_options playback{};
    assert(!playback.surround);
    assert(!playback.enhanced);
    assert(playback.externalization);
    assert(playback.depth == vgmtooling::model::spatial_depth_mode::full);
    assert(vgmtooling::model::resolve_spatial_playback(playback)
        == vgmtooling::model::spatial_playback_path::reference_stereo);
    assert(!vgmtooling::model::uses_source_renderer(playback));
    assert(!vgmtooling::model::uses_enhanced_renderer(playback));
    assert(!vgmtooling::model::uses_externalization(playback));

    playback.enhanced = true;
    assert(vgmtooling::model::uses_enhanced_renderer(playback));
    assert(vgmtooling::model::resolve_spatial_playback(playback)
        == vgmtooling::model::spatial_playback_path::reference_stereo);
    assert(!vgmtooling::model::uses_source_renderer(playback));

    playback.surround = true;
    assert(vgmtooling::model::resolve_spatial_playback(playback)
        == vgmtooling::model::spatial_playback_path::source_full_sphere);
    assert(vgmtooling::model::uses_source_renderer(playback));
    assert(vgmtooling::model::uses_enhanced_renderer(playback));
    assert(vgmtooling::model::uses_externalization(playback));

    playback.enhanced = false;
    assert(!vgmtooling::model::uses_enhanced_renderer(playback));
    assert(vgmtooling::model::uses_source_renderer(playback));

    playback.depth = vgmtooling::model::spatial_depth_mode::native;
    assert(vgmtooling::model::resolve_spatial_playback(playback)
        == vgmtooling::model::spatial_playback_path::source_native_routing);
    playback.externalization = false;
    assert(!vgmtooling::model::uses_externalization(playback));

    auto genesis = gameaudio::vgm::make_genesis_spatial_source(
        gameaudio::vgm::genesis_spatial_device::ym2612_fm,
        0,
        3,
        11,
        gameaudio::vgm::ym2612_authored_route(true, false));
    assert(genesis.family == spatial_source_family::vgm);
    assert(genesis.physical_slot_present && genesis.physical_slot == 3);
    assert(genesis.stereo_route.present);
    assert(genesis.stereo_route.left_gain == 1.0f);
    assert(genesis.stereo_route.right_gain == 0.0f);
    assert(!genesis.stereo_route.gain_preapplied);
    assert(!vgmtooling::model::may_claim_authored_3d(genesis));

    // Arithmetic provenance does not change what the signed native route means.
    // It only prevents a downstream renderer from applying that gain twice.
    auto preapplied = genesis;
    preapplied.stereo_route.gain_preapplied = true;
    assert(preapplied.stereo_route.present);
    assert(preapplied.stereo_route.left_gain == genesis.stereo_route.left_gain);
    assert(preapplied.stereo_route.right_gain == genesis.stereo_route.right_gain);
    assert(!vgmtooling::model::may_claim_authored_3d(preapplied));

    // Musical/presentation evidence is allowed to guide Omniphony, but remains
    // explicitly inferred and never upgrades itself into authored geometry.
    genesis.presentation.foundation = 0.85f;
    genesis.presentation.foreground = 0.40f;
    genesis.presentation.vertical_affinity = -0.25f;
    genesis.presentation.confidence = 0.90f;
    assert(genesis.presentation.authority == vgmtooling::model::spatial_evidence_authority::inferred);
    assert(!vgmtooling::model::may_claim_authored_3d(genesis));

    float mono[4] = {0.0f, 0.25f, -0.25f, 0.0f};
    const vgmtooling::model::spatial_audio_lane_view lane{
        vgmtooling::model::spatial_audio_lane_kind::dry_source,
        mono,
        genesis,
    };
    const vgmtooling::model::spatial_source_block_view block{&lane, 1, 4};
    assert(block.lane_count == 1);
    assert(block.frame_count == 4);
    assert(block.lanes[0].mono_pcm == mono);
    assert(block.lanes[0].kind == vgmtooling::model::spatial_audio_lane_kind::dry_source);

    gameaudio::spc::spc_runtime_capture_record record;
    record.fields = gameaudio::spc::spc_runtime_capture_field::voice
        | gameaudio::spc::spc_runtime_capture_field::route_gain_left
        | gameaudio::spc::spc_runtime_capture_field::route_gain_right
        | gameaudio::spc::spc_runtime_capture_field::echo_send_enabled;
    record.voice = 6;
    record.route_gain_left = -128;
    record.route_gain_right = 64;
    record.echo_send_enabled = true;

    const auto spc = gameaudio::spc::make_spc_spatial_source(record, 7);
    assert(spc.family == spatial_source_family::spc);
    assert(spc.physical_slot_present && spc.physical_slot == 6);
    assert(spc.stereo_route.present);
    assert(spc.stereo_route.left_gain == -1.0f);
    assert(std::fabs(spc.stereo_route.right_gain - (64.0f / 127.0f)) < 1.0e-6f);
    assert(!spc.stereo_route.gain_preapplied);
    assert(spc.effect_send_known && spc.effect_send_enabled);
    assert(!vgmtooling::model::may_claim_authored_3d(spc));

    // The S-DSP echo return is one shared stereo field, not eight fictional
    // per-instrument wet stems. Preserve L/R polarity and link both halves to
    // one stable field identity for Omniphony presentation.
    const auto echo_left = gameaudio::spc::spc_source_bus::make_echo_source(
        gameaudio::spc::spc_source_bus::echo_side::left, 9, -128);
    const auto echo_right = gameaudio::spc::spc_source_bus::make_echo_source(
        gameaudio::spc::spc_source_bus::echo_side::right, 9, 64);
    assert(echo_left.persistent_part_present && echo_right.persistent_part_present);
    assert(echo_left.persistent_part_id == echo_right.persistent_part_id);
    assert(echo_left.source_id != echo_right.source_id);
    assert(echo_left.stereo_route.left_gain == -1.0f);
    assert(echo_left.stereo_route.right_gain == 0.0f);
    assert(echo_right.stereo_route.left_gain == 0.0f);
    assert(std::fabs(echo_right.stereo_route.right_gain - (64.0f / 127.0f)) < 1.0e-6f);
    assert(echo_left.presentation.diffuse == 1.0f);
    assert(echo_left.presentation.width == 1.0f);
    assert(!vgmtooling::model::may_claim_authored_3d(echo_left));

    auto true_position = spc;
    true_position.authored_position_present = true;
    true_position.authored_position[0] = 0.25f;
    true_position.authored_position[1] = -0.5f;
    true_position.authored_position[2] = 1.0f;
    assert(vgmtooling::model::may_claim_authored_3d(true_position));

    assert(vgmtooling::model::clamp_unit_interval(-0.5f) == 0.0f);
    assert(vgmtooling::model::clamp_unit_interval(0.5f) == 0.5f);
    assert(vgmtooling::model::clamp_unit_interval(1.5f) == 1.0f);

    return 0;
}
