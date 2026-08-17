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

    assert(bridge.observe_frame(voices.data(), voices.size()));
    assert(capture.valid());
    assert(capture.count() == 1);
    assert(capture.frames()[0].native_sample == 41);
    assert(capture.frames()[0].source[0] == 998);
    assert(capture.frames()[0].source[1] == -1000);
    for (std::size_t voice = 2; voice < spc_native_voice_count; ++voice)
        assert(capture.frames()[0].source[voice] == 0);
    assert(bridge.next_native_sample() == 42);

    // Malformed source phase is a hard causal boundary. It must not advance the
    // native ordinal or emit a partial frame with some voices reconstructed and
    // others silently substituted.
    assert(bridge.begin_block());
    voices[3].decoded.fraction_q12 = 4096;
    assert(!bridge.observe_frame(voices.data(), voices.size()));
    assert(!bridge.active());
    assert(bridge.last_error() ==
        snes_spc_enhanced_source_hook_error::reconstruction_rejected);
    assert(capture.count() == 0);
    assert(bridge.next_native_sample() == 42);

    return 0;
}
