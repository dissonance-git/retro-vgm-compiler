#include "components/spc/spc_enhanced_reconstruction.h"

#include <cassert>
#include <cmath>
#include <cstdint>

using gameaudio::spc::apply_spc_reference_envelope_quantization;
using gameaudio::spc::reconstruct_spc_lanczos4;
using gameaudio::spc::spc_decoded_source_window;
using gameaudio::spc::spc_floor_divide_2048;

int main() {
    // Constant input must remain constant at every fractional position. This
    // catches gain ripple introduced by a finite, unnormalized sinc window.
    spc_decoded_source_window constant;
    constant.samples.fill(12000);
    for (const std::uint16_t fraction : {0u, 1u, 1024u, 2048u, 3072u, 4095u}) {
        constant.fraction_q12 = fraction;
        const auto reconstructed = reconstruct_spc_lanczos4(constant);
        assert(reconstructed.valid);
        assert(std::abs(reconstructed.sample - 12000.0) < 1.0e-8);
    }

    // An integer source coordinate is an identity point. Enhanced
    // reconstruction must not move an already exact decoded sample.
    spc_decoded_source_window integer_position;
    integer_position.samples = {-1200, 300, -700, 4321, 9100, -2400, 800, 50};
    integer_position.fraction_q12 = 0;
    const auto integer_result = reconstruct_spc_lanczos4(integer_position);
    assert(integer_result.valid);
    assert(std::abs(integer_result.sample - 4321.0) < 1.0e-8);

    // A linear source neighborhood should interpolate near the midpoint rather
    // than collapse to either adjacent decoded sample. The finite Lanczos
    // window is not required to reproduce a polynomial exactly; this only locks
    // the causal source coordinate and neighborhood orientation.
    spc_decoded_source_window ramp;
    ramp.samples = {-3000, -2000, -1000, 0, 1000, 2000, 3000, 4000};
    ramp.fraction_q12 = 2048;
    const auto midpoint = reconstruct_spc_lanczos4(ramp);
    assert(midpoint.valid);
    assert(midpoint.sample > 400.0);
    assert(midpoint.sample < 600.0);

    // The q12 phase contract fails closed rather than wrapping an invalid phase
    // into a different source coordinate.
    auto invalid = ramp;
    invalid.fraction_q12 = 4096;
    assert(!reconstruct_spc_lanczos4(invalid).valid);

    // Reconstruction and envelope precision remain separate experiments. The
    // first enhanced implementation can replace interpolation while retaining
    // the historical envelope quantization boundary.
    assert(apply_spc_reference_envelope_quantization(16000.0, 0x0400u) == 8000);
    assert(apply_spc_reference_envelope_quantization(-16000.0, 0x0400u) == -8000);
    assert((apply_spc_reference_envelope_quantization(12345.0, 0x07FFu) & 1) == 0);
    assert(apply_spc_reference_envelope_quantization(50000.0, 0x07FFu) <= 32767);

    // Preserve the arithmetic-right-shift behavior explicitly even on C++17
    // implementations whose signed right shift would make a different choice.
    assert(spc_floor_divide_2048(2049) == 1);
    assert(spc_floor_divide_2048(-2049) == -2);
    assert(spc_floor_divide_2048(-1) == -1);
    assert(apply_spc_reference_envelope_quantization(-1.0, 1u) == -2);

    return 0;
}
