#include "model/analysis_feature.h"

#include <cassert>
#include <cstdint>
#include <string>

using namespace vgmtooling::model;

int main() {
    // Lower musical facts are deliberately identical for both modeled listeners.
    analysis_feature_set music;
    music.add(present_feature(
        "onset_count",
        semantic_layer::musical_performance,
        attribute_value{std::uint64_t{16}},
        evidence_status::derived,
        1.0,
        "events"));
    music.add(present_feature(
        "syncopation_index",
        semantic_layer::musical_structure,
        attribute_value{0.42},
        evidence_status::derived,
        0.94,
        "normalized"));
    music.add(present_feature(
        "perceived_beat_rate",
        semantic_layer::auditory_interpretation,
        attribute_value{120.0},
        evidence_status::hypothesis,
        0.91,
        "bpm"));

    const auto* syncopation = music.find("syncopation_index");
    const auto* beat = music.find("perceived_beat_rate");
    assert(syncopation != nullptr);
    assert(beat != nullptr);
    assert(syncopation->claim_layer == semantic_layer::musical_structure);
    assert(beat->claim_layer == semantic_layer::auditory_interpretation);

    // Listener/model A is familiar with the style and the piece. The response
    // values are hypotheses/model outputs, not properties of the source music.
    analysis_feature_set listener_a;
    auto groove_a = present_feature(
        "urge_to_move",
        semantic_layer::listener_response,
        attribute_value{0.82},
        evidence_status::hypothesis,
        0.73,
        "normalized");
    groove_a.provenance.push_back({
        evidence_status::hypothesis,
        0.73,
        "groove-model-fixture/A",
        std::nullopt,
        "response model assumes high style familiarity and listener preference; identical musical evidence is consumed without modifying it",
    });
    listener_a.add(std::move(groove_a));

    auto surprise_a = present_feature(
        "melodic_information_content",
        semantic_layer::listener_response,
        attribute_value{2.1},
        evidence_status::hypothesis,
        0.80,
        "bits");
    surprise_a.provenance.push_back({
        evidence_status::hypothesis,
        0.80,
        "expectation-model-fixture/A",
        std::nullopt,
        "long-term corpus includes the target style and short-term context includes the preceding phrase",
    });
    listener_a.add(std::move(surprise_a));

    auto valence_a = present_feature(
        "felt_valence",
        semantic_layer::listener_response,
        attribute_value{0.71},
        evidence_status::hypothesis,
        0.60,
        "normalized");
    valence_a.provenance.push_back({
        evidence_status::hypothesis,
        0.60,
        "emotion-mechanism-fixture/A",
        std::nullopt,
        "felt-emotion estimate conditioned on familiarity/episodic-association fixture",
    });
    listener_a.add(std::move(valence_a));

    // Listener/model B hears exactly the same lower music but has a different
    // learned distribution and no familiarity. Different response values are
    // therefore legitimate without creating a different source/performance graph.
    analysis_feature_set listener_b;
    auto groove_b = present_feature(
        "urge_to_move",
        semantic_layer::listener_response,
        attribute_value{0.37},
        evidence_status::hypothesis,
        0.66,
        "normalized");
    groove_b.provenance.push_back({
        evidence_status::hypothesis,
        0.66,
        "groove-model-fixture/B",
        std::nullopt,
        "response model assumes low style familiarity and neutral preference",
    });
    listener_b.add(std::move(groove_b));

    auto surprise_b = present_feature(
        "melodic_information_content",
        semantic_layer::listener_response,
        attribute_value{5.4},
        evidence_status::hypothesis,
        0.78,
        "bits");
    surprise_b.provenance.push_back({
        evidence_status::hypothesis,
        0.78,
        "expectation-model-fixture/B",
        std::nullopt,
        "long-term corpus is stylistically mismatched while the observed melody is unchanged",
    });
    listener_b.add(std::move(surprise_b));

    auto valence_b = present_feature(
        "felt_valence",
        semantic_layer::listener_response,
        attribute_value{0.43},
        evidence_status::hypothesis,
        0.52,
        "normalized");
    valence_b.provenance.push_back({
        evidence_status::hypothesis,
        0.52,
        "emotion-mechanism-fixture/B",
        std::nullopt,
        "felt-emotion estimate without the episodic-familiarity condition used for listener A",
    });
    listener_b.add(std::move(valence_b));

    const auto* groove_a_value = listener_a.find("urge_to_move");
    const auto* groove_b_value = listener_b.find("urge_to_move");
    const auto* surprise_a_value = listener_a.find("melodic_information_content");
    const auto* surprise_b_value = listener_b.find("melodic_information_content");
    const auto* valence_a_value = listener_a.find("felt_valence");
    const auto* valence_b_value = listener_b.find("felt_valence");

    assert(groove_a_value != nullptr);
    assert(groove_b_value != nullptr);
    assert(surprise_a_value != nullptr);
    assert(surprise_b_value != nullptr);
    assert(valence_a_value != nullptr);
    assert(valence_b_value != nullptr);

    assert(groove_a_value->claim_layer == semantic_layer::listener_response);
    assert(groove_b_value->claim_layer == semantic_layer::listener_response);
    assert(surprise_a_value->claim_layer == semantic_layer::listener_response);
    assert(valence_a_value->claim_layer == semantic_layer::listener_response);

    assert(std::get<double>(groove_a_value->value.value()) !=
           std::get<double>(groove_b_value->value.value()));
    assert(std::get<double>(surprise_a_value->value.value()) !=
           std::get<double>(surprise_b_value->value.value()));
    assert(std::get<double>(valence_a_value->value.value()) !=
           std::get<double>(valence_b_value->value.value()));

    // Model identity/context must stay in provenance. A listener-response value
    // with no context can still be represented, but these controls require the
    // response hypotheses to say why they differ.
    assert(!groove_a_value->provenance.empty());
    assert(!groove_b_value->provenance.empty());
    assert(groove_a_value->provenance[0].source != groove_b_value->provenance[0].source);

    // The lower evidence is unchanged by either response model.
    assert(std::get<double>(music.find("syncopation_index")->value.value()) == 0.42);
    assert(std::get<double>(music.find("perceived_beat_rate")->value.value()) == 120.0);
    assert(music.find("syncopation_index")->claim_layer == semantic_layer::musical_structure);
    assert(music.find("perceived_beat_rate")->claim_layer == semantic_layer::auditory_interpretation);

    // A response can be meaningful but unavailable from source-only analysis.
    analysis_feature_set source_only;
    source_only.add(unresolved_feature(
        "felt_valence",
        semantic_layer::listener_response,
        feature_availability::unavailable,
        "executable source truth alone does not determine one listener's felt emotion",
        "source-only-fixture"));
    source_only.add(unresolved_feature(
        "urge_to_move",
        semantic_layer::listener_response,
        feature_availability::unavailable,
        "musical structure alone does not determine one listener's groove response",
        "source-only-fixture"));
    source_only.add(unresolved_feature(
        "episodic_familiarity",
        semantic_layer::listener_response,
        feature_availability::unavailable,
        "listener memory state is not stored in the executable music source",
        "source-only-fixture"));

    assert(source_only.find("felt_valence")->availability == feature_availability::unavailable);
    assert(source_only.find("urge_to_move")->availability == feature_availability::unavailable);
    assert(source_only.find("episodic_familiarity")->availability == feature_availability::unavailable);

    return 0;
}
