#include "model/analysis_feature.h"

#include <cassert>
#include <cstdint>
#include <string>

using namespace vgmtooling::model;

int main() {
    // One performed frequency is a legitimate lower musical fact without a
    // universal scale degree, tonic, note name, or 12-TET interpretation.
    analysis_feature_set performed;
    auto frequency = present_feature(
        "performed_pitch_frequency_hz",
        semantic_layer::musical_performance,
        attribute_value{440.0},
        evidence_status::derived,
        0.99,
        "Hz");
    frequency.provenance.push_back({
        evidence_status::derived,
        0.99,
        "cross-cultural-pitch-fixture",
        std::nullopt,
        "frequency recovered from performance/acoustic evidence without assigning a scale category",
    });
    performed.add(std::move(frequency));

    performed.add(unresolved_feature(
        "inferred_tonic",
        semantic_layer::musical_structure,
        feature_availability::unknown,
        "tonic is a meaningful hypothesis only under an analysis that establishes such a category",
        "cross-cultural-pitch-fixture"));
    performed.add(unresolved_feature(
        "octave_equivalence",
        semantic_layer::musical_structure,
        feature_availability::unknown,
        "octave equivalence is not assumed merely because a physical frequency is known",
        "cross-cultural-pitch-fixture"));

    assert(performed.find("performed_pitch_frequency_hz")->availability ==
           feature_availability::present);
    assert(performed.find("performed_pitch_frequency_hz")->claim_layer ==
           semantic_layer::musical_performance);
    assert(performed.find("inferred_tonic")->availability == feature_availability::unknown);
    assert(performed.find("octave_equivalence")->availability == feature_availability::unknown);

    // Two theory/culture-scoped models can categorize the same lower frequency
    // differently. Neither category rewrites the performed frequency.
    analysis_feature_set system_a;
    auto degree_a = present_feature(
        "scale_degree_under_model",
        semantic_layer::musical_structure,
        attribute_value{std::string{"degree-A"}},
        evidence_status::hypothesis,
        0.74);
    degree_a.provenance.push_back({
        evidence_status::hypothesis,
        0.74,
        "tuning-model-A",
        std::nullopt,
        "category is defined relative to model A's tonic/tuning system",
    });
    system_a.add(std::move(degree_a));

    analysis_feature_set system_b;
    auto degree_b = present_feature(
        "scale_degree_under_model",
        semantic_layer::musical_structure,
        attribute_value{std::string{"degree-B"}},
        evidence_status::hypothesis,
        0.69);
    degree_b.provenance.push_back({
        evidence_status::hypothesis,
        0.69,
        "tuning-model-B",
        std::nullopt,
        "same lower frequency categorized under a different musical system/reference",
    });
    system_b.add(std::move(degree_b));

    assert(std::get<std::string>(system_a.find("scale_degree_under_model")->value.value()) !=
           std::get<std::string>(system_b.find("scale_degree_under_model")->value.value()));
    assert(system_a.find("scale_degree_under_model")->claim_layer ==
           semantic_layer::musical_structure);
    assert(system_b.find("scale_degree_under_model")->claim_layer ==
           semantic_layer::musical_structure);
    assert(std::get<double>(performed.find("performed_pitch_frequency_hz")->value.value()) == 440.0);

    // A microtonal authored symbol can be exact while its performed acoustic
    // realization remains unknown until a tuning/reference is selected.
    analysis_feature_set authored;
    auto authored_pitch = present_feature(
        "authored_pitch_token",
        semantic_layer::authored_program,
        attribute_value{std::string{"makam-pitch-token-fixture"}},
        evidence_status::exact,
        1.0);
    authored_pitch.provenance.push_back({
        evidence_status::exact,
        1.0,
        "microtonal-score-fixture",
        std::nullopt,
        "symbolic pitch category is explicit in the source notation",
    });
    authored.add(std::move(authored_pitch));
    authored.add(unresolved_feature(
        "performed_pitch_frequency_hz",
        semantic_layer::musical_performance,
        feature_availability::unknown,
        "symbolic pitch category alone does not select one theoretical or performance-derived tuning realization",
        "microtonal-score-fixture"));

    assert(authored.find("authored_pitch_token")->status == evidence_status::exact);
    assert(authored.find("authored_pitch_token")->claim_layer == semantic_layer::authored_program);
    assert(authored.find("performed_pitch_frequency_hz")->availability ==
           feature_availability::unknown);

    // The same authored pitch can be realized through different tunings without
    // changing source identity. These are performance realizations with explicit
    // tuning provenance, not competing edits to the authored token.
    analysis_feature_set theoretical_realization;
    auto theoretical_frequency = present_feature(
        "performed_pitch_frequency_hz",
        semantic_layer::musical_performance,
        attribute_value{438.6},
        evidence_status::derived,
        1.0,
        "Hz");
    theoretical_frequency.provenance.push_back({
        evidence_status::derived,
        1.0,
        "theoretical-tuning-fixture",
        std::nullopt,
        "frequency derived from the authored pitch under one explicit theoretical tuning",
    });
    theoretical_realization.add(std::move(theoretical_frequency));

    analysis_feature_set performance_tuned_realization;
    auto performed_frequency = present_feature(
        "performed_pitch_frequency_hz",
        semantic_layer::musical_performance,
        attribute_value{441.3},
        evidence_status::derived,
        0.96,
        "Hz");
    performed_frequency.provenance.push_back({
        evidence_status::derived,
        0.96,
        "recording-derived-tuning-fixture",
        std::nullopt,
        "frequency derived from the same symbolic pitch using tuning measured from a related performance",
    });
    performance_tuned_realization.add(std::move(performed_frequency));

    assert(std::get<double>(theoretical_realization.find("performed_pitch_frequency_hz")->value.value()) !=
           std::get<double>(performance_tuned_realization.find("performed_pitch_frequency_hz")->value.value()));
    assert(std::get<std::string>(authored.find("authored_pitch_token")->value.value()) ==
           "makam-pitch-token-fixture");

    // Exact timing does not force a meter. A free-rhythm or insufficiently
    // described source can expose event times while meter remains unanswered.
    analysis_feature_set free_rhythm;
    free_rhythm.add(present_feature(
        "event_time_seconds",
        semantic_layer::musical_performance,
        attribute_value{1.375},
        evidence_status::exact,
        1.0,
        "s"));
    free_rhythm.add(unresolved_feature(
        "explicit_meter",
        semantic_layer::authored_program,
        feature_availability::unavailable,
        "source contains timing but no authored meter object",
        "free-rhythm-fixture"));
    free_rhythm.add(unresolved_feature(
        "inferred_meter",
        semantic_layer::musical_structure,
        feature_availability::unknown,
        "event timing alone does not justify one metrical interpretation",
        "free-rhythm-fixture"));

    assert(free_rhythm.find("event_time_seconds")->availability == feature_availability::present);
    assert(free_rhythm.find("explicit_meter")->availability == feature_availability::unavailable);
    assert(free_rhythm.find("inferred_meter")->availability == feature_availability::unknown);

    // A named rhythmic framework can be exact source/theory data while the
    // listener's perceived beat/meter remains a separate perceptual hypothesis.
    analysis_feature_set cyclic_rhythm;
    auto usul = present_feature(
        "authored_rhythmic_framework",
        semantic_layer::authored_program,
        attribute_value{std::string{"usul-fixture"}},
        evidence_status::exact,
        1.0);
    usul.provenance.push_back({
        evidence_status::exact,
        1.0,
        "symbolic-rhythm-fixture",
        std::nullopt,
        "named rhythmic framework is explicit in the symbolic source",
    });
    cyclic_rhythm.add(std::move(usul));

    auto perceived_meter = present_feature(
        "perceived_meter",
        semantic_layer::auditory_interpretation,
        attribute_value{std::string{"listener-meter-hypothesis"}},
        evidence_status::hypothesis,
        0.63);
    perceived_meter.provenance.push_back({
        evidence_status::hypothesis,
        0.63,
        "meter-perception-model-fixture",
        std::nullopt,
        "listener-level metrical organization inferred from rendered timing",
    });
    cyclic_rhythm.add(std::move(perceived_meter));

    assert(cyclic_rhythm.find("authored_rhythmic_framework")->claim_layer ==
           semantic_layer::authored_program);
    assert(cyclic_rhythm.find("perceived_meter")->claim_layer ==
           semantic_layer::auditory_interpretation);
    assert(cyclic_rhythm.find("authored_rhythmic_framework")->status == evidence_status::exact);
    assert(cyclic_rhythm.find("perceived_meter")->status == evidence_status::hypothesis);

    return 0;
}
