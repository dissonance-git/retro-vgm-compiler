#pragma once

#include "cadential_arrival_hypothesis.h"
#include "tonal_region_relation.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace vgmtooling::model {

inline tonal_region_relation_evidence_origin map_tonal_center_origin_to_region_origin(
    tonal_center_evidence_origin origin) noexcept {
    switch (origin) {
    case tonal_center_evidence_origin::pitch_distribution:
        return tonal_region_relation_evidence_origin::pitch_collection;
    case tonal_center_evidence_origin::bass_structure:
        return tonal_region_relation_evidence_origin::harmony;
    case tonal_center_evidence_origin::phrase_structure:
        return tonal_region_relation_evidence_origin::phrase_structure;
    case tonal_center_evidence_origin::harmony:
        return tonal_region_relation_evidence_origin::harmony;
    case tonal_center_evidence_origin::authored_source:
        return tonal_region_relation_evidence_origin::authored_source;
    case tonal_center_evidence_origin::external_annotation:
        return tonal_region_relation_evidence_origin::external_annotation;
    }
    return tonal_region_relation_evidence_origin::external_annotation;
}

inline bool tonal_region_contains_coordinate(
    const time_span& region,
    const time_coordinate& coordinate) noexcept {
    if (!finite_valid_tonal_region(region) ||
        !compatible_tonal_region_coordinate_basis(region.start, coordinate)) {
        return false;
    }
    return coordinate.tick >= region.start.tick &&
           coordinate.tick <= region.end->tick;
}

inline tonal_region_relation_evidence make_phrase_partition_region_evidence(
    const phrase_boundary_consensus& boundary,
    const tonal_center_hypothesis& source_center,
    const tonal_center_hypothesis& target_center,
    std::string dependency_group,
    node_id source_node = 0,
    std::int64_t alignment_tolerance_ticks = 0) {
    if (dependency_group.empty())
        throw std::invalid_argument("phrase-partition region evidence requires a dependency group");
    if (alignment_tolerance_ticks < 0)
        throw std::invalid_argument("phrase-partition alignment tolerance must be nonnegative");
    if (infer_tonal_region_topology(source_center.region, target_center.region) !=
        tonal_region_topology::sequential) {
        throw std::invalid_argument("phrase-partition evidence requires sequential tonal regions");
    }
    if (!compatible_tonal_region_coordinate_basis(
            boundary.representative,
            *source_center.region.end) ||
        !compatible_tonal_region_coordinate_basis(
            boundary.representative,
            target_center.region.start) ||
        std::llabs(boundary.representative.tick - source_center.region.end->tick) > alignment_tolerance_ticks ||
        std::llabs(boundary.representative.tick - target_center.region.start.tick) > alignment_tolerance_ticks) {
        throw std::invalid_argument("phrase boundary does not partition the supplied tonal regions");
    }

    tonal_region_relation_evidence evidence;
    evidence.kind = tonal_region_relation_evidence_kind::phrase_partition;
    evidence.origin = tonal_region_relation_evidence_origin::phrase_structure;
    evidence.confidence = boundary.confidence;
    evidence.dependency_group = std::move(dependency_group);
    evidence.source_node = source_node;
    evidence.status = evidence_status::hypothesis;
    evidence.source = "cross-part or authored phrase-boundary consensus partitioning tonal regions";
    return evidence;
}

inline tonal_region_relation_evidence make_structural_arrival_region_evidence(
    const cadential_arrival_hypothesis& arrival,
    const tonal_center_hypothesis& target_center,
    std::string dependency_group,
    node_id source_node = 0) {
    if (dependency_group.empty())
        throw std::invalid_argument("structural-arrival region evidence requires a dependency group");
    if (!tonal_region_contains_coordinate(target_center.region, arrival.arrival_time))
        throw std::invalid_argument("cadential arrival must lie inside the target tonal region");

    tonal_region_relation_evidence evidence;
    evidence.kind = tonal_region_relation_evidence_kind::structural_arrival;
    evidence.origin = tonal_region_relation_evidence_origin::harmony;
    evidence.confidence = std::min(arrival.confidence, target_center.confidence);
    evidence.dependency_group = std::move(dependency_group);
    evidence.source_node = source_node;
    evidence.status = evidence_status::hypothesis;
    evidence.source = "phrase-aligned harmonic arrival inside target tonal region";
    return evidence;
}

inline tonal_region_relation_evidence make_structural_pitch_collection_region_evidence(
    const pitch_class_collection_profile& collection,
    const tonal_center_hypothesis& target_center,
    std::string dependency_group,
    node_id source_node = 0) {
    if (dependency_group.empty())
        throw std::invalid_argument("structural pitch-collection region evidence requires a dependency group");
    validate_pitch_class_collection_profile(collection);
    if (collection.scope == pitch_class_collection_scope::surface_performance)
        throw std::invalid_argument("surface pitch collection cannot support a structural tonal-region relation");
    if (!time_span_contains_span(target_center.region, collection.region))
        throw std::invalid_argument("structural pitch collection must lie inside the target tonal region");

    tonal_region_relation_evidence evidence;
    evidence.kind = tonal_region_relation_evidence_kind::structural_pitch_collection;
    evidence.origin = tonal_region_relation_evidence_origin::pitch_collection;
    evidence.confidence = std::min(
        {collection.confidence, collection.projection_coverage, target_center.confidence});
    evidence.dependency_group = std::move(dependency_group);
    evidence.source_node = source_node;
    evidence.status = evidence_status::hypothesis;
    evidence.source = collection.source;
    return evidence;
}

inline tonal_region_relation_evidence make_center_persistence_region_evidence(
    const tonal_center_hypothesis& target_center,
    double minimum_support_confidence = 0.60) {
    if (!std::isfinite(minimum_support_confidence) ||
        minimum_support_confidence < 0.0 || minimum_support_confidence > 1.0) {
        throw std::invalid_argument("center-persistence support threshold must lie in [0, 1]");
    }

    const tonal_center_evidence* strongest = nullptr;
    for (const auto& item : target_center.supporting_evidence) {
        if (item.kind != tonal_center_evidence_kind::recurrence &&
            item.kind != tonal_center_evidence_kind::duration) {
            continue;
        }
        if (item.confidence < minimum_support_confidence)
            continue;
        if (strongest == nullptr || item.confidence > strongest->confidence)
            strongest = &item;
    }
    if (strongest == nullptr)
        throw std::invalid_argument("target tonal center has no grounded recurrence/duration evidence for persistence");

    tonal_region_relation_evidence evidence;
    evidence.kind = tonal_region_relation_evidence_kind::center_persistence;
    evidence.origin = map_tonal_center_origin_to_region_origin(strongest->origin);
    evidence.confidence = std::min(target_center.confidence, strongest->confidence);
    evidence.dependency_group = strongest->dependency_group;
    evidence.source_node = strongest->source_node;
    evidence.status = strongest->status;
    evidence.source = strongest->source.empty()
        ? "target tonal-center recurrence/duration evidence"
        : strongest->source;
    return evidence;
}

} // namespace vgmtooling::model
