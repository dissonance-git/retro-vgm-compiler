#include "model/analysis_feature.h"
#include "model/musical_execution_graph.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>

using namespace vgmtooling::model;

namespace {

node_id add_source(
    musical_execution_graph& graph,
    const char* label,
    const char* hash,
    const char* source_name) {
    node source;
    source.kind = node_kind::source_object;
    source.layer = semantic_layer::source_representation;
    source.flow = flow_kind::stream;
    source.label = label;
    source.attributes.push_back({
        "content_hash",
        std::string{hash},
        evidence_status::exact,
        1.0,
        "",
    });
    source.provenance.push_back({
        evidence_status::exact,
        1.0,
        source_name,
        0u,
        "exact artifact identity",
    });
    return graph.add_node(std::move(source));
}

node_id add_relation_node(
    musical_execution_graph& graph,
    semantic_layer layer,
    const char* label,
    const char* relation_kind,
    evidence_status status,
    double confidence,
    const char* source,
    const char* detail,
    provenance_flags flags = to_flags(provenance_flag::none)) {
    node relation;
    relation.kind = node_kind::musical_relation;
    relation.layer = layer;
    relation.flow = flow_kind::value;
    relation.label = label;
    relation.attributes.push_back({
        "relation_kind",
        std::string{relation_kind},
        status,
        confidence,
        "",
    });
    relation.provenance.push_back({
        status,
        confidence,
        source,
        std::nullopt,
        detail,
        flags,
    });
    return graph.add_node(std::move(relation));
}

void support_relation(
    musical_execution_graph& graph,
    node_id source_id,
    node_id relation_id,
    evidence_status status,
    double confidence,
    const char* evidence_source,
    const char* detail,
    provenance_flags flags = to_flags(provenance_flag::none)) {
    edge support;
    support.kind = edge_kind::derived_from;
    support.from = source_id;
    support.to = relation_id;
    support.provenance.push_back({
        status,
        confidence,
        evidence_source,
        std::nullopt,
        detail,
        flags,
    });
    graph.add_edge(std::move(support));
}

} // namespace

int main() {
    musical_execution_graph graph;

    const node_id prototype = add_source(
        graph,
        "prototype soundtrack artifact",
        "sha256-prototype",
        "prototype-source");
    const node_id final = add_source(
        graph,
        "final soundtrack artifact",
        "sha256-final",
        "final-source");

    // Artifact identity is exact and says the files are different.
    const node* prototype_node = graph.find_node(prototype);
    const node* final_node = graph.find_node(final);
    assert(prototype_node != nullptr);
    assert(final_node != nullptr);
    assert(std::get<std::string>(prototype_node->attributes[0].value) !=
           std::get<std::string>(final_node->attributes[0].value));
    assert(prototype_node->layer == semantic_layer::source_representation);
    assert(final_node->layer == semantic_layer::source_representation);

    // A musical-structure comparison can find strong similarity while the
    // artifact hashes remain different. The comparison method is part of the
    // provenance and does not prove historical identity.
    const node_id structural_similarity = add_relation_node(
        graph,
        semantic_layer::musical_structure,
        "transposition/time-warp invariant structural similarity",
        "structural_similarity",
        evidence_status::derived,
        0.94,
        "structure-comparison-fixture",
        "comparison is invariant to declared transposition/time-warp transforms");
    node* structural = graph.find_node(structural_similarity);
    assert(structural != nullptr);
    structural->attributes.push_back({
        "similarity_score",
        0.91,
        evidence_status::derived,
        0.94,
        "normalized",
    });
    support_relation(
        graph,
        prototype,
        structural_similarity,
        evidence_status::derived,
        0.94,
        "structure-comparison-fixture",
        "prototype contributes one side of the structural comparison");
    support_relation(
        graph,
        final,
        structural_similarity,
        evidence_status::derived,
        0.94,
        "structure-comparison-fixture",
        "final artifact contributes the other side of the structural comparison");

    // Similarity alone supports only a same-work hypothesis. It must live in
    // musicological_context rather than masquerading as source identity.
    const node_id same_work_from_similarity = add_relation_node(
        graph,
        semantic_layer::musicological_context,
        "same musical work hypothesis from similarity",
        "same_work_identity",
        evidence_status::hypothesis,
        0.70,
        "structure-comparison-fixture",
        "high structural similarity supports but does not prove common work identity");
    support_relation(
        graph,
        prototype,
        same_work_from_similarity,
        evidence_status::hypothesis,
        0.70,
        "structure-comparison-fixture",
        "prototype is one candidate witness of the hypothesized work");
    support_relation(
        graph,
        final,
        same_work_from_similarity,
        evidence_status::hypothesis,
        0.70,
        "structure-comparison-fixture",
        "final is the second candidate witness of the hypothesized work");
    support_relation(
        graph,
        structural_similarity,
        same_work_from_similarity,
        evidence_status::hypothesis,
        0.70,
        "structure-comparison-fixture",
        "similarity result is evidence for, not identity with, the musicological claim");

    // External documentation is a different evidence route. The fact that a
    // catalog states one arrangement ID can be represented exactly relative to
    // that catalog while retaining external-annotation provenance.
    analysis_feature_set external_catalog;
    auto shared_arrangement_id = present_feature(
        "documented_shared_arrangement_id",
        semantic_layer::musicological_context,
        attribute_value{std::string{"ARR-0042"}},
        evidence_status::exact,
        1.0);
    shared_arrangement_id.support_nodes.push_back(prototype);
    shared_arrangement_id.support_nodes.push_back(final);
    shared_arrangement_id.provenance.push_back({
        evidence_status::exact,
        1.0,
        "external-catalog-fixture",
        std::nullopt,
        "catalog explicitly assigns both artifacts the same arrangement identifier",
        to_flags(provenance_flag::external_annotation),
    });
    external_catalog.add(std::move(shared_arrangement_id));

    const node_id documented_arrangement_relation = add_relation_node(
        graph,
        semantic_layer::musicological_context,
        "documented shared arrangement relation",
        "same_arrangement_identity",
        evidence_status::derived,
        1.0,
        "external-catalog-fixture",
        "same arrangement relation derived from explicit shared catalog identifier",
        to_flags(provenance_flag::external_annotation));
    support_relation(
        graph,
        prototype,
        documented_arrangement_relation,
        evidence_status::derived,
        1.0,
        "external-catalog-fixture",
        "prototype carries the documented arrangement identifier",
        to_flags(provenance_flag::external_annotation));
    support_relation(
        graph,
        final,
        documented_arrangement_relation,
        evidence_status::derived,
        1.0,
        "external-catalog-fixture",
        "final carries the documented arrangement identifier",
        to_flags(provenance_flag::external_annotation));

    // Stylistic attribution remains yet another claim. It is not strengthened
    // merely because the same-work or same-arrangement relation is stronger.
    analysis_feature_set attribution_analysis;
    auto composer = present_feature(
        "composer_attribution",
        semantic_layer::musicological_context,
        attribute_value{std::string{"composer-X"}},
        evidence_status::hypothesis,
        0.58);
    composer.support_nodes.push_back(prototype);
    composer.support_nodes.push_back(final);
    composer.provenance.push_back({
        evidence_status::hypothesis,
        0.58,
        "style-model-fixture",
        std::nullopt,
        "style similarity is attribution evidence, not documentary composer proof",
    });
    attribution_analysis.add(std::move(composer));

    const auto* arrangement_feature = external_catalog.find("documented_shared_arrangement_id");
    const auto* attribution_feature = attribution_analysis.find("composer_attribution");
    assert(arrangement_feature != nullptr);
    assert(attribution_feature != nullptr);
    assert(arrangement_feature->claim_layer == semantic_layer::musicological_context);
    assert(arrangement_feature->status == evidence_status::exact);
    assert(has_flag(arrangement_feature->provenance[0].flags, provenance_flag::external_annotation));
    assert(attribution_feature->claim_layer == semantic_layer::musicological_context);
    assert(attribution_feature->status == evidence_status::hypothesis);

    // Structural similarity and musicological identity are separate claims.
    assert(graph.find_node(structural_similarity)->layer == semantic_layer::musical_structure);
    assert(graph.find_node(same_work_from_similarity)->layer == semantic_layer::musicological_context);
    assert(graph.find_node(documented_arrangement_relation)->layer == semantic_layer::musicological_context);
    assert(graph.find_node(same_work_from_similarity)->attributes[0].status == evidence_status::hypothesis);
    assert(graph.find_node(documented_arrangement_relation)->attributes[0].status == evidence_status::derived);

    // Neither higher relation mutates the exact source identities.
    assert(std::get<std::string>(graph.find_node(prototype)->attributes[0].value) == "sha256-prototype");
    assert(std::get<std::string>(graph.find_node(final)->attributes[0].value) == "sha256-final");
    assert(graph.find_node(prototype)->provenance[0].status == evidence_status::exact);
    assert(graph.find_node(final)->provenance[0].status == evidence_status::exact);

    // Unknown historical direction is different from a false direction.
    analysis_feature_set chronology;
    chronology.add(unresolved_feature(
        "documented_derivation_direction",
        semantic_layer::musicological_context,
        feature_availability::unknown,
        "the fixture establishes related artifacts but contains no documentary evidence proving which derived from which",
        "version-identity-fixture"));
    assert(chronology.find("documented_derivation_direction")->availability == feature_availability::unknown);
    assert(!chronology.find("documented_derivation_direction")->value.has_value());

    return 0;
}
