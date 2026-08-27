#include "model/harmonic_span_relation.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 1000, 0};
}

time_span span(std::int64_t start, std::int64_t end) {
    return {at(start), at(end)};
}

node_id add_harmonic_event(
    musical_execution_graph& graph,
    const char* label,
    std::int64_t tick) {
    node value;
    value.kind = node_kind::pattern;
    value.layer = semantic_layer::musical_structure;
    value.flow = flow_kind::value;
    value.label = label;
    value.active = time_span{at(tick), std::nullopt};
    return graph.add_node(std::move(value));
}

harmonic_transition_hypothesis transition(
    std::int64_t first_tick,
    std::int64_t second_tick,
    std::int64_t first_root,
    std::int64_t second_root,
    double confidence) {
    harmonic_transition_hypothesis result;
    result.first_time = at(first_tick);
    result.second_time = at(second_tick);
    result.first_root_pitch_class = first_root;
    result.second_root_pitch_class = second_root;
    result.first_quality = tertian_triad_quality::major;
    result.second_quality = tertian_triad_quality::major;
    result.root_motion_reliable = true;
    result.confidence = confidence;
    return result;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    musical_execution_graph graph;
    const node_id a = add_harmonic_event(graph, "A", 100);
    const node_id b = add_harmonic_event(graph, "B", 140);
    const node_id c = add_harmonic_event(graph, "C", 180);
    const node_id d = add_harmonic_event(graph, "D", 220);

    const std::vector<harmonic_transition_hypothesis> chain{
        transition(100, 140, 0, 5, 0.93),
        transition(140, 180, 5, 7, 0.91),
        transition(180, 220, 7, 0, 0.89),
    };
    const auto scope = span(100, 220);

    voice_leading_hypothesis voices;
    voices.first_time = at(100);
    voices.second_time = at(220);
    voices.motions.push_back({60, 60, 0, a, a, true});
    voices.identity_preserved_voices = 1;
    voices.all_correspondence_identity_grounded = true;
    voices.confidence = 0.84;
    const auto voice_evidence =
        make_voice_leading_harmonic_span_evidence(
            voices, scope, {a, d});

    tonal_region_relation_hypothesis retained;
    retained.kind = tonal_region_relation_kind::retained_center;
    retained.source_region = span(90, 120);
    retained.target_region = span(200, 240);
    retained.centers_equivalent = true;
    retained.confidence = 0.86;
    const auto tonal_evidence =
        make_retained_center_harmonic_span_evidence(
            retained, scope, {a, d});

    // Equal-looking endpoints are not enough. Prolongation requires both
    // cross-span continuity and an independently retained structural anchor,
    // while B and C remain first-class surface events.
    const auto prolongation = make_harmonic_span_relation_hypothesis(
        graph,
        harmonic_span_relation_kind::prolongation_candidate,
        chain,
        {a, b, c, d},
        {voice_evidence, tonal_evidence});
    CHECK(prolongation.surface_events_preserved);
    CHECK(prolongation.transition_count == 3);
    CHECK(prolongation.intervening_event_count == 2);
    CHECK(prolongation.surface_nodes.size() == 4);
    CHECK(prolongation.support_domains == 2);
    CHECK(prolongation.cross_domain_grounded);
    CHECK(!prolongation.tonal_function_named);
    CHECK(!prolongation.cadence_class_established);
    CHECK(!prolongation.relation_established);
    CHECK(close_enough(prolongation.confidence, 0.84));

    const auto prolongation_role =
        project_harmonic_span_phrase_role_evidence(
            prolongation,
            phrase_role_formal_scale::phrase_group);
    CHECK(prolongation_role.role == phrase_role_kind::prolongation);
    CHECK(prolongation_role.origin ==
        phrase_role_evidence_origin::harmonic_dependency);
    CHECK(prolongation_role.support_nodes.size() == 4);
    CHECK(close_enough(prolongation_role.confidence, 0.84));

    const node_id prolongation_node =
        add_harmonic_span_relation_hypothesis(graph, prolongation);
    CHECK(graph.find_node(prolongation_node) != nullptr);
    CHECK(graph.edges_to(
        prolongation_node,
        edge_kind::derived_from).size() >= 4);

    bool equality_only_rejected = false;
    try {
        (void)make_harmonic_span_relation_hypothesis(
            graph,
            harmonic_span_relation_kind::prolongation_candidate,
            chain,
            {a, b, c, d},
            {tonal_evidence});
    } catch (const std::invalid_argument&) {
        equality_only_rejected = true;
    }
    CHECK(equality_only_rejected);

    phrase_role_evidence continuing;
    continuing.role = phrase_role_kind::continuation;
    continuing.scope = span(120, 200);
    continuing.formal_scale = phrase_role_formal_scale::phrase_group;
    continuing.origin = phrase_role_evidence_origin::harmonic_process;
    continuing.polarity = phrase_role_evidence_polarity::supports;
    continuing.status = evidence_status::hypothesis;
    continuing.confidence = 0.80;
    continuing.source = "continuation-regression";
    continuing.detail = "the earlier process remains active through intervening material";
    continuing.support_nodes = {b, c};
    const auto unresolved =
        make_unresolved_process_harmonic_span_evidence(
            continuing, scope);

    cadential_arrival_hypothesis later_arrival;
    later_arrival.departure_time = at(180);
    later_arrival.arrival_time = at(220);
    later_arrival.cross_part_phrase_grounded = true;
    later_arrival.harmonic_root_motion_reliable = true;
    later_arrival.voice_leading_grounded = true;
    later_arrival.confidence = 0.82;
    const auto arrival_evidence =
        make_later_arrival_harmonic_span_evidence(
            later_arrival, scope, {c, d});

    const auto delayed = make_harmonic_span_relation_hypothesis(
        graph,
        harmonic_span_relation_kind::delayed_resolution_candidate,
        chain,
        {a, b, c, d},
        {voice_evidence, unresolved, arrival_evidence});
    CHECK(delayed.surface_events_preserved);
    CHECK(delayed.support_domains == 3);
    CHECK(delayed.cross_domain_grounded);
    CHECK(!delayed.tonal_function_named);
    CHECK(!delayed.cadence_class_established);
    CHECK(!delayed.relation_established);
    CHECK(close_enough(delayed.confidence, 0.82));

    const auto delayed_role =
        project_harmonic_span_phrase_role_evidence(
            delayed,
            phrase_role_formal_scale::phrase_group);
    CHECK(delayed_role.role == phrase_role_kind::delayed_resolution);
    CHECK(delayed_role.origin ==
        phrase_role_evidence_origin::harmonic_dependency);
    CHECK(close_enough(delayed_role.confidence, 0.82));

    // A later arrival plus smooth endpoint voice leading cannot retroactively
    // invent an unresolved dependency that was never observed.
    bool arrival_without_unresolved_rejected = false;
    try {
        (void)make_harmonic_span_relation_hypothesis(
            graph,
            harmonic_span_relation_kind::delayed_resolution_candidate,
            chain,
            {a, b, c, d},
            {voice_evidence, arrival_evidence});
    } catch (const std::invalid_argument&) {
        arrival_without_unresolved_rejected = true;
    }
    CHECK(arrival_without_unresolved_rejected);

    // The local surface chain is not decorative metadata. Breaking B -> C must
    // invalidate the long-range candidate rather than silently hopping over it.
    bool broken_surface_rejected = false;
    try {
        auto broken = chain;
        broken[1].first_time = at(150);
        (void)make_harmonic_span_relation_hypothesis(
            graph,
            harmonic_span_relation_kind::prolongation_candidate,
            broken,
            {a, b, c, d},
            {voice_evidence, tonal_evidence});
    } catch (const std::invalid_argument&) {
        broken_surface_rejected = true;
    }
    CHECK(broken_surface_rejected);

    const auto contradiction = make_harmonic_span_evidence(
        harmonic_span_evidence_kind::contradiction,
        harmonic_span_evidence_origin::external_annotation,
        harmonic_span_evidence_polarity::counters,
        0.91,
        "contradictory-analysis",
        "independent analysis rejects the proposed long-range dependency",
        {b, c});
    const auto conflicted = make_harmonic_span_relation_hypothesis(
        graph,
        harmonic_span_relation_kind::delayed_resolution_candidate,
        chain,
        {a, b, c, d},
        {voice_evidence, unresolved, arrival_evidence, contradiction});
    CHECK(conflicted.strong_conflict_present);
    CHECK(close_enough(conflicted.confidence, 0.49));
    CHECK(!conflicted.relation_established);

    return 0;
}
