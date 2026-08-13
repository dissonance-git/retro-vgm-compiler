#include "components/vgm/enhancement/genesis_nominal_pitch.h"

#include <cassert>
#include <cmath>

using namespace gameaudio::vgm;

namespace {

bool near(double lhs, double rhs, double tolerance) {
    return std::abs(lhs - rhs) <= tolerance;
}

} // namespace

int main() {
    // Missing/invalid device coordinates stay missing rather than becoming 0 Hz.
    assert(!ym2612_nominal_pitch_frequency_hz(0, 4, 7670453).has_value());
    assert(!ym2612_nominal_pitch_frequency_hz(0x400, 8, 7670453).has_value());
    assert(!ym2612_nominal_pitch_frequency_hz(0x400, 4, 0).has_value());
    assert(!sn76489_nominal_pitch_frequency_hz(0, 3579545).has_value());
    assert(!sn76489_nominal_pitch_frequency_hz(0x400, 3579545).has_value());
    assert(!sn76489_nominal_pitch_frequency_hz(0x100, 0).has_value());

    // YM BLOCK is an octave coordinate at fixed FNUM.
    const auto ym_block4 = ym2612_nominal_pitch_frequency_hz(0x434, 4, 7670453);
    const auto ym_block5 = ym2612_nominal_pitch_frequency_hz(0x434, 5, 7670453);
    assert(ym_block4.has_value());
    assert(ym_block5.has_value());
    assert(near(*ym_block5, *ym_block4 * 2.0, 1e-9));

    // Concrete Sonic 3 corpus clock example. This remains a continuous source-
    // side frequency coordinate, not an assertion that the patch is exactly an
    // equal-tempered note or that its acoustic fundamental equals this value.
    assert(near(*ym_block5, 874.5625207689073, 1e-9));

    // PSG tone frequency is inversely proportional to its programmed period.
    const auto psg_125 = sn76489_nominal_pitch_frequency_hz(0x125, 3579545);
    const auto psg_24a = sn76489_nominal_pitch_frequency_hz(0x24a, 3579545);
    assert(psg_125.has_value());
    assert(psg_24a.has_value());
    assert(near(*psg_125, *psg_24a * 2.0, 1e-9));
    assert(near(*psg_125, 381.7774104095563, 1e-9));

    // Clock provenance matters. The same register state under another master
    // clock is a different frequency and must not be silently normalized away.
    const auto ym_other_clock = ym2612_nominal_pitch_frequency_hz(0x434, 5, 8000000);
    const auto psg_other_clock = sn76489_nominal_pitch_frequency_hz(0x125, 4000000);
    assert(ym_other_clock.has_value());
    assert(psg_other_clock.has_value());
    assert(*ym_other_clock != *ym_block5);
    assert(*psg_other_clock != *psg_125);

    return 0;
}
