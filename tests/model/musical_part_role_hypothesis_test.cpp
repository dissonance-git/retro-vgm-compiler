#include "model/musical_part_role_hypothesis.h"

#include <cassert>
#include <cmath>
#include <string>

using namespace vgmtooling::model;

namespace {

time_span span(std::int64_t begin, std::int64_t end) {
    return time_span{
        time_coordinate{time_domain::source, begin, 44100, 0},
        time_coordinate{time_domain::source, end, 44100, 0},
    };
}

node_id add_part(musical_execution_graph& graph) {
    node part;
    part.kind = node_kind::part;
    part.layer = semantic_layer::musical_performance;
    part.flow = flow_kind::stream;
    part.label = "persistent musical part";
    part.attributes.push_back({
        "identity_scope",
        std::string{"persistent_musical_part"},
        evidence_status::hypothesis,
        0.90,
        "",
    });
    return graph.add_node(std::move(part));
}

part_role_evidence evidence(
    part_role_evidence_kind kind,
    double confidence,
    std::string detail,
    part_role_evidence_polarity polarity = part_role_evidence_polarity::supports,
    part_role_evidence_origin origin = part_role_evidence_origin::musical_analysis) {
    return part_role_evidence{
        kind,
        origin,
        polarity,
        evidence_status::hypothesis,
        confidence,
        "role-test",
        std::move(detail),
        {},
    };
}

} // namespace

int main() {
    // Patch/timbre/register evidence is useful orchestration context, but it
    // cannot establish a strong musical role by itself.
    const auto timbre_only = make_musical_part_role_hypothesis(
        1,
        musical_part_role::melodic_foreground,
        span(0, 100),
        0.99,
        {
            evidence(part_role_evidence_kind::timbre_assignment, 0.98, "bright FM program"),
            evidence(part_role_evidence_kind::register_position, 0.96, "upper register"),
            evidence(part_role_evidence_kind::activity_density, 0.95, "dense activity"),
        });
    assert(timbre_only.realization_only);
    assert(!timbre_only.relationally_grounded);
    assert(std::fabs(timbre_only.confidence - part_role_realization_only_ceiling) < 1e-12);

    // Independent harmonic and perceptual/structural evidence can strongly
    // support a bass-foundation role without relying on physical channel.
    const auto bass = make_musical_part_role_hypothesis(
        7,
        musical_part_role::bass_foundation,
        span(100, 300),
        0.88,
        {
            evidence(
                part_role_evidence_kind::harmonic_bass_ownership,
                0.92,
                "same persistent part owns the lowest voice across harmonic change"),
            evidence(
                part_role_evidence_kind::rhythmic_ostinato,
                0.86,
                "repeating foundation rhythm supports the harmonic span"),
        });
    assert(bass.relationally_grounded);
    assert(bass.cross_domain_grounded);
    assert(!bass.realization_only);
    assert(std::fabs(bass.confidence - 0.88) < 1e-12);

    // A strong competing role keeps a non-explicit analytical claim below the
    // strong threshold rather than forcing an arbitrary winner.
    const auto conflicted = make_musical_part_role_hypothesis(
        7,
        musical_part_role::counterline,
        span(100, 300),
        0.90,
        {
            evidence(
                part_role_evidence_kind::counterpoint_independence,
                0.91,
                "line moves independently against foreground material"),
            evidence(
                part_role_evidence_kind::competing_role,
                0.87,
                "same evidence also supports melodic foreground",
                part_role_evidence_polarity::counters),
        });
    assert(conflicted.strong_conflict_present);
    assert(std::fabs(conflicted.confidence - part_role_strong_conflict_ceiling) < 1e-12);

    // An explicit authored role can outrank a generic analytical conflict, but
    // still cannot outrun the explicit source evidence itself.
    const auto authored = make_musical_part_role_hypothesis(
        9,
        musical_part_role::melodic_foreground,
        span(0, 200),
        0.99,
        {
            evidence(
                part_role_evidence_kind::authored_role,
                0.96,
                "validated source labels the logical part as melody",
                part_role_evidence_polarity::supports,
                part_role_evidence_origin::authored_program),
            evidence(
                part_role_evidence_kind::competing_role,
                0.90,
                "register alone resembles accompaniment",
                part_role_evidence_polarity::counters),
        });
    assert(authored.explicit_role_grounded);
    assert(std::fabs(authored.confidence - 0.96) < 1e-12);

    // Role is not persistent-part identity. One continuing part can change jobs
    // across time while both role hypotheses reference the same part node.
    musical_execution_graph graph;
    const node_id part_id = add_part(graph);
    const auto accompaniment = make_musical_part_role_hypothesis(
        part_id,
        musical_part_role::accompaniment,
        span(0, 100),
        0.82,
        {
            evidence(part_role_evidence_kind::rhythmic_ostinato, 0.84, "repeating accompaniment cell"),
            evidence(part_role_evidence_kind::sustained_texture, 0.82, "supports foreground line"),
        });
    const auto foreground = make_musical_part_role_hypothesis(
        part_id,
        musical_part_role::melodic_foreground,
        span(100, 200),
        0.86,
        {
            evidence(part_role_evidence_kind::melodic_motif_prominence, 0.90, "takes over primary motif"),
            evidence(part_role_evidence_kind::phrase_initiation_or_completion, 0.86, "initiates new phrase"),
        });

    const node_id accompaniment_id = add_musical_part_role_hypothesis(graph, accompaniment);
    const node_id foreground_id = add_musical_part_role_hypothesis(graph, foreground);
    assert(accompaniment_id != foreground_id);
    assert(graph.edges_from(accompaniment_id, edge_kind::references).size() == 1);
    assert(graph.edges_from(foreground_id, edge_kind::references).size() == 1);
    assert(graph.edges_from(accompaniment_id, edge_kind::references)[0]->to == part_id);
    assert(graph.edges_from(foreground_id, edge_kind::references)[0]->to == part_id);
    assert(graph.edges_from(part_id, edge_kind::groups_into).empty());

    return 0;
}
