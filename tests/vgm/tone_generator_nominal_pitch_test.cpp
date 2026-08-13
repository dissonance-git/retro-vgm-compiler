#include "tone_generator_nominal_pitch.h"

#include <cassert>
#include <cmath>

namespace {

void expect_near(const double actual, const double expected, const double tolerance = 1e-9) {
    assert(std::abs(actual - expected) <= tolerance);
}

} // namespace

int main() {
    using namespace gameaudio::vgm;

    // These clocks are the actual clocks declared by the permanent real-music
    // corpus controls. The integer device coordinates below all land near A4,
    // but the test deliberately stops at nominal Hz and never assigns a note.
    const auto ay = ay8910_tone_nominal_frequency_hz(254, 1789773);
    const auto scc = k051649_wave_nominal_frequency_hz(212, 1500000);
    const auto huc = huc6280_wave_nominal_frequency_hz(254, 3579545);
    const auto nes_square = nes_apu_square_nominal_frequency_hz(253, 1789772);
    const auto nes_triangle = nes_apu_triangle_nominal_frequency_hz(126, 1789772);
    const auto gb_square = game_boy_square_nominal_frequency_hz(1750, 4194304);
    const auto gb_wave = game_boy_wave_nominal_frequency_hz(1899, 4194304);

    assert(ay && scc && huc && nes_square && nes_triangle && gb_square && gb_wave);
    expect_near(*ay, 440.3968996062992);
    expect_near(*scc, 440.14084507042253);
    expect_near(*huc, 440.39677657480314);
    expect_near(*nes_square, 440.3966535433071);
    expect_near(*nes_triangle, 440.3966535433071);
    expect_near(*gb_square, 439.83892617449663);
    expect_near(*gb_wave, 439.83892617449663);

    // A shared numerical coordinate does not imply a shared pitch law even
    // inside one chip. On DMG, the same 11-bit value feeds an 8-step square
    // sequencer and a 32-sample wavetable sequencer, producing a 2:1 result.
    const auto gb_same_code_square = game_boy_square_nominal_frequency_hz(1750, 4194304);
    const auto gb_same_code_wave = game_boy_wave_nominal_frequency_hz(1750, 4194304);
    assert(gb_same_code_square && gb_same_code_wave);
    expect_near(*gb_same_code_wave, *gb_same_code_square / 2.0);

    // AY tone period zero aliases period one, while SCC exposes a separate
    // halted low-period region. These are device semantics, not generic period
    // arithmetic.
    const auto ay_zero = ay8910_tone_nominal_frequency_hz(0, 1789773);
    const auto ay_one = ay8910_tone_nominal_frequency_hz(1, 1789773);
    assert(ay_zero && ay_one);
    expect_near(*ay_zero, *ay_one);
    assert(k051649_frequency_is_halted(8));
    assert(!k051649_frequency_is_halted(9));

    // HuC6280 register zero wraps to the longest 12-bit waveform period.
    const auto huc_zero = huc6280_wave_nominal_frequency_hz(0, 3579545);
    const auto huc_4095 = huc6280_wave_nominal_frequency_hz(4095, 3579545);
    assert(huc_zero && huc_4095);
    assert(*huc_zero < *huc_4095);

    // Fail closed on impossible coordinates or missing clocks.
    assert(!ay8910_tone_nominal_frequency_hz(0x1000, 1789773));
    assert(!game_boy_square_nominal_frequency_hz(0x0800, 4194304));
    assert(!k051649_wave_nominal_frequency_hz(0x1000, 1500000));
    assert(!huc6280_wave_nominal_frequency_hz(0x1000, 3579545));
    assert(!nes_apu_square_nominal_frequency_hz(0x0800, 1789772));
    assert(!nes_apu_triangle_nominal_frequency_hz(0x0800, 1789772));
    assert(!ay8910_tone_nominal_frequency_hz(254, 0));

    return 0;
}
