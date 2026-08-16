#pragma once

#include "musical_part_role_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace vgmtooling::model {

// A realization identity is intentionally opaque and representation-scoped.
// Examples may be a YM2612 program fingerprint, exact BRR sample version,
// tracker instrument, MIDI program/bank tuple, or another source-native timbre
// identity. Equal strings across different bases do not imply equivalence.
struct orchestration_realization {
    std::string basis;
    std::string identity;
    evidence_status status = evidence_status::derived;
    double confidence = 0.0;
    std::string source;
};

struct part_orchestration_state {
    node_id part_id = 0;
    musical_part_role role = musical_part_role::unresolved;
    time_span active{};
    double role_confidence = 0.0;
    std::optional<orchestration_realization> realization{};
    std::optional<double> register_coordinate{};
    std::string register_basis;
    std::optional<double> activity_density{};
    double confidence = 0.0;
};

enum class orchestration_transition_kind : std::uint8_t {
    unresolved = 0,
    stable_assignment,
    timbral_recoloring,
    registral_revoicing,
    role_change,
    role_transfer,
    compound_reorchestration,
};

struct orchestration_transition_hypothesis {
    orchestration_transition_kind kind = orchestration_transition_kind::unresolved;
    node_id first_part_id = 0;
    node_id second_part_id = 0;
    musical_part_role first_role = musical_part_role::unresolved;
    musical_part_role second_role = musical_part_role::unresolved;
    time_coordinate transition_time{};
    bool persistent_part_preserved = false;
    bool role_preserved = false;
    bool realization_comparable = false;
    bool timbre_changed = false;
    bool register_comparable = false;
    double register_shift = 0.0;
    bool activity_density_compared = false;
    double activity_density_delta = 0.0;
    bool musical_material_continuity_grounded = false;
    double musical_material_continuity_confidence = 0.0;
    double confidence = 0.0;
};

constexpr double ungrounded_cross_part_orchestration_ceiling = 0.49;
constexpr double incomparable_realization_orchestration_ceiling = 0.64;
constexpr double orchestration_material_transfer_threshold = 0.75;

inline const char* to_string(orchestration_transition_kind kind) noexcept {
    switch (kind) {
    case orchestration_transition_kind::unresolved:
        return "unresolved";
    case orchestration_transition_kind::stable_assignment:
        return "stable_assignment";
    case orchestration_transition_kind::timbral_recoloring:
        return "timbral_recoloring";
    case orchestration_transition_kind::registral_revoicing:
        return "registral_revoicing";
    case orchestration_transition_kind::role_change:
        return "role_change";
    case orchestration_transition_kind::role_transfer:
        return "role_transfer";
    case orchestration_transition_kind::compound_reorchestration:
        return "compound_reorchestration";
    }
    return "unknown";
}

inline void validate_orchestration_realization(
    const orchestration_realization& realization) {
    if (realization.basis.empty() || realization.identity.empty())
        throw std::invalid_argument("orchestration realization requires basis and identity");
    if (realization.confidence < 0.0 || realization.confidence > 1.0)
        throw std::invalid_argument("orchestration realization confidence must be in [0, 1]");
    if (realization.source.empty())
        throw std::invalid_argument("orchestration realization requires a source");
}

inline part_orchestration_state make_part_orchestration_state(
    const musical_part_role_hypothesis& role,
    std::optional<orchestration_realization> realization = std::nullopt,
    std::optional<double> register_coordinate = std::nullopt,
    std::string register_basis = {},
    std::optional<double> activity_density = std::nullopt) {
    if (role.part_id == 0 || role.role == musical_part_role::unresolved)
        throw std::invalid_argument("orchestration state requires a resolved part-role hypothesis");
    if (register_coordinate.has_value()) {
        if (!std::isfinite(*register_coordinate) || register_basis.empty())
            throw std::invalid_argument("orchestration register coordinate requires a finite value and basis");
    } else if (!register_basis.empty()) {
        throw std::invalid_argument("orchestration register basis requires a coordinate");
    }
    if (activity_density.has_value() &&
        (!std::isfinite(*activity_density) || *activity_density < 0.0 || *activity_density > 1.0)) {
        throw std::invalid_argument("orchestration activity density must be in [0, 1]");
    }

    part_orchestration_state result;
    result.part_id = role.part_id;
    result.role = role.role;
    result.active = role.active;
    result.role_confidence = role.confidence;
    result.realization = std::move(realization);
    result.register_coordinate = register_coordinate;
    result.register_basis = std::move(register_basis);
    result.activity_density = activity_density;
    result.confidence = role.confidence;
    if (result.realization.has_value()) {
        validate_orchestration_realization(*result.realization);
        result.confidence = std::min(result.confidence, result.realization->confidence);
    }
    return result;
}

inline orchestration_transition_hypothesis infer_orchestration_transition(
    const part_orchestration_state& first,
    const part_orchestration_state& second,
    std::optional<double> musical_material_continuity_confidence = std::nullopt) {
    if (!part_role_same_time_basis(first.active.start, second.active.start))
        throw std::invalid_argument("orchestration transition requires one compatible time basis");
    if (!first.active.end.has_value() || !second.active.end.has_value())
        throw std::invalid_argument("orchestration transition requires bounded states");
    if (second.active.start.tick < first.active.start.tick)
        throw std::invalid_argument("orchestration transition requires chronological states");
    if (musical_material_continuity_confidence.has_value() &&
        (!std::isfinite(*musical_material_continuity_confidence) ||
         *musical_material_continuity_confidence < 0.0 ||
         *musical_material_continuity_confidence > 1.0)) {
        throw std::invalid_argument("musical-material continuity confidence must be in [0, 1]");
    }

    orchestration_transition_hypothesis result;
    result.first_part_id = first.part_id;
    result.second_part_id = second.part_id;
    result.first_role = first.role;
    result.second_role = second.role;
    result.transition_time = second.active.start;
    result.persistent_part_preserved = first.part_id == second.part_id;
    result.role_preserved = first.role == second.role;
    result.confidence = std::min(first.confidence, second.confidence);

    const bool both_have_realization =
        first.realization.has_value() && second.realization.has_value();
    const bool realization_basis_conflict = both_have_realization &&
        first.realization->basis != second.realization->basis;
    if (both_have_realization && !realization_basis_conflict) {
        result.realization_comparable = true;
        result.timbre_changed = first.realization->identity != second.realization->identity;
    }

    if (first.register_coordinate.has_value() && second.register_coordinate.has_value() &&
        first.register_basis == second.register_basis) {
        result.register_comparable = true;
        result.register_shift = *second.register_coordinate - *first.register_coordinate;
    }

    if (first.activity_density.has_value() && second.activity_density.has_value()) {
        result.activity_density_compared = true;
        result.activity_density_delta = *second.activity_density - *first.activity_density;
    }

    if (musical_material_continuity_confidence.has_value() &&
        *musical_material_continuity_confidence >= orchestration_material_transfer_threshold) {
        result.musical_material_continuity_grounded = true;
        result.musical_material_continuity_confidence = *musical_material_continuity_confidence;
        result.confidence = std::min(result.confidence, *musical_material_continuity_confidence);
    }

    const bool register_changed = result.register_comparable &&
        std::fabs(result.register_shift) > 1e-9;

    if (!result.persistent_part_preserved) {
        if (result.role_preserved && result.musical_material_continuity_grounded) {
            result.kind = orchestration_transition_kind::role_transfer;
        } else {
            result.kind = orchestration_transition_kind::unresolved;
            result.confidence = std::min(
                result.confidence,
                ungrounded_cross_part_orchestration_ceiling);
        }
        return result;
    }

    if (!result.role_preserved) {
        result.kind = (result.timbre_changed || register_changed)
            ? orchestration_transition_kind::compound_reorchestration
            : orchestration_transition_kind::role_change;
    } else if (result.timbre_changed && register_changed) {
        result.kind = orchestration_transition_kind::compound_reorchestration;
    } else if (result.timbre_changed) {
        result.kind = orchestration_transition_kind::timbral_recoloring;
    } else if (register_changed) {
        result.kind = orchestration_transition_kind::registral_revoicing;
    } else if (realization_basis_conflict) {
        result.kind = orchestration_transition_kind::unresolved;
        result.confidence = std::min(
            result.confidence,
            incomparable_realization_orchestration_ceiling);
    } else {
        result.kind = orchestration_transition_kind::stable_assignment;
    }
    return result;
}

} // namespace vgmtooling::model
