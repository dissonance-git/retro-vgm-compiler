#include "model/analysis_feature.h"

#include <cassert>
#include <string>

using namespace vgmtooling::model;

namespace {

const std::string& string_value(const analysis_feature* feature) {
    assert(feature != nullptr);
    assert(feature->value.has_value());
    return std::get<std::string>(*feature->value);
}

} // namespace

int main() {
    // A source dialect is exact relative to the authored artifact. Its
    // commands should be interpreted by a dialect-specific parser before any
    // normalization to common musical meaning.
    analysis_feature_set authored;

    authored.add(present_feature(
        "source_dialect",
        semantic_layer::source_representation,
        attribute_value{std::string{"MUCOM88 MML"}},
        evidence_status::exact,
        1.0));
    authored.add(present_feature(
        "authored_gate_control",
        semantic_layer::authored_program,
        attribute_value{std::string{"q4"}},
        evidence_status::exact,
        1.0));
    authored.add(present_feature(
        "authored_modulation_control",
        semantic_layer::authored_program,
        attribute_value{std::string{"M20,1,12,4"}},
        evidence_status::exact,
        1.0));
    authored.add(present_feature(
        "authored_loop_structure",
        semantic_layer::authored_program,
        attribute_value{std::string{"source-level-repeat"}},
        evidence_status::exact,
        1.0));

    // The compiled representation may preserve corresponding semantics under
    // a different command grammar. It is a second exact representation when
    // produced or recovered from a validated compiler/format, not a synonym
    // for the original source text.
    analysis_feature_set compiled;
    compiled.add(present_feature(
        "compiled_sequence_format",
        semantic_layer::source_representation,
        attribute_value{std::string{"MUCOM88 compiled sequence"}},
        evidence_status::exact,
        1.0));
    compiled.add(present_feature(
        "compiled_note_stop_control",
        semantic_layer::driver_execution,
        attribute_value{std::string{"early-note-stop command"}},
        evidence_status::exact,
        1.0));
    compiled.add(present_feature(
        "compiled_modulation_control",
        semantic_layer::driver_execution,
        attribute_value{std::string{"modulation command"}},
        evidence_status::exact,
        1.0));
    compiled.add(present_feature(
        "compiled_loop_control",
        semantic_layer::driver_execution,
        attribute_value{std::string{"loop markers and exits"}},
        evidence_status::exact,
        1.0));

    assert(string_value(authored.find("source_dialect")) == "MUCOM88 MML");
    assert(string_value(compiled.find("compiled_sequence_format")) ==
           "MUCOM88 compiled sequence");
    assert(string_value(authored.find("source_dialect")) !=
           string_value(compiled.find("compiled_sequence_format")));

    // A smaller projection can retain useful musical behavior without
    // retaining the original source grammar or control-flow identity.
    analysis_feature_set midi_projection;
    midi_projection.add(present_feature(
        "projected_note_event",
        semantic_layer::musical_performance,
        attribute_value{std::string{"note-on/note-off"}},
        evidence_status::derived,
        1.0));
    midi_projection.add(present_feature(
        "projected_pitch_bend",
        semantic_layer::musical_performance,
        attribute_value{std::string{"pitch-bend trajectory"}},
        evidence_status::derived,
        0.98));
    midi_projection.add(unresolved_feature(
        "original_mml_command_spelling",
        semantic_layer::authored_program,
        feature_availability::unavailable,
        "the MIDI projection does not intrinsically retain the original MML command spelling",
        "authored-source-projection-boundary"));
    midi_projection.add(unresolved_feature(
        "original_loop_program_structure",
        semantic_layer::authored_program,
        feature_availability::unavailable,
        "an expanded event projection does not intrinsically preserve the authored loop/control-flow structure",
        "authored-source-projection-boundary"));

    assert(midi_projection.find("projected_note_event")->status ==
           evidence_status::derived);
    assert(midi_projection.find("original_mml_command_spelling")->availability ==
           feature_availability::unavailable);
    assert(midi_projection.find("original_loop_program_structure")->availability ==
           feature_availability::unavailable);

    // Translation policy must never become authored truth. A target format may
    // require a default when the source is underspecified, but that default is
    // evidence about the transformation, not about the composer/source.
    analysis_feature_set translation;
    auto default_octave = present_feature(
        "projection_default_octave",
        semantic_layer::source_representation,
        attribute_value{std::int64_t{5}},
        evidence_status::exact,
        1.0);
    default_octave.provenance.push_back({
        evidence_status::exact,
        1.0,
        "projection-policy-control",
        std::nullopt,
        "exact default chosen by the translator; not an authored octave fact",
    });
    translation.add(std::move(default_octave));
    translation.add(unresolved_feature(
        "authored_octave_from_translation_default",
        semantic_layer::authored_program,
        feature_availability::unknown,
        "a translator default cannot establish the source author's intended octave when the input was underspecified",
        "translation-default-boundary"));

    assert(translation.find("projection_default_octave")->status ==
           evidence_status::exact);
    assert(translation.find("authored_octave_from_translation_default")->availability ==
           feature_availability::unknown);

    return 0;
}
