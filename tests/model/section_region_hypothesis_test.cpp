#include "model/section_region_hypothesis.h"

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

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}
} // namespace

int main() {
    musical_execution_graph graph;
    const node_id phrase_a = add_phrase(graph, 0, 100, 0.92);
    const node_id phrase_ap = add_phrase(graph, 100, 200, 0.90);
    const node_id phrase_b = add_phrase(graph, 200, 300, 0.88);

    // Merely placing several phrases next to one another is weak evidence for
    // a higher section. No fixed bar count or arbitrary grouping is promoted.
    const auto unstructured = infer_section_region(
        graph,
        {phrase_a, phrase_ap, phrase_b});
    CHECK(!unstructured.phrase_family_grounded);
    CHECK(!unstructured.closing_arrival_grounded);
    CHECK(close_enough(unstructured.confidence, unstructured_phrase_group_ceiling));

    phrase_relation_hypothesis aa_prime;
    aa_prime.first_phrase = phrase_a;
    aa_prime.second_phrase = phrase_ap;
    aa_prime.kind = phrase_relation_kind::varied_recurrence;
    aa_prime.confidence = 0.86;

    const auto related = infer_section_region(
        graph,
        {phrase_a, phrase_ap, phrase_b},
        {aa_prime});
    CHECK(related.phrase_family_grounded);
    CHECK(related.internal_relation_count == 1);
    CHECK(close_enough(related.confidence, related_phrase_section_ceiling));

    cadential_arrival_hypothesis close;
    close.arrival_time = at(300);
    close.confidence = 0.80;

    const auto closed = infer_section_region(
        graph,
        {phrase_a, phrase_ap, phrase_b},
        {aa_prime},
        close);
    CHECK(closed.phrase_family_grounded);
    CHECK(closed.closing_arrival_grounded);
    CHECK(close_enough(closed.confidence, 0.80));
    CHECK(closed.span.start.tick == 0);
    CHECK(closed.span.end.has_value());
    CHECK(closed.span.end->tick == 300);

    const node_id section_id = add_section_region_hypothesis(graph, closed);
    const node* section = graph.find_node(section_id);
    CHECK(section != nullptr);
    CHECK(section->kind == node_kind::section);
    CHECK(section->active.has_value());
    CHECK(section->active->end.has_value());
    CHECK(section->active->end->tick == 300);
    CHECK(graph.edges_to(section_id, edge_kind::groups_into).size() == 3);

    return 0;
}
