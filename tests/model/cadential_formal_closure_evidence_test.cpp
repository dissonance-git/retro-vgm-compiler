#include "model/cadential_formal_closure_evidence.h"

#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

time_coordinate at(std::int64_t tick) {
    return {time_domain::source, tick, 0, 0};
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

phrase_boundary_evidence evidence(
    phrase_boundary_evidence_kind kind,
    phrase_boundary_evidence_origin origin,
    double confidence,
    const char* source,
    evidence_status status = evidence_status::hypothesis) {
    phrase_boundary_evidence result;
    result.kind = kind;
    result.origin = origin;
    result.polarity = phrase_boundary_evidence_polarity::supports;
    result.status = status;
    result.confidence = confidence;
    result.source = source;
    result.detail = "formal closure regression evidence";
    return result;
}

phrase_boundary_hypothesis phrase(
    std::vector<phrase_boundary_evidence> support,
    double proposed_confidence = 0.92) {
    return make_phrase_boundary_hypothesis(
        at(200),
        proposed_confidence,
        std::move(support));
}

part_phrase_boundary_hypothesis part(
    node_id part_id,
    phrase_boundary_hypothesis boundary) {
    part_phrase_boundary_hypothesis result;
    result.part_id = part_id;
    result.boundary = std::move(boundary);
    return result;
}

phrase_boundary_consensus cross_part_completion_boundary() {
    std::vector<part_phrase_boundary_hypothesis> parts;
    parts.push_back(part(11, phrase({
        evidence(
            phrase_boundary_evidence_kind::motif_completion,
            phrase_boundary_evidence_origin::motif_analysis,
            0.90,
            "motif completion"),
        evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            0.88,
            "timing support"),
    })));
    parts.push_back(part(12, phrase({
        evidence(
            phrase_boundary_evidence_kind::repeated_motif_alignment,
            phrase_boundary_evidence_origin::motif_analysis,
            0.86,
            "repeated motif completion"),
        evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            0.87,
            "second-part timing support"),
    })));
    return make_phrase_boundary_consensus(std::move(parts));
}

phrase_boundary_consensus cross_part_cadence_only_boundary() {
    std::vector<part_phrase_boundary_hypothesis> parts;
    for (node_id part_id : {node_id{11}, node_id{12}}) {
        parts.push_back(part(part_id, phrase({
            evidence(
                phrase_boundary_evidence_kind::cadence_or_resolution,
                phrase_boundary_evidence_origin::harmonic_analysis,
                0.95,
                "upstream cadence detector"),
            evidence(
                phrase_boundary_evidence_kind::temporal_gap,
                phrase_boundary_evidence_origin::performance_timing,
                0.90,
                "timing support"),
        })));
    }
    return make_phrase_boundary_consensus(std::move(parts));
}
} // namespace

int main() {
    // Independent motif completion aligned across a cross-part phrase boundary
    // is enough for a formal-closure candidate, but still not an established
    // closure or cadence class.
    const auto completion_boundary = cross_part_completion_boundary();
    const auto completion = infer_cadential_formal_closure_evidence(
        completion_boundary);
    CHECK(completion.phrase_boundary_grounded);
    CHECK(completion.cross_part_phrase_grounded);
    CHECK(completion.noncadential_completion_grounded);
    CHECK(completion.completion_observations == 2);
    CHECK(completion.completion_support_domains == 1);
    CHECK(completion.kind ==
        cadential_formal_closure_kind::completion_aligned_phrase_end_candidate);
    CHECK(completion.closure_candidate_resolved);
    CHECK(!completion.formal_closure_established);
    CHECK(close_enough(
        completion.confidence,
        cadential_formal_closure_candidate_ceiling));

    // Circular evidence is explicitly firewalled. A phrase boundary that is
    // supported by an upstream cadence detector may be a good boundary, but it
    // cannot use that same label to prove the formal closure needed by cadence.
    const auto circular_boundary = cross_part_cadence_only_boundary();
    const auto circular = infer_cadential_formal_closure_evidence(
        circular_boundary);
    CHECK(circular.phrase_boundary_grounded);
    CHECK(circular.cross_part_phrase_grounded);
    CHECK(circular.cadence_derived_support_present);
    CHECK(!circular.noncadential_completion_grounded);
    CHECK(!circular.closure_candidate_resolved);
    CHECK(circular.kind == cadential_formal_closure_kind::phrase_boundary_only);
    CHECK(close_enough(
        circular.confidence,
        cadential_phrase_boundary_only_ceiling));

    // Timing alignment alone marks punctuation-like segmentation, not formal
    // completion.
    std::vector<part_phrase_boundary_hypothesis> timing_parts;
    timing_parts.push_back(part(11, phrase({
        evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            0.92,
            "timing only A"),
    })));
    timing_parts.push_back(part(12, phrase({
        evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            0.91,
            "timing only B"),
    })));
    const auto timing_boundary = make_phrase_boundary_consensus(
        std::move(timing_parts));
    const auto timing = infer_cadential_formal_closure_evidence(timing_boundary);
    CHECK(timing.cross_part_phrase_grounded);
    CHECK(!timing.noncadential_completion_grounded);
    CHECK(!timing.closure_candidate_resolved);
    CHECK(close_enough(timing.confidence, phrase_boundary_timing_only_ceiling));

    // A local motif completion is useful evidence, but without cross-part or
    // explicit authored grounding it cannot become a global closure candidate.
    std::vector<part_phrase_boundary_hypothesis> local_parts;
    local_parts.push_back(part(11, phrase({
        evidence(
            phrase_boundary_evidence_kind::motif_completion,
            phrase_boundary_evidence_origin::motif_analysis,
            0.90,
            "local motif completion"),
        evidence(
            phrase_boundary_evidence_kind::temporal_gap,
            phrase_boundary_evidence_origin::performance_timing,
            0.88,
            "local timing support"),
    })));
    const auto local_boundary = make_phrase_boundary_consensus(
        std::move(local_parts));
    const auto local = infer_cadential_formal_closure_evidence(local_boundary);
    CHECK(!local.cross_part_phrase_grounded);
    CHECK(local.noncadential_completion_grounded);
    CHECK(!local.closure_candidate_resolved);
    CHECK(close_enough(local.confidence, cadential_phrase_boundary_only_ceiling));

    // An authored boundary is still only a boundary. This layer refuses to
    // silently reinterpret it as an authored phrase-end/closure annotation.
    std::vector<part_phrase_boundary_hypothesis> authored_parts;
    authored_parts.push_back(part(11, phrase({
        evidence(
            phrase_boundary_evidence_kind::authored_boundary,
            phrase_boundary_evidence_origin::authored_source,
            0.98,
            "authored boundary",
            evidence_status::exact),
    }, 0.98)));
    const auto authored_boundary = make_phrase_boundary_consensus(
        std::move(authored_parts));
    CHECK(authored_boundary.authored_grounded);
    const auto authored = infer_cadential_formal_closure_evidence(authored_boundary);
    CHECK(authored.authored_boundary_grounded);
    CHECK(!authored.noncadential_completion_grounded);
    CHECK(!authored.closure_candidate_resolved);

    // Forged summary flags cannot overrule the retained persistent-part set.
    bool forged_cross_part_rejected = false;
    try {
        auto forged = completion_boundary;
        forged.cross_part_grounded = false;
        (void)infer_cadential_formal_closure_evidence(forged);
    } catch (const std::invalid_argument&) {
        forged_cross_part_rejected = true;
    }
    CHECK(forged_cross_part_rejected);

    // Missing retained part provenance is malformed rather than weak evidence.
    bool missing_parts_rejected = false;
    try {
        auto forged = completion_boundary;
        forged.supporting_parts.clear();
        (void)infer_cadential_formal_closure_evidence(forged);
    } catch (const std::invalid_argument&) {
        missing_parts_rejected = true;
    }
    CHECK(missing_parts_rejected);

    return 0;
}
