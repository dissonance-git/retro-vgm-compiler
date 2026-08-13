#include "components/vgm/enhancement/smps_programmed_pitch.h"

#include <cassert>

using namespace gameaudio::vgm;

int main() {
    // SMPS sequence syntax keeps rest and coordinate flags outside the note
    // domain. Do not reinterpret either one as a pitch.
    assert(smps_is_rest_token(0x80));
    assert(!smps_is_note_token(0x80));
    assert(!smps_programmed_pitch_from_token(0x80).has_value());
    assert(!smps_is_note_token(0xE0));
    assert(!smps_programmed_pitch_from_token(0xE0).has_value());

    // The compiled sequence advances one token per chromatic step. SMPS2ASM's
    // historical naming makes C0=$81, B0=$8C, C1=$8D.
    const auto c0 = smps_programmed_pitch_from_token(0x81);
    const auto b0 = smps_programmed_pitch_from_token(0x8C);
    const auto c1 = smps_programmed_pitch_from_token(0x8D);
    assert(c0.has_value() && c0->chromatic_steps_from_c0 == 0);
    assert(b0.has_value() && b0->chromatic_steps_from_c0 == 11);
    assert(c1.has_value() && c1->chromatic_steps_from_c0 == 12);

    // Track transposition is a distinct semitone displacement applied before
    // the FM/PSG frequency table lookup.
    const auto c1_down_octave = smps_programmed_pitch_from_token(0x8D, -12);
    const auto c0_up_octave = smps_programmed_pitch_from_token(0x81, 12);
    assert(c1_down_octave.has_value());
    assert(c0_up_octave.has_value());
    assert(c1_down_octave->chromatic_steps_from_c0 == 0);
    assert(c0_up_octave->chromatic_steps_from_c0 == 12);

    // Do not reproduce the driver's finite lookup-table wrap as a musical
    // meaning. Out-of-range transposition remains a signed source coordinate
    // whose executable validity must be checked by the source-family adapter.
    const auto below_c0 = smps_programmed_pitch_from_token(0x81, -1);
    assert(below_c0.has_value());
    assert(below_c0->chromatic_steps_from_c0 == -1);

    // No pitch spelling is produced here. In the assembler source nCs0 and
    // nDb0 are aliases of the same byte, so compiled data cannot recover which
    // enharmonic spelling a human author might have preferred.
    const auto black_key = smps_programmed_pitch_from_token(0x82);
    assert(black_key.has_value());
    assert(black_key->chromatic_steps_from_c0 == 1);

    return 0;
}
