#include "model/analysis_feature.h"

#include <cassert>
#include <cstdint>
#include <string>

using namespace vgmtooling::model;

int main() {
    // Exact authored/source-facing semantics can coexist with exact synthesis
    // identity without proving a literal acoustic instrument.
    analysis_feature_set source;
    source.add(present_feature(
        "authored_instrument_label",
        semantic_layer::authored_program,
        attribute_value{std::string{"strings"}},
        evidence_status::exact,
        1.0));
    source.add(present_feature(
        "synthesis_object_id",
        semantic_layer::synthesis,
        attribute_value{std::string{"ym2612-patch-P"}},
        evidence_status::exact,
        1.0));

    assert(source.find("authored_instrument_label")->claim_layer == semantic_layer::authored_program);
    assert(source.find("synthesis_object_id")->claim_layer == semantic_layer::synthesis);
    assert(source.find("synthesis_object_id")->status == evidence_status::exact);

    // The same exact synthesis object can produce different acoustic descriptor
    // values under different pitches/dynamics. Descriptor change is therefore
    // not synthesis-object identity change.
    analysis_feature_set realization_low;
    realization_low.add(present_feature(
        "source_synthesis_object_id",
        semantic_layer::synthesis,
        attribute_value{std::string{"ym2612-patch-P"}},
        evidence_status::exact,
        1.0));
    realization_low.add(present_feature(
        "spectral_centroid_hz",
        semantic_layer::acoustic_realization,
        attribute_value{1830.0},
        evidence_status::derived,
        0.99,
        "Hz"));
    realization_low.add(present_feature(
        "attack_time_ms",
        semantic_layer::acoustic_realization,
        attribute_value{26.0},
        evidence_status::derived,
        0.96,
        "ms"));

    analysis_feature_set realization_high;
    realization_high.add(present_feature(
        "source_synthesis_object_id",
        semantic_layer::synthesis,
        attribute_value{std::string{"ym2612-patch-P"}},
        evidence_status::exact,
        1.0));
    realization_high.add(present_feature(
        "spectral_centroid_hz",
        semantic_layer::acoustic_realization,
        attribute_value{2475.0},
        evidence_status::derived,
        0.99,
        "Hz"));
    realization_high.add(present_feature(
        "attack_time_ms",
        semantic_layer::acoustic_realization,
        attribute_value{18.0},
        evidence_status::derived,
        0.96,
        "ms"));

    assert(std::get<std::string>(realization_low.find("source_synthesis_object_id")->value.value()) ==
           std::get<std::string>(realization_high.find("source_synthesis_object_id")->value.value()));
    assert(std::get<double>(realization_low.find("spectral_centroid_hz")->value.value()) !=
           std::get<double>(realization_high.find("spectral_centroid_hz")->value.value()));
    assert(std::get<double>(realization_low.find("attack_time_ms")->value.value()) !=
           std::get<double>(realization_high.find("attack_time_ms")->value.value()));

    // An audio classifier can suggest a source family, but the output remains
    // a model/listener-level hypothesis with training/model provenance.
    analysis_feature_set auditory;
    auto classifier = present_feature(
        "instrument_family_classifier_output",
        semantic_layer::auditory_interpretation,
        attribute_value{std::string{"violin_family"}},
        evidence_status::hypothesis,
        0.81);
    classifier.provenance.push_back({
        evidence_status::hypothesis,
        0.81,
        "instrument-classifier-fixture/v3",
        std::nullopt,
        "classifier trained on a finite acoustic-instrument label set; output is not source identity",
    });
    auditory.add(std::move(classifier));

    assert(auditory.find("instrument_family_classifier_output")->claim_layer ==
           semantic_layer::auditory_interpretation);
    assert(auditory.find("instrument_family_classifier_output")->status == evidence_status::hypothesis);
    assert(!auditory.find("instrument_family_classifier_output")->provenance.empty());

    // The authored label and classifier output may be compatible without being
    // the same claim or proving a literal historical/physical instrument.
    assert(std::get<std::string>(source.find("authored_instrument_label")->value.value()) == "strings");
    assert(std::get<std::string>(auditory.find("instrument_family_classifier_output")->value.value()) ==
           "violin_family");

    analysis_feature_set organology;
    organology.add(unresolved_feature(
        "organological_class",
        semantic_layer::musicological_context,
        feature_availability::not_applicable,
        "the source object is deliberately electronic FM synthesis and does not require a physical acoustic-instrument classification",
        "synthetic-source-fixture"));
    organology.add(unresolved_feature(
        "reference_acoustic_instrument_identity",
        semantic_layer::musicological_context,
        feature_availability::unknown,
        "the authored strings label and violin-family classifier are insufficient to prove one specific reference instrument",
        "synthetic-source-fixture"));

    assert(organology.find("organological_class")->availability == feature_availability::not_applicable);
    assert(organology.find("organological_class")->claim_layer == semantic_layer::musicological_context);
    assert(organology.find("reference_acoustic_instrument_identity")->availability ==
           feature_availability::unknown);

    // Musical role is independent again. The same exact patch can serve two
    // different structural/performance functions without becoming two patches.
    analysis_feature_set role_a;
    auto melody = present_feature(
        "musical_role",
        semantic_layer::musical_structure,
        attribute_value{std::string{"melody"}},
        evidence_status::hypothesis,
        0.76);
    melody.provenance.push_back({
        evidence_status::hypothesis,
        0.76,
        "role-analysis-fixture/A",
        std::nullopt,
        "role inferred from register/activity/context rather than patch identity",
    });
    role_a.add(std::move(melody));

    analysis_feature_set role_b;
    auto accompaniment = present_feature(
        "musical_role",
        semantic_layer::musical_structure,
        attribute_value{std::string{"accompaniment"}},
        evidence_status::hypothesis,
        0.69);
    accompaniment.provenance.push_back({
        evidence_status::hypothesis,
        0.69,
        "role-analysis-fixture/B",
        std::nullopt,
        "same synthesis object used in a different musical context",
    });
    role_b.add(std::move(accompaniment));

    assert(std::get<std::string>(role_a.find("musical_role")->value.value()) !=
           std::get<std::string>(role_b.find("musical_role")->value.value()));
    assert(std::get<std::string>(source.find("synthesis_object_id")->value.value()) == "ym2612-patch-P");

    // Enhanced realization is a candidate solution, not recovered historical
    // source truth. It belongs to acoustic realization while its source support
    // remains explicit in provenance.
    analysis_feature_set reconstruction;
    auto candidate = present_feature(
        "candidate_reconstruction_id",
        semantic_layer::acoustic_realization,
        attribute_value{std::string{"high-resolution-realization-17"}},
        evidence_status::hypothesis,
        0.64);
    candidate.provenance.push_back({
        evidence_status::hypothesis,
        0.64,
        "source-conditioned-reconstruction-fixture",
        std::nullopt,
        "candidate preserves exact patch/musical constraints while improving acoustic realization; not claimed as original acoustic instrument truth",
    });
    reconstruction.add(std::move(candidate));

    assert(reconstruction.find("candidate_reconstruction_id")->claim_layer ==
           semantic_layer::acoustic_realization);
    assert(reconstruction.find("candidate_reconstruction_id")->status == evidence_status::hypothesis);

    // None of the later claims changes the exact synthesis identity.
    assert(std::get<std::string>(source.find("synthesis_object_id")->value.value()) == "ym2612-patch-P");

    return 0;
}
