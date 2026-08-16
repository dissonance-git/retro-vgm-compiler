#include "model/soundtrack_relation_hypothesis.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

using namespace vgmtooling::model;

namespace {

node_id add_cue(musical_execution_graph& graph, const char* label) {
    node value;
    value.kind = node_kind::source_object;
    value.layer = semantic_layer::source_representation;
    value.flow = flow_kind::stream;
    value.label = label;
    return graph.add_node(std::move(value));
}

node_id add_feature(
    musical_execution_graph& graph,
    const char* label,
    semantic_layer layer = semantic_layer::musical_structure) {
    node value;
    value.kind = node_kind::musical_relation;
    value.layer = layer;
    value.flow = flow_kind::value;
    value.label = label;
    return graph.add_node(std::move(value));
}

const attribute* find_attribute(const node& value, const std::string& key) {
    for (const auto& item : value.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

bool close_enough(double lhs, double rhs) {
    return std::abs(lhs - rhs) < 1e-9;
}

} // namespace

int main() {
    musical_execution_graph graph;

    const node_id cue_a = add_cue(graph, "cue A");
    const node_id cue_b = add_cue(graph, "cue B");
    const node_id cue_c = add_cue(graph, "cue C");

    const node_id melody_match = add_feature(graph, "transposition-invariant melodic match");
    const node_id harmony_match = add_feature(graph, "shared harmonic-function path");
    const node_id orchestration_match = add_feature(graph, "orchestration-family remapping");

    // A domain-specific structural claim may be strong on evidence from that
    // domain alone. Detecting a motif under transposition does not require a
    // second domain merely to establish motif recurrence.
    auto motif = make_soundtrack_relation_hypothesis(
        soundtrack_relation_kind::motif_recurrence,
        0.96,
        {cue_a, cue_b},
        {
            {
                soundtrack_evidence_domain::melody,
                soundtrack_evidence_origin::musical_analysis,
                evidence_status::derived,
                0.96,
                "fixture-melodic-comparator",
                "interval contour and rhythmic skeleton match after transposition",
                musical_transformation_kind::transposition,
                {melody_match},
            },
        });

    assert(motif.status == evidence_status::derived);
    assert(close_enough(motif.confidence, 0.96));
    assert(!motif.cross_domain_grounded);
    assert(soundtrack_relation_layer(motif.relation) == semantic_layer::musical_structure);

    const node_id motif_relation = add_soundtrack_relation_hypothesis(graph, motif);
    const node* motif_node = graph.find_node(motif_relation);
    assert(motif_node != nullptr);
    assert(motif_node->layer == semantic_layer::musical_structure);
    assert(std::get<std::string>(find_attribute(*motif_node, "soundtrack_relation")->value) ==
           "motif_recurrence");
    assert(std::get<std::string>(find_attribute(*motif_node, "transformation_0")->value) ==
           "transposition");

    // A broad cue-family claim is different. One melodic resemblance is not
    // enough to turn two tracks into a high-confidence family relation.
    auto single_domain_family = make_soundtrack_relation_hypothesis(
        soundtrack_relation_kind::cue_family,
        0.94,
        {cue_a, cue_c},
        {
            {
                soundtrack_evidence_domain::melody,
                soundtrack_evidence_origin::musical_analysis,
                evidence_status::derived,
                0.94,
                "fixture-melodic-comparator",
                "shared head motif",
                std::nullopt,
                {melody_match},
            },
        });

    assert(single_domain_family.status == evidence_status::hypothesis);
    assert(!single_domain_family.cross_domain_grounded);
    assert(close_enough(single_domain_family.confidence, soundtrack_single_domain_context_ceiling));
    assert(soundtrack_relation_layer(single_domain_family.relation) ==
           semantic_layer::musicological_context);

    // Independent musical domains can ground the broader relation without
    // requiring the exact same representation in each cue.
    auto cross_domain_family = make_soundtrack_relation_hypothesis(
        soundtrack_relation_kind::cue_family,
        0.92,
        {cue_a, cue_b, cue_c},
        {
            {
                soundtrack_evidence_domain::melody,
                soundtrack_evidence_origin::musical_analysis,
                evidence_status::derived,
                0.95,
                "fixture-melodic-comparator",
                "recurring interval skeleton across all three cues",
                musical_transformation_kind::transposition,
                {melody_match},
            },
            {
                soundtrack_evidence_domain::harmony,
                soundtrack_evidence_origin::musical_analysis,
                evidence_status::derived,
                0.89,
                "fixture-harmonic-comparator",
                "shared functional harmonic route with altered surface voicing",
                musical_transformation_kind::reharmonization,
                {harmony_match},
            },
            {
                soundtrack_evidence_domain::orchestration,
                soundtrack_evidence_origin::sequence_or_engine,
                evidence_status::derived,
                0.87,
                "fixture-sequence-analysis",
                "voice roles map onto the same arrangement skeleton with different instrument families",
                musical_transformation_kind::orchestration_remap,
                {orchestration_match},
            },
        });

    assert(cross_domain_family.cross_domain_grounded);
    assert(close_enough(cross_domain_family.confidence, 0.92));

    const node_id family_relation =
        add_soundtrack_relation_hypothesis(graph, cross_domain_family);
    const node* family_node = graph.find_node(family_relation);
    assert(family_node != nullptr);
    assert(family_node->layer == semantic_layer::musicological_context);
    assert(family_node->provenance.size() == 3);
    assert(find_attribute(*family_node, "transformation_0") != nullptr);
    assert(find_attribute(*family_node, "transformation_1") != nullptr);
    assert(find_attribute(*family_node, "transformation_2") != nullptr);

    // Three subject edges plus three evidence edges remain explicit. This lets
    // downstream tools distinguish 'which cues are related' from 'what facts
    // support that relation'.
    const auto family_edges = graph.edges_to(family_relation, edge_kind::derived_from);
    assert(family_edges.size() == 6);

    // A direct documentary source may ground a family/arrangement claim even
    // when the machine analysis currently exposes only one musical domain.
    auto documented_arrangement = make_soundtrack_relation_hypothesis(
        soundtrack_relation_kind::arrangement_relation,
        0.98,
        {cue_a, cue_b},
        {
            {
                soundtrack_evidence_domain::engine_identity,
                soundtrack_evidence_origin::external_annotation,
                evidence_status::exact,
                1.0,
                "fixture-official-cue-sheet",
                "both cues are documented as variants of the same arrangement identifier",
                std::nullopt,
                {},
            },
        });

    assert(documented_arrangement.cross_domain_grounded);
    assert(close_enough(documented_arrangement.confidence, 0.98));
    const node_id arrangement_relation =
        add_soundtrack_relation_hypothesis(graph, documented_arrangement);
    const node* arrangement_node = graph.find_node(arrangement_relation);
    assert(arrangement_node != nullptr);
    assert(has_flag(arrangement_node->provenance[0].flags, provenance_flag::external_annotation));

    bool rejected_single_subject = false;
    try {
        (void)make_soundtrack_relation_hypothesis(
            soundtrack_relation_kind::formal_parallel,
            0.8,
            {cue_a},
            {{
                soundtrack_evidence_domain::form,
                soundtrack_evidence_origin::musical_analysis,
                evidence_status::derived,
                0.8,
                "fixture",
                "one cue cannot form an inter-cue relation",
                std::nullopt,
                {},
            }});
    } catch (const std::invalid_argument&) {
        rejected_single_subject = true;
    }
    assert(rejected_single_subject);

    bool rejected_empty_evidence = false;
    try {
        (void)make_soundtrack_relation_hypothesis(
            soundtrack_relation_kind::cue_family,
            0.8,
            {cue_a, cue_b},
            {});
    } catch (const std::invalid_argument&) {
        rejected_empty_evidence = true;
    }
    assert(rejected_empty_evidence);

    return 0;
}
