#include "components/spc/spc_enhanced_native_interval.h"

#include <array>
#include <cassert>
#include <cmath>

int main() {
    using namespace gameaudio::spc;

    spc_enhanced_native_interval constant;
    constant.decoded.fill(12000);
    constant.decoded_origin_sample = 0;
    constant.start_q12 = 7 * 4096;
    constant.pitch_step_q12 = 4096;
    constant.envelope = 0x07FF;

    std::array<std::int16_t, 8> out{};
    assert(render_spc_native_interval_multiple(constant, 3, out));
    // A constant source remains constant at 96 kHz phases 0, 1/3, 2/3.
    assert(out[0] == out[1]);
    assert(out[1] == out[2]);

    // A changing source produces genuinely distinct sub-native samples. This is
    // not a 32 kHz sample duplicated three times and not final-bus upsampling.
    spc_enhanced_native_interval ramp = constant;
    for (std::size_t i = 0; i < ramp.decoded.size(); ++i)
        ramp.decoded[i] = static_cast<std::int16_t>(static_cast<int>(i) * 1000 - 7000);
    assert(render_spc_native_interval_multiple(ramp, 3, out));
    assert(out[0] < out[1]);
    assert(out[1] < out[2]);
    assert(out[0] != out[1]);
    assert(out[1] != out[2]);

    // At phase zero the source coordinate is exactly integer sample 7. The
    // replacement filter therefore lands on that decoded sample before envelope
    // arithmetic (sample value 0 in this ramp).
    const auto phase_zero = reconstruct_spc_native_interval(ramp, 0.0);
    assert(phase_zero.valid);
    assert(std::abs(phase_zero.pre_envelope) < 1.0e-8);
    assert(phase_zero.post_envelope == 0);

    // Native-rate noise is held across the interval. Inventing intermediate LFSR
    // states would falsely create information the S-DSP never generated.
    spc_enhanced_native_interval noise = constant;
    noise.noise_enabled = true;
    noise.noise_sample = -6000;
    noise.envelope = 0x0400;
    assert(render_spc_native_interval_multiple(noise, 3, out));
    assert(out[0] == -3000);
    assert(out[1] == -3000);
    assert(out[2] == -3000);

    // Phase 1 belongs to the next interval, so there is exactly one owner for a
    // native boundary sample.
    assert(!reconstruct_spc_native_interval(ramp, 1.0).valid);
    assert(!reconstruct_spc_native_interval(ramp, -0.1).valid);

    // Live BRR data is not edge-padded. If the exact Lanczos neighborhood has
    // not been decoded yet, publication of that interval must wait/fail.
    auto missing_future = ramp;
    missing_future.start_q12 = 13 * 4096;
    assert(!reconstruct_spc_native_interval(missing_future, 0.0).valid);

    // KON/hold intervals with pitch zero are legal and remain stationary at all
    // output phases.
    auto held = ramp;
    held.start_q12 = 8 * 4096;
    held.pitch_step_q12 = 0;
    assert(render_spc_native_interval_multiple(held, 3, out));
    assert(out[0] == out[1]);
    assert(out[1] == out[2]);

    return 0;
}
