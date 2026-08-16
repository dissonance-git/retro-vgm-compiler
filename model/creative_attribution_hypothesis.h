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

// Creative attribution is role-scoped by construction. A person may occupy
// several roles historically, but evidence for one role must not silently
// become evidence for another.
enum class creative_attribution_role : std::uint8_t {
    composer = 0,
    arranger_programmer,
    driver_toolchain,
    patch_sample_designer,
};

enum class creative_attribution_evidence_kind : std::uint8_t {
    composition_structure = 0,
    arrangement_execution,
    driver_toolchain_signature,
    patch_sample_signature,
    // A higher-order recurring creator rule that may integrate symbolic,
    // arrangement, synthesis, performance, auditory, and soundtrack evidence.
    // Role scope determines what the rule is allowed to support.
    creator_grammar,
    documentary_role_credit,
    external_recollection,
    version_lineage,
    metadata_label,
    contradiction,
};

enum class creative_attribution_polarity : std::uint8_t {
    supports = 0,
    counters,
};

struct creative_attribution_evidence {
    creative_attribution_evidence_kind kind =
        creative_attribution_evidence_kind::composition_structure;

    // The historical role that this evidence is actually allowed to inform.
    // metadata_label is intentionally non-role-bearing and should use the
    // target role only for bookkeeping, never as proof.
    creative_attribution_role role_scope = creative_attribution_role::composer;
    creative_attribution_polarity polarity = creative_attribution_polarity::supports;
    evidence_status status = evidence_status::hypothesis;
    double confidence = 0.0;
    std::string source;
    std::string detail;
};

struct creative_attribution_hypothesis {
    std::string candidate;
    creative_attribution_role role = creative_attribution_role::composer;
    evidence_status status = evidence_status::hypothesis;
    double proposed_confidence = 0.0;
    double confidence = 0.0;
    bool role_specific_support = false;
    bool documentary_grounded = false;
    bool cross_domain_grounded = false;
    bool metadata_only = false;
    bool strong_conflict_present = false;
    std::vector<creative_attribution_evidence> evidence;
};

// Epistemic ceilings, not calibrated probabilities.
constexpr double creative_attribution_metadata_only_ceiling = 0.25;
constexpr double creative_attribution_no_role_support_ceiling = 0.40;
constexpr double creative_attribution_single_domain_ceiling = 0.74;
constexpr double creative_attribution_strong_conflict_ceiling = 0.49;

inline const char* to_string(creative_attribution_role role) noexcept {
    switch (role) {
    case creative_attribution_role::composer:
        return "composer";
    case creative_attribution_role::arranger_programmer:
        return "arranger_programmer";
    case creative_attribution_role::driver_toolchain:
        return "driver_toolchain";
    case creative_attribution_role::patch_sample_designer:
        return "patch_sample_designer";
    }
    return "unknown";
}

inline const char* to_string(creative_attribution_evidence_kind kind) noexcept {
    switch (kind) {
    case creative_attribution_evidence_kind::composition_structure:
        return "composition_structure";
    case creative_attribution_evidence_kind::arrangement_execution:
        return "arrangement_execution";
    case creative_attribution_evidence_kind::driver_toolchain_signature:
        return "driver_toolchain_signature";
    case creative_attribution_evidence_kind::patch_sample_signature:
        return "patch_sample_signature";
    case creative_attribution_evidence_kind::creator_grammar:
        return "creator_grammar";
    case creative_attribution_evidence_kind::documentary_role_credit:
        return "documentary_role_credit";
    case creative_attribution_evidence_kind::external_recollection:
        return "external_recollection";
    case creative_attribution_evidence_kind::version_lineage:
        return "version_lineage";
    case creative_attribution_evidence_kind::metadata_label:
        return "metadata_label";
    case creative_attribution_evidence_kind::contradiction:
        return "contradiction";
    }
    return "unknown";
}

inline bool creative_evidence_is_metadata(
    creative_attribution_evidence_kind kind) noexcept {
    return kind == creative_attribution_evidence_kind::metadata_label;
}

inline bool creative_evidence_is_documentary(
    const creative_attribution_evidence& evidence) noexcept {
    return evidence.kind == creative_attribution_evidence_kind::documentary_role_credit;
}

inline std::uint8_t creative_attribution_domain(
    creative_attribution_evidence_kind kind) noexcept {
    switch (kind) {
    case creative_attribution_evidence_kind::composition_structure:
        return 0;
    case creative_attribution_evidence_kind::arrangement_execution:
        return 1;
    case creative_attribution_evidence_kind::driver_toolchain_signature:
        return 2;
    case creative_attribution_evidence_kind::patch_sample_signature:
        return 3;
    case creative_attribution_evidence_kind::creator_grammar:
        return 4;
    case creative_attribution_evidence_kind::documentary_role_credit:
    case creative_attribution_evidence_kind::external_recollection:
        return 5;
    case creative_attribution_evidence_kind::version_lineage:
        return 6;
    case creative_attribution_evidence_kind::metadata_label:
        return 7;
    case creative_attribution_evidence_kind::contradiction:
        return 8;
    }
    return 8;
}

inline void validate_creative_attribution_evidence(
    const creative_attribution_evidence& evidence) {
    if (evidence.confidence < 0.0 || evidence.confidence > 1.0)
        throw std::invalid_argument("creative-attribution evidence confidence must be in [0, 1]");
    if (evidence.source.empty())
        throw std::invalid_argument("creative-attribution evidence requires a source");
}

inline creative_attribution_hypothesis make_creative_attribution_hypothesis(
    std::string candidate,
    creative_attribution_role role,
    double proposed_confidence,
    std::vector<creative_attribution_evidence> evidence) {
    if (candidate.empty())
        throw std::invalid_argument("creative attribution requires a candidate");
    if (proposed_confidence < 0.0 || proposed_confidence > 1.0)
        throw std::invalid_argument("creative-attribution confidence must be in [0, 1]");
    if (evidence.empty())
        throw std::invalid_argument("creative attribution requires evidence");

    bool has_role_support = false;
    bool documentary = false;
    bool metadata_support = false;
    bool non_metadata_role_support = false;
    bool strong_conflict = false;
    std::set<std::uint8_t> support_domains;

    for (const auto& item : evidence) {
        validate_creative_attribution_evidence(item);

        if (item.polarity == creative_attribution_polarity::supports) {
            if (creative_evidence_is_metadata(item.kind)) {
                metadata_support = true;
                continue;
            }

            // The role-scope check is the core firewall. A strong programming
            // fingerprint does not count as composer support unless separate
            // evidence explicitly establishes that historical role.
            if (item.role_scope != role)
                continue;

            has_role_support = true;
            non_metadata_role_support = true;
            support_domains.insert(creative_attribution_domain(item.kind));
            if (creative_evidence_is_documentary(item) &&
                item.status == evidence_status::exact &&
                item.confidence >= 0.90) {
                documentary = true;
            }
        } else if (
            item.role_scope == role && item.confidence >= 0.80 &&
            item.kind == creative_attribution_evidence_kind::contradiction) {
            strong_conflict = true;
        }
    }

    creative_attribution_hypothesis result;
    result.candidate = std::move(candidate);
    result.role = role;
    result.proposed_confidence = proposed_confidence;
    result.role_specific_support = has_role_support;
    result.documentary_grounded = documentary;
    result.cross_domain_grounded = support_domains.size() >= 2;
    result.metadata_only = metadata_support && !non_metadata_role_support;
    result.strong_conflict_present = strong_conflict;
    result.evidence = std::move(evidence);

    double confidence = proposed_confidence;
    if (result.metadata_only)
        confidence = std::min(confidence, creative_attribution_metadata_only_ceiling);
    else if (!has_role_support)
        confidence = std::min(confidence, creative_attribution_no_role_support_ceiling);
    else if (!result.cross_domain_grounded && !documentary)
        confidence = std::min(confidence, creative_attribution_single_domain_ceiling);

    // Exact, role-specific documentary evidence can coexist with technical
    // conflict without being automatically erased. Otherwise a strong direct
    // contradiction keeps the hypothesis below a strong-attribution threshold.
    if (strong_conflict && !documentary)
        confidence = std::min(confidence, creative_attribution_strong_conflict_ceiling);

    result.confidence = confidence;
    return result;
}

} // namespace vgmtooling::model
