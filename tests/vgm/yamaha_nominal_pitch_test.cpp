#include "yamaha_nominal_pitch.h"

#include <cassert>
#include <cmath>

using namespace gameaudio::vgm;

static bool near(const double a, const double b, const double tolerance = 1e-9) {
    return std::abs(a - b) <= tolerance;
}

int main() {
    // Different Yamaha FM branches can encode the same nominal channel basis
    // differently. These controls land at the same ~439.990595 Hz without
    // converting any of them to a note name.
    const auto opl2 = opl_nominal_pitch_frequency_hz(
        580, 4, 3'579'545u, opl_chip_variant::ym3812);
    const auto opl3 = opl_nominal_pitch_frequency_hz(
        580, 4, 14'318'180u, opl_chip_variant::ymf262);
    const auto opll = opll_nominal_pitch_frequency_hz(
        290, 4, 3'579'545u);

    assert(opl2.has_value());
    assert(opl3.has_value());
    assert(opll.has_value());
    assert(near(*opl2, 439.990594651964));
    assert(near(*opl2, *opl3));
    assert(near(*opl2, *opll));

    // YM3526/Y8950 share the same OPL clock division as YM3812 at this layer.
    const auto opl = opl_nominal_pitch_frequency_hz(
        580, 4, 3'579'545u, opl_chip_variant::ym3526);
    const auto y8950 = opl_nominal_pitch_frequency_hz(
        580, 4, 3'579'545u, opl_chip_variant::y8950);
    assert(opl.has_value() && y8950.has_value());
    assert(near(*opl, *opl2));
    assert(near(*y8950, *opl2));

    // One block step doubles the nominal basis while leaving the device-native
    // FNUM untouched.
    const auto opl2_octave = opl_nominal_pitch_frequency_hz(
        580, 5, 3'579'545u, opl_chip_variant::ym3812);
    const auto opll_octave = opll_nominal_pitch_frequency_hz(
        290, 5, 3'579'545u);
    assert(opl2_octave.has_value() && opll_octave.has_value());
    assert(near(*opl2_octave, *opl2 * 2.0));
    assert(near(*opll_octave, *opll * 2.0));

    // Fail closed rather than manufacturing a musical pitch from absent or
    // out-of-range device state.
    assert(!opl_nominal_pitch_frequency_hz(
        0, 4, 3'579'545u, opl_chip_variant::ym3812).has_value());
    assert(!opl_nominal_pitch_frequency_hz(
        1024, 4, 3'579'545u, opl_chip_variant::ym3812).has_value());
    assert(!opl_nominal_pitch_frequency_hz(
        580, 8, 3'579'545u, opl_chip_variant::ym3812).has_value());
    assert(!opl_nominal_pitch_frequency_hz(
        580, 4, 0, opl_chip_variant::ym3812).has_value());

    assert(!opll_nominal_pitch_frequency_hz(0, 4, 3'579'545u).has_value());
    assert(!opll_nominal_pitch_frequency_hz(512, 4, 3'579'545u).has_value());
    assert(!opll_nominal_pitch_frequency_hz(290, 8, 3'579'545u).has_value());
    assert(!opll_nominal_pitch_frequency_hz(290, 4, 0).has_value());

    return 0;
}
