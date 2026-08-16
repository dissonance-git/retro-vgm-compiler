#pragma once

#include "musical_part_role_hypothesis.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace vgmtooling::model {

// A realization-role deployment is a musical statement about how a grounded
// persistent part is realized, not a statement that a physical channel itself
// has a role. This is the bridge needed for hypotheses such as "this creator
// repeatedly assigns genuine melodic material to PSG" without collapsing the
// exact PSG programming into composition evidence.
enum class realization_role_deployment_kind : std::uint8_t {
    unresolved = 0,
    independent_part,
    doubling_support,
    percussion_or_texture,
};

struct realization_role_deployment_hypothesis {
    node_id part_id = 0;
    musical_part_role role = musical_part_role::unresolved;
    realization_role_deployment_kind kind =
        realization_role_deployment_kind::unresolved;
    std::string realization_family;
    time_span active{};
    double activity_density = 0.0;
    bool persistent_part_grounded = false;
    bool role_grounded = false;
    bool cross_domain_role_grounded = false;
    bool realization_grounded = false;
    double confidence = 0.0;
};

constexpr double realization_role_minimum_role_confidence = 0.60;
constexpr double realization_role_single_domain_ceiling = 0.74;
constexpr double realization_role_unresolved_independence_ceiling = 0.49;

inline const char* to_string(realization_role_deployment_kind kind) noexcept {
    switch (kind) {
    case realization_role_deployment_kind::unresolved:
        return "unresolved";
    case realization_role_deployment_kind::independent_part:
        return "independent_part";
    case realization_role_deployment_kind::doubling_support:
        return "doubling_support";
    case realization_role_deployment_kind::percussion_or_texture:
        return "percussion_or_texture";
    }
    return "unknown";
}

inline realization_role_deployment_hypothesis
make_realization_role_deployment_hypothesis(
    const musical_part_role_hypothesis& role,
    std::string realization_family,
    realization_role_deployment_kind kind,
    double activity_density,
    double realization_confidence = 1.0) {
    if (role.part_id == 0 || role.role == musical_part_role::unresolved)
        throw std::invalid_argument(
            "realization-role deployment requires a resolved persistent-part role");
    if (realization_family.empty())
        throw std::invalid_argument(
            "realization-role deployment requires a realization family");
    if (kind == realization_role_deployment_kind::unresolved)
        throw std::invalid_argument(
            "realization-role deployment requires a resolved deployment kind");
    if (!std::isfinite(activity_density) ||
        activity_density < 0.0 || activity_density > 1.0) {
        throw std::invalid_argument(
            "realization-role deployment activity density must be in [0, 1]");
    }
    if (!std::isfinite(realization_confidence) ||
        realization_confidence < 0.0 || realization_confidence > 1.0) {
        throw std::invalid_argument(
            "realization-role deployment confidence must be in [0, 1]");
    }

    realization_role_deployment_hypothesis result;
    result.part_id = role.part_id;
    result.role = role.role;
    result.kind = kind;
    result.realization_family = std::move(realization_family);
    result.active = role.active;
    result.activity_density = activity_density;
    result.persistent_part_grounded = true;
    result.role_grounded =
        role.confidence >= realization_role_minimum_role_confidence &&
        (role.relationally_grounded || role.explicit_role_grounded);
    result.cross_domain_role_grounded = role.cross_domain_grounded;
    result.realization_grounded = realization_confidence >= 0.60;

    double confidence = std::min(role.confidence, realization_confidence);
    if (!result.role_grounded || !result.realization_grounded)
        confidence = std::min(
            confidence,
            realization_role_unresolved_independence_ceiling);
    else if (!result.cross_domain_role_grounded && !role.explicit_role_grounded)
        confidence = std::min(
            confidence,
            realization_role_single_domain_ceiling);

    // Calling a line independent requires a grounded role on a persistent part.
    // The caller may not promote raw physical-channel noncoincidence directly.
    if (kind == realization_role_deployment_kind::independent_part &&
        !result.role_grounded) {
        confidence = std::min(
            confidence,
            realization_role_unresolved_independence_ceiling);
    }

    result.confidence = confidence;
    return result;
}

inline bool realization_role_deployment_creator_eligible(
    const realization_role_deployment_hypothesis& value) noexcept {
    return value.persistent_part_grounded &&
        value.role_grounded &&
        value.realization_grounded &&
        value.confidence >= realization_role_minimum_role_confidence;
}

} // namespace vgmtooling::model
