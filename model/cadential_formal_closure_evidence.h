#pragma once

#include "phrase_boundary_consensus.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>

namespace vgmtooling::model {

// Phrase boundaries and cadences are not the same thing. This layer asks a
// narrower question: does a grounded phrase boundary also carry independent
// evidence that some musical process actually completed there?
//
// Crucially, cadence_or_resolution evidence is never allowed to establish this
// closure witness. Otherwise cadence classification could become circular:
// "this is a cadence because a cadence detector helped prove formal closure."
enum class cadential_formal_closure_kind : std::uint8_t {
    unresolved = 0,
    phrase_boundary_only,
    completion_aligned_phrase_end_candidate,
};

struct cadential_formal_closure_evidence {
    cadential_formal_closure_kind kind = cadential_formal_closure_kind::unresolved;
    time_coordinate boundary_time{};
    bool phrase_boundary_grounded = false;
    bool cross_part_phrase_grounded = false;
    bool authored_boundary_grounded = false;
    bool noncadential_completion_grounded = false;
    bool cadence_derived_support_present = false;
    std::size_t completion_observations = 0;
    std::size_t completion_support_domains = 0;
    bool closure_candidate_resolved = false;
    bool formal_closure_established = false;
    double phrase_boundary_confidence = 0.0;
    double completion_confidence = 0.0;
    double confidence = 0.0;
};

constexpr double cadential_phrase_boundary_only_ceiling = 0.60;
constexpr double cadential_formal_closure_candidate_ceiling = 0.82;

inline bool valid_cadential_formal_confidence(double value) noexcept {
    return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

inline bool noncadential_phrase_completion_kind(
    phrase_boundary_evidence_kind kind) noexcept {
    return kind == phrase_boundary_evidence_kind::motif_completion ||
        kind == phrase_boundary_evidence_kind::repeated_motif_alignment;
}

inline cadential_formal_closure_evidence infer_cadential_formal_closure_evidence(
    const phrase_boundary_consensus& boundary) {
    if (!valid_cadential_formal_confidence(boundary.confidence))
        throw std::invalid_argument(
            "cadential formal closure requires phrase-boundary confidence in [0, 1]");
    if (boundary.supporting_parts.empty() || boundary.part_hypotheses.empty())
        throw std::invalid_argument(
            "cadential formal closure requires retained persistent-part phrase evidence");

    std::set<node_id> unique_parts;
    for (node_id part_id : boundary.supporting_parts) {
        if (part_id == 0)
            throw std::invalid_argument(
                "cadential formal closure requires nonzero persistent-part ids");
        unique_parts.insert(part_id);
    }
    if (unique_parts.size() != boundary.supporting_parts.size())
        throw std::invalid_argument(
            "cadential formal closure requires unique persistent-part ids");
    if (boundary.cross_part_grounded != (unique_parts.size() >= 2))
        throw std::invalid_argument(
            "cadential formal closure cross-part flag disagrees with retained parts");

    cadential_formal_closure_evidence result;
    result.boundary_time = boundary.representative;
    result.phrase_boundary_grounded = true;
    result.cross_part_phrase_grounded = boundary.cross_part_grounded;
    result.authored_boundary_grounded = boundary.authored_grounded;
    result.phrase_boundary_confidence = boundary.confidence;
    result.kind = cadential_formal_closure_kind::phrase_boundary_only;
    result.confidence = std::min(
        boundary.confidence,
        cadential_phrase_boundary_only_ceiling);

    std::set<phrase_boundary_evidence_origin> completion_domains;
    double strongest_completion = 0.0;

    for (const auto& hypothesis : boundary.part_hypotheses) {
        if (!valid_cadential_formal_confidence(hypothesis.confidence))
            throw std::invalid_argument(
                "cadential formal closure requires valid retained phrase confidence");
        if (!compatible_phrase_boundary_time_basis(
                boundary.representative,
                hypothesis.boundary)) {
            throw std::invalid_argument(
                "cadential formal closure requires one compatible phrase time basis");
        }

        for (const auto& item : hypothesis.evidence) {
            validate_phrase_boundary_evidence(item);
            if (item.polarity != phrase_boundary_evidence_polarity::supports)
                continue;

            if (item.kind == phrase_boundary_evidence_kind::cadence_or_resolution) {
                result.cadence_derived_support_present = true;
                continue;
            }
            if (!noncadential_phrase_completion_kind(item.kind))
                continue;

            ++result.completion_observations;
            completion_domains.insert(item.origin);
            strongest_completion = std::max(strongest_completion, item.confidence);
        }
    }

    result.completion_support_domains = completion_domains.size();
    result.completion_confidence = strongest_completion;
    result.noncadential_completion_grounded = result.completion_observations > 0;

    // A local completion cue is not enough for a global cadence context. The
    // phrase boundary itself must already be cross-part or explicitly authored.
    if (result.noncadential_completion_grounded &&
        (result.cross_part_phrase_grounded || result.authored_boundary_grounded)) {
        result.kind =
            cadential_formal_closure_kind::completion_aligned_phrase_end_candidate;
        result.closure_candidate_resolved = true;
        result.confidence = std::min({
            boundary.confidence,
            strongest_completion,
            cadential_formal_closure_candidate_ceiling,
        });
    }

    // This precursor deliberately does not establish a cadence class or even
    // final formal closure. It only proves that a phrase boundary and an
    // independently observed completion process coincide without borrowing the
    // cadence label itself as evidence.
    result.formal_closure_established = false;
    return result;
}

} // namespace vgmtooling::model
