#include "model/persistent_part_hypothesis.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

using namespace vgmtooling::model;

namespace {

node_id add_episode(
    musical_execution_graph& graph,
    std::int64_t start,
    std::int64_t end,
    const char* label) {
    node value;
    value.kind = node_kind::voice_instance;
    value.layer = semantic_layer::synthesis;
    value.flow = flow_kind::stream;
    value.label = label;
    value.active = time_span{
        {time_domain::device, start, 32000, 0},
        time_coordinate{time_domain::device, end, 32000, 0},
    };
    return graph.add_node(std::move(value));
}

const attribute* find_attribute(const node& value, const char* key) {
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
    const node_id a = add_episode(graph, 0, 100, "episode A");
    const node_id b = add_episode(graph, 120, 220, "episode B");
    const node_id c = add_episode(graph, 150, 250, "episode C");

    auto slot_only = make_persistent_part_hypothesis(
        0.95,
        {a, b},
        {{
            persistent_part_evidence_kind::physical_slot_continuity,
            persistent_part_evidence_origin::synthesis_runtime,
            persistent_part_evidence_polarity::supports,
            evidence_status::exact,
            1.0,
            "fixture-runtime",
            "both episodes used the same hardware slot",
            {a, b},
        }});
    assert(!slot_only.identity_bearing_support);
    assert(!slot_only.cross_domain_grounded);
    assert(close_enough(slot_only.confidence, persistent_part_slot_only_confidence_ceiling));

    auto identity_only = make_persistent_part_hypothesis(
        0.93,
        {a, b},
        {{
            persistent_part_evidence_kind::source_identity,
            persistent_part_evidence_origin::synthesis_runtime,
            persistent_part_evidence_polarity::supports,
            evidence_status::derived,
            0.95,
            "fixture-runtime",
            "both episodes reference the same event-time source object",
            {a, b},
        }});
    assert(identity_only.identity_bearing_support);
    assert(!identity_only.cross_domain_grounded);
    assert(close_enough(identity_only.confidence, persistent_part_single_domain_confidence_ceiling));

    auto cross_domain = make_persistent_part_hypothesis(
        0.86,
        {a, b},
        {
            {
                persistent_part_evidence_kind::instrument_program_identity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.93,
                "fixture-program-fingerprint",
                "same pitch-invariant synthesis-program fingerprint",
                {a, b},
            },
            {
                persistent_part_evidence_kind::temporal_adjacency,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.88,
                "fixture-timeline",
                "second episode begins shortly after first episode ends",
                {a, b},
            },
            {
                persistent_part_evidence_kind::pitch_trajectory_continuity,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::supports,
                evidence_status::hypothesis,
                0.80,
                "fixture-relative-pitch",
                "relative pitch change is compatible with a continuing line",
                {a, b},
            },
        });
    assert(cross_domain.identity_bearing_support);
    assert(cross_domain.cross_domain_grounded);
    assert(!cross_domain.documentary_grounded);
    assert(close_enough(cross_domain.confidence, 0.86));

    const node_id part_id = add_persistent_part_hypothesis(graph, cross_domain);
    const node* part = graph.find_node(part_id);
    assert(part != nullptr);
    assert(part->kind == node_kind::part);
    assert(part->layer == semantic_layer::musical_performance);
    assert(part->active.has_value());
    assert(part->active->start.tick == 0);
    assert(part->active->end.has_value());
    assert(part->active->end->tick == 220);
    assert(std::get<std::string>(find_attribute(*part, "identity_scope")->value) ==
           "persistent_musical_part");
    assert(std::get<bool>(find_attribute(*part, "cross_domain_grounded")->value));

    const auto grouped = graph.edges_to(part_id, edge_kind::groups_into);
    assert(grouped.size() == 2);
    const auto evidence_edges = graph.edges_to(part_id, edge_kind::derived_from);
    assert(evidence_edges.size() == 6);

    auto conflicted = make_persistent_part_hypothesis(
        0.88,
        {a, c},
        {
            {
                persistent_part_evidence_kind::source_identity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.95,
                "fixture-runtime",
                "same sample source",
                {a, c},
            },
            {
                persistent_part_evidence_kind::temporal_adjacency,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::supports,
                evidence_status::hypothesis,
                0.60,
                "fixture-timeline",
                "episodes are near each other",
                {a, c},
            },
            {
                persistent_part_evidence_kind::simultaneous_conflict,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::counters,
                evidence_status::derived,
                0.95,
                "fixture-overlap",
                "episodes overlap substantially, so a monophonic continuity interpretation conflicts",
                {a, c},
            },
        });
    assert(conflicted.strong_conflict_present);
    assert(close_enough(conflicted.confidence, persistent_part_strong_conflict_confidence_ceiling));

    auto documented = make_persistent_part_hypothesis(
        0.98,
        {a, c},
        {
            {
                persistent_part_evidence_kind::driver_track_identity,
                persistent_part_evidence_origin::driver_execution,
                persistent_part_evidence_polarity::supports,
                evidence_status::exact,
                1.0,
                "fixture-driver-trace",
                "both runtime episodes are scheduled by the same validated driver track",
                {a, c},
            },
            {
                persistent_part_evidence_kind::simultaneous_conflict,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::counters,
                evidence_status::derived,
                0.90,
                "fixture-overlap",
                "physical episodes overlap",
                {a, c},
            },
        });
    assert(documented.documentary_grounded);
    assert(close_enough(documented.confidence, 0.98));

    bool rejected_single_subject = false;
    try {
        (void)make_persistent_part_hypothesis(
            0.8,
            {a},
            {{
                persistent_part_evidence_kind::source_identity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.8,
                "fixture",
                "one observation cannot establish persistence",
                {a},
            }});
    } catch (const std::invalid_argument&) {
        rejected_single_subject = true;
    }
    assert(rejected_single_subject);

    bool rejected_no_support = false;
    try {
        (void)make_persistent_part_hypothesis(
            0.8,
            {a, b},
            {{
                persistent_part_evidence_kind::identity_discontinuity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::counters,
                evidence_status::derived,
                0.8,
                "fixture",
                "counterevidence alone cannot create a part",
                {a, b},
            }});
    } catch (const std::invalid_argument&) {
        rejected_no_support = true;
    }
    assert(rejected_no_support);

    return 0;
}
