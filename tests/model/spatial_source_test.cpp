#include "model/spatial_source.h"
#include "components/vgm/enhancement/genesis_spatial_source.h"
#include "components/spc/spc_spatial_source.h"

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
    assert(!spc_caps.isolated_dry_pcm);
    assert(!spc_caps.shared_effect_return);
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

    const auto genesis = gameaudio::vgm::make_genesis_spatial_source(
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
    assert(!vgmtooling::model::may_claim_authored_3d(genesis));

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
    assert(spc.effect_send_known && spc.effect_send_enabled);
    assert(!vgmtooling::model::may_claim_authored_3d(spc));

    auto true_position = spc;
    true_position.authored_position_present = true;
    true_position.authored_position[0] = 0.25f;
    true_position.authored_position[1] = -0.5f;
    true_position.authored_position[2] = 1.0f;
    assert(vgmtooling::model::may_claim_authored_3d(true_position));

    return 0;
}
