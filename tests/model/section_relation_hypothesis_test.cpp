#include "model/section_relation_hypothesis.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 0, 0};
}

node_id add_phrase(
    musical_execution_graph& graph,
    std::int64_t start,
    std::int64_t end,
    double confidence) {
    node phrase;
    phrase.kind = node_kind::section;
    phrase.layer = semantic_layer::musical_structure;
    phrase.flow = flow_kind::stream;
    phrase.label = "phrase region hypothesis";
    phrase.active = time_span{at(start), at(end)};
    phrase.attributes.push_back({
        "identity_scope",
        std::string{"phrase_region_hypothesis"},
        evidence_status::hypothesis,
        confidence,
        "",
    });
    return graph.add_node(std::move(phrase));
}

node_id add_section(
    musical_execution_graph& graph,
    std::vector<node_id> phrases,
    std::int64_t start,
    std::int64_t end,
    double confidence) {
    section_region_hypothesis section;
    section.span = time_span{at(start), at(end)};
    section.phrase_ids = std::move(phrases);
    section.confidence = confidence;
    return add_section_region_hypothesis(graph, section);
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}
} // namespace

int main() {
    musical_execution_graph graph;
    const node_id a1 = add_phrase(graph, 0, 100, 0.92);
    const node_id a2 = add_phrase(graph, 100, 200, 0.90);
    const node_id ap1 = add_phrase(graph, 200, 300, 0.89);
    const node_id ap2 = add_phrase(graph, 300, 400, 0.88);

    const node_id first_section = add_section(graph, {a1, a2}, 0, 200, 0.85);
    const node_id second_section = add_section(graph, {ap1, ap2}, 200, 400, 0.83);

    phrase_relation_hypothesis first_link;
    first_link.first_phrase = a1;
    first_link.second_phrase = ap1;
    first_link.kind = phrase_relation_kind::recurrence;
    first_link.confidence = 0.90;

    // One phrase link suggests development, but cannot establish a strong
    // whole-section identity.
    const auto partial = infer_section_relation(
        graph,
        first_section,
        second_section,
        {first_link},
        0.95);
    CHECK(partial.kind == section_relation_kind::developmental_relation);
    CHECK(!partial.multi_phrase_grounded);
    CHECK(close_enough(partial.first_phrase_coverage, 0.5));
    CHECK(close_enough(partial.second_phrase_coverage, 0.5));
    CHECK(close_enough(partial.confidence, single_phrase_section_relation_ceiling));

    phrase_relation_hypothesis second_link;
    second_link.first_phrase = a2;
    second_link.second_phrase = ap2;
    second_link.kind = phrase_relation_kind::varied_recurrence;
    second_link.confidence = 0.84;

    // Both phrase families recur, with one varied return. The section-level
    // relation becomes A -> A' without inventing a conventional form label.
    const auto varied = infer_section_relation(
        graph,
        first_section,
        second_section,
        {first_link, second_link},
        0.95);
    CHECK(varied.kind == section_relation_kind::varied_recurrence);
    CHECK(varied.multi_phrase_grounded);
    CHECK(varied.cross_phrase_relation_count == 2);
    CHECK(close_enough(varied.first_phrase_coverage, 1.0));
    CHECK(close_enough(varied.second_phrase_coverage, 1.0));
    CHECK(close_enough(varied.confidence, 0.83));

    const edge_id relation_id = add_section_relation_hypothesis(graph, varied);
    const edge* relation = graph.find_edge(relation_id);
    CHECK(relation != nullptr);
    CHECK(relation->kind == edge_kind::transforms);
    CHECK(relation->from == first_section);
    CHECK(relation->to == second_section);

    // Harmonic rhythm is corroborating evidence only. A poor rhythmic match
    // can demote an otherwise attractive section relation.
    const auto rhythm_conflict = infer_section_relation(
        graph,
        first_section,
        second_section,
        {first_link, second_link},
        0.40);
    CHECK(close_enough(rhythm_conflict.confidence, 0.40));

    return 0;
}
