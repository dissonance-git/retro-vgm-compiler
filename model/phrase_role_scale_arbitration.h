#pragma once

#include "phrase_role_evidence.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vgmtooling::model {

// Phrase roles live at explicit formal scales. Arbitration therefore preserves
// nested claims instead of flattening them into one winner: a local ending can
// coexist with a larger continuation, prolongation, or delayed resolution.
enum class phrase_role_scale_relation_kind : std::uint8_t {
    unresolved = 0,
    nested_coexistence,
    local_close_inside_global_continuation,
    local_close_inside_global_prolongation,
    local_close_inside_global_delayed_resolution,
};

struct phrase_role_scale_arbitration {
    phrase_role_scale_relation_kind kind =
        phrase_role_scale_relation_kind::unresolved;
    phrase_role_kind inner_role = phrase_role_kind::continuation;
    phrase_role_kind outer_role = phrase_role_kind::continuation;
    phrase_role_formal_scale inner_scale =
        phrase_role_formal_scale::local_phrase;
    phrase_role_formal_scale outer_scale =
        phrase_role_formal_scale::local_phrase;
    time_span inner_scope{};
    time_span outer_scope{};
    bool strict_nesting = false;
    bool cross_scale_coexistence_preserved = false;
    bool both_roles_cross_domain_grounded = false;
    bool role_established = false;
    double confidence = 0.0;
};

inline const char* to_string(
    phrase_role_scale_relation_kind kind) noexcept {
    switch (kind) {
    case phrase_role_scale_relation_kind::unresolved:
        return "unresolved";
    case phrase_role_scale_relation_kind::nested_coexistence:
        return "nested_coexistence";
    case phrase_role_scale_relation_kind::local_close_inside_global_continuation:
        return "local_close_inside_global_continuation";
    case phrase_role_scale_relation_kind::local_close_inside_global_prolongation:
        return "local_close_inside_global_prolongation";
    case phrase_role_scale_relation_kind::local_close_inside_global_delayed_resolution:
        return "local_close_inside_global_delayed_resolution";
    }
    return "unknown";
}

inline bool finite_phrase_role_arbitration_scope(
    const time_span& scope) noexcept {
    return scope.end.has_value() &&
        compatible_phrase_role_time_basis(scope.start, *scope.end) &&
        scope.end->tick >= scope.start.tick;
}

inline bool phrase_role_scope_strictly_contains(
    const time_span& outer,
    const time_span& inner) noexcept {
    if (!finite_phrase_role_arbitration_scope(outer) ||
        !finite_phrase_role_arbitration_scope(inner) ||
        !compatible_phrase_role_time_basis(outer.start, inner.start)) {
        return false;
    }
    const bool contains =
        outer.start.tick <= inner.start.tick &&
        inner.end->tick <= outer.end->tick;
    const bool same =
        outer.start.tick == inner.start.tick &&
        outer.end->tick == inner.end->tick;
    return contains && !same;
}

inline bool phrase_role_scale_is_broader(
    phrase_role_formal_scale outer,
    phrase_role_formal_scale inner) noexcept {
    return static_cast<std::uint8_t>(outer) >
        static_cast<std::uint8_t>(inner);
}

inline phrase_role_scale_relation_kind classify_phrase_role_scale_relation(
    phrase_role_kind inner,
    phrase_role_kind outer) noexcept {
    if (inner != phrase_role_kind::ending)
        return phrase_role_scale_relation_kind::nested_coexistence;
    switch (outer) {
    case phrase_role_kind::continuation:
        return phrase_role_scale_relation_kind::
            local_close_inside_global_continuation;
    case phrase_role_kind::prolongation:
        return phrase_role_scale_relation_kind::
            local_close_inside_global_prolongation;
    case phrase_role_kind::delayed_resolution:
        return phrase_role_scale_relation_kind::
            local_close_inside_global_delayed_resolution;
    default:
        return phrase_role_scale_relation_kind::nested_coexistence;
    }
}

inline phrase_role_scale_arbitration infer_phrase_role_scale_arbitration(
    const phrase_role_hypothesis& inner,
    const phrase_role_hypothesis& outer) {
    validate_phrase_role_scope(inner.scope);
    validate_phrase_role_scope(outer.scope);

    if (!finite_phrase_role_arbitration_scope(inner.scope) ||
        !finite_phrase_role_arbitration_scope(outer.scope)) {
        throw std::invalid_argument(
            "multi-scale phrase-role arbitration requires finite explicit scopes");
    }
    if (!phrase_role_scale_is_broader(
            outer.formal_scale, inner.formal_scale)) {
        throw std::invalid_argument(
            "outer phrase role must use a strictly broader formal scale");
    }
    if (!phrase_role_scope_strictly_contains(
            outer.scope, inner.scope)) {
        throw std::invalid_argument(
            "multi-scale phrase-role arbitration requires strict temporal nesting");
    }

    phrase_role_scale_arbitration result;
    result.kind = classify_phrase_role_scale_relation(
        inner.role, outer.role);
    result.inner_role = inner.role;
    result.outer_role = outer.role;
    result.inner_scale = inner.formal_scale;
    result.outer_scale = outer.formal_scale;
    result.inner_scope = inner.scope;
    result.outer_scope = outer.scope;
    result.strict_nesting = true;
    result.cross_scale_coexistence_preserved = true;
    result.both_roles_cross_domain_grounded =
        inner.cross_domain_grounded &&
        outer.cross_domain_grounded;
    result.role_established = false;
    result.confidence = std::min(
        inner.confidence,
        outer.confidence);
    return result;
}

inline phrase_role_hypothesis
make_nested_local_close_inside_global_continuation_hypothesis(
    const phrase_role_scale_arbitration& arbitration,
    const phrase_role_hypothesis& local_ending,
    const phrase_role_hypothesis& global_continuation,
    double proposed_confidence = 0.95) {
    if (arbitration.kind != phrase_role_scale_relation_kind::
            local_close_inside_global_continuation ||
        !arbitration.strict_nesting ||
        !arbitration.cross_scale_coexistence_preserved ||
        !arbitration.both_roles_cross_domain_grounded) {
        throw std::invalid_argument(
            "nested local-close composite requires grounded local ending and broader continuation");
    }
    if (local_ending.role != phrase_role_kind::ending ||
        global_continuation.role != phrase_role_kind::continuation ||
        !same_phrase_role_scope(
            local_ending.scope, arbitration.inner_scope) ||
        !same_phrase_role_scope(
            global_continuation.scope, arbitration.outer_scope) ||
        local_ending.formal_scale != arbitration.inner_scale ||
        global_continuation.formal_scale != arbitration.outer_scale) {
        throw std::invalid_argument(
            "nested local-close composite inputs do not match the arbitration");
    }

    std::vector<phrase_role_evidence> evidence;
    evidence.reserve(
        local_ending.evidence.size() +
        global_continuation.evidence.size());

    for (auto item : local_ending.evidence) {
        item.role =
            phrase_role_kind::nested_local_close_inside_global_continuation;
        item.detail =
            "local-close support preserved inside larger continuation: " +
            item.detail;
        evidence.push_back(std::move(item));
    }
    for (auto item : global_continuation.evidence) {
        item.role =
            phrase_role_kind::nested_local_close_inside_global_continuation;
        item.detail =
            "larger-continuation support preserved around local close: " +
            item.detail;
        evidence.push_back(std::move(item));
    }
    if (evidence.empty())
        throw std::invalid_argument(
            "nested local-close composite requires retained evidence");

    const double bounded_proposal = std::min({
        proposed_confidence,
        local_ending.confidence,
        global_continuation.confidence,
    });
    return make_phrase_role_hypothesis(
        phrase_role_kind::nested_local_close_inside_global_continuation,
        global_continuation.scope,
        global_continuation.formal_scale,
        bounded_proposal,
        std::move(evidence));
}

} // namespace vgmtooling::model
