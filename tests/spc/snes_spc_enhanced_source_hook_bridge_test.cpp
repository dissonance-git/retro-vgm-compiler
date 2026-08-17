#include "components/spc/snes_spc_enhanced_source_hook_bridge.h"

#include <array>
#include <cassert>

int main() {
    using namespace gameaudio::spc;

    spc_native_source_capture capture;
    snes_spc_enhanced_source_hook_bridge bridge;
    bridge.reset(&capture, 41);
    assert(bridge.active());
    assert(bridge.next_native_sample() == 41);
    assert(bridge.begin_block());

    std::array<spc_enhanced_voice_input, spc_native_voice_count> voices{};
    for (auto& voice : voices) {
        voice.decoded.samples.fill(0);
        voice.decoded.fraction_q12 = 0;
        voice.envelope = 0;
    }

    // Sampled voice: interpolation may change, but the first experiment keeps
    // the native envelope quantization boundary.
    voices[0].decoded.samples.fill(1000);
    voices[0].decoded.fraction_q12 = 2048;
    voices[0].envelope = 0x07FF;

    // Noise voice: native noise bypasses BRR interpolation and therefore must
    // not be fed through the replacement reconstruction filter.
    voices[1].noise_enabled = true;
    voices[1].noise_sample = -2000;
    voices[1].envelope = 0x0400;

    // Evidence-approved source restoration enters at the same pre-envelope
    // coordinate. The upstream PCM is normalized; the proven preparation scale
    // maps 0.5 back to 1000 decoded-game PCM units before the historical
    // envelope arithmetic is applied.
    std::array<float, 8> upstream{};
    upstream.fill(0.5f);
    spc_sample_restoration_candidate restored;
    restored.game_brr_identity = {1, 1};
    restored.upstream_identity = {2, 2};
    restored.relation = spc_sample_lineage_relation::exact_pre_brr_source;
    restored.evidence = spc_sample_restoration_evidence::exact_upstream_source;
    restored.upstream = {upstream.data(), upstream.size(), 48000.0, 2000.0};
    restored.coordinate_map.game_origin = 0.0;
    restored.coordinate_map.upstream_origin = 0.0;
    restored.coordinate_map.upstream_frames_per_game_sample = 1.0;
    restored.coordinate_map.preparation_chain_exact = true;
    restored.identity_validation_passed = true;

    voices[2].restoration = &restored;
    voices[2].game_sample_position = 3.5;
    voices[2].envelope = 0x07FF;

    assert(bridge.observe_frame(voices.data(), voices.size()));
    assert(capture.valid());
    assert(capture.count() == 1);
    assert(capture.frames()[0].native_sample == 41);
    assert(capture.frames()[0].source[0] == 998);
    assert(capture.frames()[0].source[1] == -1000);
    assert(capture.frames()[0].source[2] == 998);
    for (std::size_t voice = 3; voice < spc_native_voice_count; ++voice)
        assert(capture.frames()[0].source[voice] == 0);
    assert(bridge.next_native_sample() == 42);

    // A supplied restoration is a committed Enhanced source choice. If its
    // evidence gate becomes invalid, do not silently fall back to the BRR voice
    // for just that frame.
    bridge.reset(&capture, 42);
    assert(bridge.begin_block());
    auto unvalidated = restored;
    unvalidated.identity_validation_passed = false;
    voices[2].restoration = &unvalidated;
    assert(!bridge.observe_frame(voices.data(), voices.size()));
    assert(!bridge.active());
    assert(bridge.last_error() ==
        snes_spc_enhanced_source_hook_error::restoration_rejected);
    assert(capture.count() == 0);
    assert(bridge.next_native_sample() == 42);

    // Malformed decoded-source phase is likewise a hard causal boundary when no
    // upstream restoration is selected.
    bridge.reset(&capture, 42);
    assert(bridge.begin_block());
    voices[2].restoration = nullptr;
    voices[3].decoded.fraction_q12 = 4096;
    assert(!bridge.observe_frame(voices.data(), voices.size()));
    assert(!bridge.active());
    assert(bridge.last_error() ==
        snes_spc_enhanced_source_hook_error::reconstruction_rejected);
    assert(capture.count() == 0);
    assert(bridge.next_native_sample() == 42);

    return 0;
}
