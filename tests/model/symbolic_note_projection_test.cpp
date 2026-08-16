#include "model/symbolic_note_projection.h"

#include <cassert>
#include <cmath>
#include <stdexcept>

using namespace vgmtooling::model;

namespace {

time_span span(std::int64_t start, std::int64_t end) {
    return {
        {time_domain::source, start, 44100, 0},
        time_coordinate{time_domain::source, end, 44100, 0},
    };
}

absolute_musical_pitch_observation pitch(
    node_id source_node,
    node_id part_id,
    double frequency_hz,
    double confidence = 1.0) {
    return {
        source_node,
        part_id,
        span(100, 200),
        frequency_hz,
        musical_pitch_role::performed,
        evidence_status::derived,
        confidence,
        "synthetic-musical-pitch",
    };
}

symbolic_note_projection_policy twelve_tet() {
    symbolic_note_projection_policy policy;
    policy.tuning = {12, 440.0, 69, 1.0, "synthetic-a440-12tet"};
    policy.maximum_deviation_cents = 35.0;
    return policy;
}

bool close_enough(double first, double second, double tolerance = 1e-9) {
    return std::fabs(first - second) <= tolerance;
}

} // namespace

int main() {
    // Exact A4 can expose MIDI note 69, but MIDI still cannot represent the
    // native timbre/control state that produced the musical pitch.
    const auto exact = make_symbolic_note_projection(pitch(1, 42, 440.0), twelve_tet());
    assert(exact.midi_note.has_value());
    assert(*exact.midi_note == 69);
    assert(exact.tuning_step == 69);
    assert(close_enough(exact.tuning_deviation_cents, 0.0));
    assert(!has_loss(exact.losses, symbolic_projection_loss::pitch_quantized));
    assert(!has_loss(exact.losses, symbolic_projection_loss::persistent_part_unknown));
    assert(has_loss(exact.losses, symbolic_projection_loss::timbre_unrepresented));
    assert(has_loss(exact.losses, symbolic_projection_loss::native_control_state_unrepresented));

    // A modest detuning can still be projected, but the quantization loss and
    // residual cents remain visible instead of being erased by the MIDI note.
    const auto detuned = make_symbolic_note_projection(pitch(2, 42, 445.0), twelve_tet());
    assert(detuned.midi_note.has_value());
    assert(*detuned.midi_note == 69);
    assert(detuned.tuning_deviation_cents > 19.0);
    assert(detuned.tuning_deviation_cents < 20.0);
    assert(has_loss(detuned.losses, symbolic_projection_loss::pitch_quantized));
    assert(detuned.confidence < 1.0);

    // Provisional symbolic export is allowed before persistent-part recovery,
    // matching the useful capability of historical VGM-to-MIDI tools without
    // pretending that the physical channel is the musical part.
    const auto unknown_part = make_symbolic_note_projection(pitch(3, 0, 440.0), twelve_tet());
    assert(unknown_part.midi_note.has_value());
    assert(has_loss(
        unknown_part.losses,
        symbolic_projection_loss::persistent_part_unknown));

    // Equal-temperament projection is more general than MIDI. 24-TET can have
    // a valid symbolic step while deliberately exposing no MIDI-note view.
    auto quarter_tone_policy = twelve_tet();
    quarter_tone_policy.tuning.divisions_per_octave = 24;
    quarter_tone_policy.tuning.reference_step = 138;
    quarter_tone_policy.tuning.source = "synthetic-a440-24tet";
    const auto quarter_tone = make_symbolic_note_projection(
        pitch(4, 42, 440.0),
        quarter_tone_policy);
    assert(quarter_tone.tuning_step == 138);
    assert(!quarter_tone.midi_note.has_value());

    // Do not round a badly fitting pitch into the nearest MIDI note merely to
    // make export convenient.
    bool rejected_bad_fit = false;
    try {
        (void)make_symbolic_note_projection(pitch(5, 42, 452.89), twelve_tet());
    } catch (const std::invalid_argument&) {
        rejected_bad_fit = true;
    }
    assert(rejected_bad_fit);

    // A normal note export needs a bounded end. Callers may explicitly relax
    // this for streaming/provisional note-on views.
    auto unbounded = pitch(6, 42, 440.0);
    unbounded.active.end.reset();
    bool rejected_unbounded = false;
    try {
        (void)make_symbolic_note_projection(unbounded, twelve_tet());
    } catch (const std::invalid_argument&) {
        rejected_unbounded = true;
    }
    assert(rejected_unbounded);

    auto streaming_policy = twelve_tet();
    streaming_policy.require_bounded_duration = false;
    const auto streaming = make_symbolic_note_projection(unbounded, streaming_policy);
    assert(streaming.midi_note.has_value());

    return 0;
}
