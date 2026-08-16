#pragma once

#include "musical_execution_graph.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

// Musical role is deliberately time-local. One persistent part may function as
// accompaniment in one span and become foreground material later without
// changing persistent identity.
enum class musical_part_role : std::uint8_t {
    unresolved = 0,
    melodic_foreground,
    bass_foundation,
    counterline,
    accompaniment,
    inner_voice,
    ostinato,
    sustained_support,
    percussion_pulse,
    doubling_support,
    accent_punctuation,
};

enum class part_role_evidence_kind : std::uint8_t {
    authored_role = 0,
    driver_role,
    external_role,
    harmonic_bass_ownership,
    melodic_motif_prominence,
    phrase_initiation_or_completion,
    counterpoint_independence,
    imitation_or_response,
    rhythmic_ostinato,
    sustained_texture,
    doubling_correspondence,
    percussion_event_identity,
    auditory_salience,
    register_position,
    activity_density,
    timbre_assignment,
    competing_role,
};

enum class part_role_evidence_origin : std::uint8_t {
    authored_program = 0,
    driver_execution,
    musical_analysis,
    synthesis_runtime,
    auditory_analysis,
    external_annotation,
};

enum class part_role_evidence_polarity : std::uint8_t {
    supports = 0,
    counters,
};

struct part_role_evidence {
    part_role_evidence_kind kind = part_role_evidence_kind::register_position;
    part_role_evidence_origin origin = part_role_evidence_origin::musical_analysis;
    part_role_evidence_polarity polarity = part_role_evidence_polarity::supports;
    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
    std::string source;
    std::string detail;
    std::vector<node_id> support_nodes;
};

struct musical_part_role_hypothesis {
    node_id part_id = 0;
    musical_part_role role = musical_part_role::unresolved;
    time_span active{};
    evidence_status status = evidence_status::hypothesis;
    double proposed_confidence = 0.0;
    double confidence = 0.0;
    bool explicit_role_grounded = false;
    bool relationally_grounded = false;
    bool cross_domain_grounded = false;
    bool realization_only = false;
    bool strong_conflict_present = false;
    std::vector<part_role_evidence> evidence;
};

// Epistemic ceilings, not calibrated probabilities.
constexpr double part_role_realization_only_ceiling = 0.45;
constexpr double part_role_no_relational_support_ceiling = 0.64;
constexpr double part_role_single_domain_ceiling = 0.74;
constexpr double part_role_strong_conflict_ceiling = 0.49;

inline const char* to_string(musical_part_role role) noexcept {
    switch (role) {
    case musical_part_role::unresolved:
        return "unresolved";
    case musical_part_role::melodic_foreground:
        return "melodic_foreground";
    case musical_part_role::bass_foundation:
        return "bass_foundation";
    case musical_part_role::counterline:
        return "counterline";
    case musical_part_role::accompaniment:
        return "accompaniment";
    case musical_part_role::inner_voice:
        return "inner_voice";
    case musical_part_role::ostinato:
        return "ostinato";
    case musical_part_role::sustained_support:
        return "sustained_support";
    case musical_part_role::percussion_pulse:
        return "percussion_pulse";
    case musical_part_role::doubling_support:
        return "doubling_support";
    case musical_part_role::accent_punctuation:
        return "accent_punctuation";
    }
    return "unknown";
}

inline const char* to_string(part_role_evidence_kind kind) noexcept {
    switch (kind) {
    case part_role_evidence_kind::authored_role:
        return "authored_role";
    case part_role_evidence_kind::driver_role:
        return "driver_role";
    case part_role_evidence_kind::external_role:
        return "external_role";
    case part_role_evidence_kind::harmonic_bass_ownership:
        return "harmonic_bass_ownership";
    case part_role_evidence_kind::melodic_motif_prominence:
        return "melodic_motif_prominence";
    case part_role_evidence_kind::phrase_initiation_or_completion:
        return "phrase_initiation_or_completion";
    case part_role_evidence_kind::counterpoint_independence:
        return "counterpoint_independence";
    case part_role_evidence_kind::imitation_or_response:
        return "imitation_or_response";
    case part_role_evidence_kind::rhythmic_ostinato:
        return "rhythmic_ostinato";
    case part_role_evidence_kind::sustained_texture:
        return "sustained_texture";
    case part_role_evidence_kind::doubling_correspondence:
        return "doubling_correspondence";
    case part_role_evidence_kind::percussion_event_identity:
        return "percussion_event_identity";
    case part_role_evidence_kind::auditory_salience:
        return "auditory_salience";
    case part_role_evidence_kind::register_position:
        return "register_position";
    case part_role_evidence_kind::activity_density:
        return "activity_density";
    case part_role_evidence_kind::timbre_assignment:
        return "timbre_assignment";
    case part_role_evidence_kind::competing_role:
        return "competing_role";
    }
    return "unknown";
}

inline bool part_role_same_time_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
        first.tick_rate == second.tick_rate &&
        first.loop_iteration == second.loop_iteration;
}

inline bool explicit_part_role_evidence(part_role_evidence_kind kind) noexcept {
    return kind == part_role_evidence_kind::authored_role ||
        kind == part_role_evidence_kind::driver_role ||
        kind == part_role_evidence_kind::external_role;
}

inline bool relational_part_role_evidence(part_role_evidence_kind kind) noexcept {
    switch (kind) {
    case part_role_evidence_kind::harmonic_bass_ownership:
    case part_role_evidence_kind::melodic_motif_prominence:
    case part_role_evidence_kind::phrase_initiation_or_completion:
    case part_role_evidence_kind::counterpoint_independence:
    case part_role_evidence_kind::imitation_or_response:
    case part_role_evidence_kind::rhythmic_ostinato:
    case part_role_evidence_kind::sustained_texture:
    case part_role_evidence_kind::doubling_correspondence:
    case part_role_evidence_kind::percussion_event_identity:
        return true;
    default:
        return false;
    }
}

inline bool realization_only_part_role_evidence(part_role_evidence_kind kind) noexcept {
    return kind == part_role_evidence_kind::register_position ||
        kind == part_role_evidence_kind::activity_density ||
        kind == part_role_evidence_kind::timbre_assignment;
}

inline std::uint8_t part_role_evidence_domain(part_role_evidence_kind kind) noexcept {
    switch (kind) {
    case part_role_evidence_kind::authored_role:
    case part_role_evidence_kind::driver_role:
    case part_role_evidence_kind::external_role:
        return 0;
    case part_role_evidence_kind::harmonic_bass_ownership:
        return 1;
    case part_role_evidence_kind::melodic_motif_prominence:
    case part_role_evidence_kind::phrase_initiation_or_completion:
    case part_role_evidence_kind::counterpoint_independence:
    case part_role_evidence_kind::imitation_or_response:
        return 2;
    case part_role_evidence_kind::rhythmic_ostinato:
    case part_role_evidence_kind::sustained_texture:
    case part_role_evidence_kind::doubling_correspondence:
    case part_role_evidence_kind::percussion_event_identity:
        return 3;
    case part_role_evidence_kind::auditory_salience:
        return 4;
    case part_role_evidence_kind::register_position:
    case part_role_evidence_kind::activity_density:
    case part_role_evidence_kind::timbre_assignment:
        return 5;
    case part_role_evidence_kind::competing_role:
        return 6;
    }
    return 6;
}

inline void validate_part_role_evidence(const part_role_evidence& evidence) {
    if (evidence.confidence < 0.0 || evidence.confidence > 1.0)
        throw std::invalid_argument("part-role evidence confidence must be in [0, 1]");
    if (evidence.source.empty())
        throw std::invalid_argument("part-role evidence requires a non-empty source");
}

inline musical_part_role_hypothesis make_musical_part_role_hypothesis(
    node_id part_id,
    musical_part_role role,
    time_span active,
    double proposed_confidence,
    std::vector<part_role_evidence> evidence) {
    if (part_id == 0)
        throw std::invalid_argument("part-role hypothesis requires a nonzero persistent-part id");
    if (role == musical_part_role::unresolved)
        throw std::invalid_argument("part-role hypothesis must name a candidate role");
    if (!active.end.has_value() ||
        !part_role_same_time_basis(active.start, *active.end) ||
        active.end->tick <= active.start.tick) {
        throw std::invalid_argument("part-role hypothesis requires a positive bounded span in one time basis");
    }
    if (proposed_confidence < 0.0 || proposed_confidence > 1.0)
        throw std::invalid_argument("part-role confidence must be in [0, 1]");
    if (evidence.empty())
        throw std::invalid_argument("part-role hypothesis requires evidence");

    bool has_support = false;
    bool explicit_role = false;
    bool relational = false;
    bool realization_only = true;
    bool strong_conflict = false;
    double strongest_support = 0.0;
    std::set<std::uint8_t> support_domains;

    for (const auto& item : evidence) {
        validate_part_role_evidence(item);
        if (item.polarity == part_role_evidence_polarity::supports) {
            has_support = true;
            strongest_support = std::max(strongest_support, item.confidence);
            support_domains.insert(part_role_evidence_domain(item.kind));
            explicit_role = explicit_role || explicit_part_role_evidence(item.kind);
            relational = relational || relational_part_role_evidence(item.kind);
            realization_only = realization_only && realization_only_part_role_evidence(item.kind);
        } else if (item.kind == part_role_evidence_kind::competing_role &&
                   item.confidence >= 0.80) {
            strong_conflict = true;
        }
    }
    if (!has_support)
        throw std::invalid_argument("part-role hypothesis requires supporting evidence");

    musical_part_role_hypothesis result;
    result.part_id = part_id;
    result.role = role;
    result.active = std::move(active);
    result.proposed_confidence = proposed_confidence;
    result.explicit_role_grounded = explicit_role;
    result.relationally_grounded = relational;
    result.cross_domain_grounded = support_domains.size() >= 2;
    result.realization_only = realization_only;
    result.strong_conflict_present = strong_conflict;
    result.evidence = std::move(evidence);

    // A caller-proposed confidence can never exceed the strongest actual support.
    double confidence = std::min(proposed_confidence, strongest_support);
    if (realization_only && !explicit_role)
        confidence = std::min(confidence, part_role_realization_only_ceiling);
    else if (!relational && !explicit_role)
        confidence = std::min(confidence, part_role_no_relational_support_ceiling);
    else if (!result.cross_domain_grounded && !explicit_role)
        confidence = std::min(confidence, part_role_single_domain_ceiling);
    if (strong_conflict && !explicit_role)
        confidence = std::min(confidence, part_role_strong_conflict_ceiling);

    result.confidence = confidence;
    return result;
}

inline bool is_persistent_musical_part_node(const node& value) noexcept {
    if (value.kind != node_kind::part || value.layer != semantic_layer::musical_performance)
        return false;
    for (const auto& item : value.attributes) {
        if (item.key != "identity_scope")
            continue;
        const auto* text = std::get_if<std::string>(&item.value);
        return text != nullptr && *text == "persistent_musical_part";
    }
    return false;
}

inline node_id add_musical_part_role_hypothesis(
    musical_execution_graph& graph,
    const musical_part_role_hypothesis& hypothesis) {
    const node* part = graph.find_node(hypothesis.part_id);
    if (part == nullptr || !is_persistent_musical_part_node(*part))
        throw std::invalid_argument("part-role hypothesis requires a persistent musical-part node");

    node relation;
    relation.kind = node_kind::musical_relation;
    relation.layer = semantic_layer::musical_structure;
    relation.flow = flow_kind::value;
    relation.label = "time-local musical part role hypothesis";
    relation.active = hypothesis.active;
    relation.attributes.push_back({
        "identity_scope",
        std::string{"time_local_part_role"},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "role",
        std::string{to_string(hypothesis.role)},
        evidence_status::hypothesis,
        hypothesis.confidence,
        "",
    });
    relation.attributes.push_back({
        "part_id",
        static_cast<std::uint64_t>(hypothesis.part_id),
        evidence_status::derived,
        1.0,
        "node_id",
    });
    relation.attributes.push_back({
        "relationally_grounded",
        hypothesis.relationally_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.attributes.push_back({
        "explicit_role_grounded",
        hypothesis.explicit_role_grounded,
        evidence_status::derived,
        1.0,
        "",
    });
    relation.attributes.push_back({
        "realization_only",
        hypothesis.realization_only,
        evidence_status::derived,
        1.0,
        "",
    });

    for (const auto& item : hypothesis.evidence) {
        relation.provenance.push_back({
            item.status,
            item.confidence,
            item.source,
            std::nullopt,
            std::string{to_string(item.kind)} + ": " + item.detail,
        });
    }
    const node_id role_id = graph.add_node(std::move(relation));

    edge reference;
    reference.kind = edge_kind::references;
    reference.from = role_id;
    reference.to = hypothesis.part_id;
    reference.attributes.push_back({
        "reference_scope",
        std::string{"part_role_subject"},
        evidence_status::derived,
        1.0,
        "",
    });
    reference.provenance.push_back({
        evidence_status::hypothesis,
        hypothesis.confidence,
        "part-role-analysis",
        std::nullopt,
        "time-local role hypothesis references this persistent musical part; role is not part identity",
    });
    graph.add_edge(std::move(reference));
    return role_id;
}

} // namespace vgmtooling::model
