#!/usr/bin/env python3
"""Guarded one-shot migration: persistent performed trajectories become motif evidence."""

from pathlib import Path


def replace_once(path_s: str, old: str, new: str, label: str) -> None:
    path = Path(path_s)
    text = path.read_text(encoding="utf-8")
    if old in text:
        if text.count(old) != 1:
            raise SystemExit(f"{label}: expected one anchor, found {text.count(old)}")
        path.write_text(text.replace(old, new, 1), encoding="utf-8")
        return
    if new not in text:
        raise SystemExit(f"{label}: neither predecessor nor successor found")


def write_exact(path_s: str, content: str) -> None:
    path = Path(path_s)
    if path.exists():
        if path.read_text(encoding="utf-8") != content:
            raise SystemExit(f"{path_s}: existing file differs from guarded content")
        return
    path.write_text(content, encoding="utf-8")


def main() -> int:
    profile = Path("model/part_motif_profile.h")
    text = profile.read_text(encoding="utf-8")
    if '#include "pitch_motion_articulation.h"' not in text:
        text = text.replace(
            '#include "musical_execution_graph.h"\n',
            '#include "musical_execution_graph.h"\n#include "pitch_motion_articulation.h"\n',
            1,
        )
    if "struct part_gesture_performance_shape" not in text:
        anchor = "namespace vgmtooling::model {\n\nstruct part_gesture_observation {"
        replacement = '''namespace vgmtooling::model {

struct part_gesture_performance_shape {
    pitch_motion_articulation_kind kind = pitch_motion_articulation_kind::steady_pitch;
    double pitch_range_semitones = 0.0;
    double net_motion_semitones = 0.0;
    std::size_t direction_changes = 0;
    double confidence = 1.0;
};

inline bool is_resolved_part_gesture_performance_shape(
    const part_gesture_performance_shape& shape) noexcept {
    return shape.kind != pitch_motion_articulation_kind::in_episode_pitch_change_unresolved &&
           shape.kind != pitch_motion_articulation_kind::rearticulation_boundary;
}

struct part_gesture_observation {'''
        if anchor not in text:
            raise SystemExit("part gesture shape anchor not found")
        text = text.replace(anchor, replacement, 1)
    if "std::optional<part_gesture_performance_shape> performance_shape{};" not in text:
        old = '''    evidence_status status = evidence_status::derived;
    double confidence = 1.0;
};

struct part_motif_profile {'''
        new = '''    evidence_status status = evidence_status::derived;
    double confidence = 1.0;
    std::optional<part_gesture_performance_shape> performance_shape{};
};

struct part_motif_profile {'''
        if old not in text:
            raise SystemExit("gesture performance-shape field anchor not found")
        text = text.replace(old, new, 1)
    if "std::optional<std::vector<part_gesture_performance_shape>> performance_shapes{};" not in text:
        old = '''    std::optional<double> pitch_range_octaves{};
    evidence_status status = evidence_status::derived;'''
        new = '''    std::optional<double> pitch_range_octaves{};
    std::optional<std::vector<part_gesture_performance_shape>> performance_shapes{};
    evidence_status status = evidence_status::derived;'''
        if old not in text:
            raise SystemExit("motif profile performance-shape field anchor not found")
        text = text.replace(old, new, 1)
    if "performance_shape_similarity" not in text:
        old = '''    std::optional<double> contour_similarity{};
    double combined_similarity = 0.0;
    double identity_confidence = 0.0;
    bool pitch_comparable = false;'''
        new = '''    std::optional<double> contour_similarity{};
    std::optional<double> performance_shape_similarity{};
    double combined_similarity = 0.0;
    double identity_confidence = 0.0;
    bool pitch_comparable = false;
    bool performance_shape_comparable = false;'''
        if old not in text:
            raise SystemExit("motif similarity performance-shape field anchor not found")
        text = text.replace(old, new, 1)
    if "motif observation performance-shape confidence" not in text:
        old = '''        if (observation.confidence < 0.0 || observation.confidence > 1.0)
            throw std::invalid_argument("motif observation confidence must be in [0, 1]");
        weakest_status = static_cast<evidence_status>(std::max('''
        new = '''        if (observation.confidence < 0.0 || observation.confidence > 1.0)
            throw std::invalid_argument("motif observation confidence must be in [0, 1]");
        if (observation.performance_shape.has_value()) {
            const auto& shape = *observation.performance_shape;
            if (!is_resolved_part_gesture_performance_shape(shape))
                throw std::invalid_argument("motif observation performance shape must be resolved");
            if (!std::isfinite(shape.pitch_range_semitones) || shape.pitch_range_semitones < 0.0 ||
                !std::isfinite(shape.net_motion_semitones) ||
                !std::isfinite(shape.confidence) || shape.confidence < 0.0 || shape.confidence > 1.0) {
                throw std::invalid_argument("motif observation performance-shape confidence and motion must be finite");
            }
            evidence_confidence = std::min(evidence_confidence, shape.confidence);
        }
        weakest_status = static_cast<evidence_status>(std::max('''
        if old not in text:
            raise SystemExit("motif observation shape validation anchor not found")
        text = text.replace(old, new, 1)
    if "all_have_performance_shape" not in text:
        old = '''        result.pitch_range_octaves = high - low;
    }

    return result;
}'''
        new = '''        result.pitch_range_octaves = high - low;
    }

    bool all_have_performance_shape = true;
    for (const auto& observation : observations)
        all_have_performance_shape = all_have_performance_shape && observation.performance_shape.has_value();
    if (all_have_performance_shape) {
        std::vector<part_gesture_performance_shape> shapes;
        shapes.reserve(observations.size());
        for (const auto& observation : observations)
            shapes.push_back(*observation.performance_shape);
        result.performance_shapes = std::move(shapes);
    }

    return result;
}'''
        if old not in text:
            raise SystemExit("motif profile shape aggregation anchor not found")
        text = text.replace(old, new, 1)
    if "compare_part_gesture_performance_shapes" not in text:
        anchor = '''inline part_motif_similarity compare_part_motif_profiles(
    const part_motif_profile& first,
    const part_motif_profile& second) {'''
        helper = '''inline double compare_part_gesture_performance_shapes(
    const std::vector<part_gesture_performance_shape>& first,
    const std::vector<part_gesture_performance_shape>& second) {
    if (first.size() != second.size() || first.empty())
        return 0.0;

    std::size_t kind_matches = 0;
    double range_difference = 0.0;
    double net_difference = 0.0;
    double direction_difference = 0.0;
    for (std::size_t index = 0; index < first.size(); ++index) {
        kind_matches += first[index].kind == second[index].kind ? 1u : 0u;
        range_difference += std::fabs(
            first[index].pitch_range_semitones - second[index].pitch_range_semitones);
        net_difference += std::fabs(
            first[index].net_motion_semitones - second[index].net_motion_semitones);
        const auto first_changes = static_cast<double>(first[index].direction_changes);
        const auto second_changes = static_cast<double>(second[index].direction_changes);
        direction_difference += std::fabs(first_changes - second_changes);
    }

    const double count = static_cast<double>(first.size());
    const double kind_similarity = static_cast<double>(kind_matches) / count;
    const double range_similarity = 1.0 / (1.0 + 0.25 * (range_difference / count));
    const double net_similarity = 1.0 / (1.0 + 0.25 * (net_difference / count));
    const double direction_similarity = 1.0 / (1.0 + direction_difference / count);
    return 0.40 * kind_similarity +
           0.25 * range_similarity +
           0.25 * net_similarity +
           0.10 * direction_similarity;
}

''' + anchor
        if anchor not in text:
            raise SystemExit("motif performance-shape comparison anchor not found")
        text = text.replace(anchor, helper, 1)
    if "result.performance_shape_comparable =" not in text:
        old = '''    if (result.pitch_comparable) {
        result.interval_similarity = bounded_difference_similarity(
            *first.interval_octaves,
            *second.interval_octaves,
            4.0);
        result.contour_similarity = contour_match_similarity(
            *first.pitch_contour,
            *second.pitch_contour);
        weighted_sum += 0.45 * *result.interval_similarity;
        weighted_sum += 0.20 * *result.contour_similarity;
        total_weight += 0.65;
    }

    result.combined_similarity = total_weight > 0.0 ? weighted_sum / total_weight : 0.0;
    const double structural_identity = result.pitch_comparable
        ? result.combined_similarity
        : std::min(result.combined_similarity, rhythm_only_motif_identity_ceiling);'''
        new = '''    if (result.pitch_comparable) {
        result.interval_similarity = bounded_difference_similarity(
            *first.interval_octaves,
            *second.interval_octaves,
            4.0);
        result.contour_similarity = contour_match_similarity(
            *first.pitch_contour,
            *second.pitch_contour);
        weighted_sum += 0.45 * *result.interval_similarity;
        weighted_sum += 0.20 * *result.contour_similarity;
        total_weight += 0.65;
    }

    result.performance_shape_comparable =
        first.performance_shapes.has_value() && second.performance_shapes.has_value() &&
        first.performance_shapes->size() == second.performance_shapes->size() &&
        !first.performance_shapes->empty();
    if (result.performance_shape_comparable) {
        result.performance_shape_similarity = compare_part_gesture_performance_shapes(
            *first.performance_shapes,
            *second.performance_shapes);
        weighted_sum += 0.20 * *result.performance_shape_similarity;
        total_weight += 0.20;
    }

    result.combined_similarity = total_weight > 0.0 ? weighted_sum / total_weight : 0.0;
    const double structural_identity = result.pitch_comparable || result.performance_shape_comparable
        ? result.combined_similarity
        : std::min(result.combined_similarity, rhythm_only_motif_identity_ceiling);'''
        if old not in text:
            raise SystemExit("motif performance-shape weighting anchor not found")
        text = text.replace(old, new, 1)
    profile.write_text(text, encoding="utf-8")

    bridge = '''#pragma once

#include "part_motif_profile.h"
#include "persistent_part_performance_trajectory.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <vector>

namespace vgmtooling::model {

inline std::optional<part_gesture_performance_shape>
resolved_part_gesture_performance_shape(
    const pitch_motion_articulation_hypothesis& articulation) {
    if (articulation.kind == pitch_motion_articulation_kind::in_episode_pitch_change_unresolved ||
        articulation.kind == pitch_motion_articulation_kind::rearticulation_boundary)
        return std::nullopt;
    if (!std::isfinite(articulation.pitch_range_semitones) ||
        articulation.pitch_range_semitones < 0.0 ||
        !std::isfinite(articulation.net_motion_semitones) ||
        !std::isfinite(articulation.confidence) ||
        articulation.confidence < 0.0 || articulation.confidence > 1.0) {
        throw std::invalid_argument("resolved performance shape carries invalid motion evidence");
    }
    return part_gesture_performance_shape{
        articulation.kind,
        articulation.pitch_range_semitones,
        articulation.net_motion_semitones,
        articulation.direction_changes,
        articulation.confidence,
    };
}

inline std::vector<part_gesture_observation>
make_performed_part_gesture_observations(
    const persistent_part_performance_trajectory& performance,
    node_id part_id,
    evidence_status part_status,
    double part_confidence) {
    if (part_id == 0)
        throw std::invalid_argument("performed motif bridge requires a persistent-part id");
    if (!std::isfinite(part_confidence) || part_confidence < 0.0 || part_confidence > 1.0)
        throw std::invalid_argument("performed motif bridge requires bounded part confidence");
    if (performance.segments.size() != performance.identity.subject_nodes.size() ||
        performance.segments.empty())
        throw std::invalid_argument("performed motif bridge requires one segment per identity episode");

    std::vector<part_gesture_observation> result;
    result.reserve(performance.segments.size());
    for (std::size_t index = 0; index < performance.segments.size(); ++index) {
        const auto& segment = performance.segments[index];
        if (segment.physical_episode_id != performance.identity.subject_nodes[index] ||
            segment.samples.empty()) {
            throw std::invalid_argument("performed motif segment no longer matches persistent identity");
        }
        const auto& onset = segment.samples.front();
        if (onset.source_node == 0 || onset.physical_episode_id != segment.physical_episode_id ||
            onset.pitch_basis.empty() || onset.interval_semantics.empty() ||
            !std::isfinite(onset.log2_pitch_coordinate)) {
            throw std::invalid_argument("performed motif onset lacks source-backed pitch semantics");
        }

        const auto shape = resolved_part_gesture_performance_shape(segment.articulation);
        double confidence = std::min({
            performance.confidence,
            part_confidence,
            segment.articulation.confidence,
        });
        if (shape.has_value())
            confidence = std::min(confidence, shape->confidence);

        part_gesture_observation observation{
            onset.source_node,
            part_id,
            onset.time,
            onset.log2_pitch_coordinate,
            onset.pitch_basis,
            onset.interval_semantics,
            part_status,
            confidence,
        };
        observation.performance_shape = shape;
        result.push_back(std::move(observation));
    }
    return result;
}

} // namespace vgmtooling::model
'''
    write_exact("model/performed_part_motif_bridge.h", bridge)

    genesis = Path("components/vgm/enhancement/genesis_part_motif_adapter.h")
    text = genesis.read_text(encoding="utf-8")
    if '#include "genesis_persistent_performance_adapter.h"' not in text:
        text = text.replace(
            '#include "genesis_part_evidence.h"\n',
            '#include "genesis_part_evidence.h"\n#include "genesis_persistent_performance_adapter.h"\n',
            1,
        )
    if '#include "../../../model/performed_part_motif_bridge.h"' not in text:
        text = text.replace(
            '#include "../../../model/part_phrase_boundary_discovery.h"\n',
            '#include "../../../model/part_phrase_boundary_discovery.h"\n#include "../../../model/performed_part_motif_bridge.h"\n',
            1,
        )
    marker = "collect_genesis_performance_trajectory_gestures"
    if marker not in text:
        block = '''
inline std::optional<std::vector<vgmtooling::model::part_gesture_observation>>
collect_genesis_performance_trajectory_gestures(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const genesis_pitch_clock_context& clocks,
    const std::string& source,
    genesis_part_continuity_policy continuity_policy,
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    if (part == nullptr || part->kind != node_kind::part)
        throw std::invalid_argument("Genesis performed-trajectory motif collection requires a persistent part");
    const auto part_evidence = read_persistent_part_motif_evidence(*part);
    auto performance = project_genesis_ym2612_part_performance(
        graph,
        part_id,
        clocks,
        source,
        continuity_policy,
        motion_policy);
    if (!performance.has_value())
        return std::nullopt;
    return make_performed_part_gesture_observations(
        *performance,
        part_id,
        part_evidence.status,
        part_evidence.confidence);
}

inline std::optional<vgmtooling::model::part_motif_profile>
make_genesis_performance_trajectory_motif_profile(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const genesis_pitch_clock_context& clocks,
    const std::string& source,
    genesis_part_continuity_policy continuity_policy,
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    auto observations = collect_genesis_performance_trajectory_gestures(
        graph,
        part_id,
        clocks,
        source,
        continuity_policy,
        motion_policy);
    if (!observations.has_value() || observations->size() < 3)
        return std::nullopt;
    return vgmtooling::model::make_part_motif_profile(*observations);
}

inline std::vector<vgmtooling::model::repeated_part_motif_hypothesis>
discover_genesis_performance_trajectory_motifs(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const genesis_pitch_clock_context& clocks,
    const std::string& source,
    genesis_part_continuity_policy continuity_policy,
    const vgmtooling::model::part_motif_discovery_policy& motif_policy = {},
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    auto observations = collect_genesis_performance_trajectory_gestures(
        graph,
        part_id,
        clocks,
        source,
        continuity_policy,
        motion_policy);
    if (!observations.has_value())
        return {};
    return vgmtooling::model::discover_repeated_part_motifs(*observations, motif_policy);
}

'''
        close = "} // namespace gameaudio::vgm\n"
        if text.count(close) != 1:
            raise SystemExit("Genesis motif namespace close is not singular")
        text = text.replace(close, block + close, 1)
    genesis.write_text(text, encoding="utf-8")

    spc = Path("components/spc/spc_part_motif_adapter.h")
    text = spc.read_text(encoding="utf-8")
    if '#include "spc_persistent_performance_adapter.h"' not in text:
        text = text.replace(
            '#include "spc_part_evidence.h"\n',
            '#include "spc_part_evidence.h"\n#include "spc_persistent_performance_adapter.h"\n',
            1,
        )
    if '#include "../../model/performed_part_motif_bridge.h"' not in text:
        text = text.replace(
            '#include "../../model/part_phrase_boundary_discovery.h"\n',
            '#include "../../model/part_phrase_boundary_discovery.h"\n#include "../../model/performed_part_motif_bridge.h"\n',
            1,
        )
    marker = "collect_spc_performance_trajectory_gestures"
    if marker not in text:
        block = '''
inline std::optional<std::vector<vgmtooling::model::part_gesture_observation>>
collect_spc_performance_trajectory_gestures(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const std::string& source,
    spc_part_continuity_policy continuity_policy = {},
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(part_id);
    if (part == nullptr || part->kind != node_kind::part)
        throw std::invalid_argument("SPC performed-trajectory motif collection requires a persistent part");
    const auto part_evidence = read_persistent_part_motif_evidence(*part);
    auto performance = project_spc_part_performance(
        graph,
        part_id,
        source,
        continuity_policy,
        motion_policy);
    if (!performance.has_value())
        return std::nullopt;
    return make_performed_part_gesture_observations(
        *performance,
        part_id,
        part_evidence.status,
        part_evidence.confidence);
}

inline std::optional<vgmtooling::model::part_motif_profile>
make_spc_performance_trajectory_motif_profile(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const std::string& source,
    spc_part_continuity_policy continuity_policy = {},
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    auto observations = collect_spc_performance_trajectory_gestures(
        graph,
        part_id,
        source,
        continuity_policy,
        motion_policy);
    if (!observations.has_value() || observations->size() < 3)
        return std::nullopt;
    return vgmtooling::model::make_part_motif_profile(*observations);
}

inline std::vector<vgmtooling::model::repeated_part_motif_hypothesis>
discover_spc_performance_trajectory_motifs(
    const vgmtooling::model::musical_execution_graph& graph,
    vgmtooling::model::node_id part_id,
    const std::string& source,
    spc_part_continuity_policy continuity_policy = {},
    const vgmtooling::model::part_motif_discovery_policy& motif_policy = {},
    vgmtooling::model::pitch_motion_analysis_policy motion_policy = {}) {
    auto observations = collect_spc_performance_trajectory_gestures(
        graph,
        part_id,
        source,
        continuity_policy,
        motion_policy);
    if (!observations.has_value())
        return {};
    return vgmtooling::model::discover_repeated_part_motifs(*observations, motif_policy);
}

'''
        close = "} // namespace gameaudio::spc\n"
        if text.count(close) != 1:
            raise SystemExit("SPC motif namespace close is not singular")
        text = text.replace(close, block + close, 1)
    spc.write_text(text, encoding="utf-8")

    shape_test = '''#include "../../model/part_motif_profile.h"

#include <cassert>
#include <cmath>
#include <optional>
#include <vector>

using namespace vgmtooling::model;

namespace {

time_coordinate at(std::int64_t tick) {
    return time_coordinate{time_domain::source, tick, 1000, 0};
}

part_gesture_performance_shape shape(
    pitch_motion_articulation_kind kind,
    double range,
    double net,
    std::size_t changes) {
    return {kind, range, net, changes, 0.96};
}

part_gesture_observation gesture(
    node_id source,
    std::int64_t tick,
    double pitch,
    std::optional<part_gesture_performance_shape> performance_shape) {
    part_gesture_observation result{
        source,
        900,
        at(tick),
        pitch,
        "performed_frequency",
        "log2_frequency_ratio_octaves",
        evidence_status::derived,
        0.96,
    };
    result.performance_shape = std::move(performance_shape);
    return result;
}
}

int main() {
    const std::vector<part_gesture_observation> first{
        gesture(1, 0, 0.0, shape(pitch_motion_articulation_kind::steady_pitch, 0.01, 0.0, 0)),
        gesture(2, 100, 1.0, shape(pitch_motion_articulation_kind::glide_candidate, 4.0, 4.0, 0)),
        gesture(3, 200, 0.0, shape(pitch_motion_articulation_kind::periodic_modulation_candidate, 1.2, 0.1, 3)),
    };
    const auto same_profile = make_part_motif_profile(first);
    assert(same_profile.performance_shapes.has_value());
    assert(same_profile.performance_shapes->size() == 3);

    auto changed = first;
    changed[1].performance_shape = shape(pitch_motion_articulation_kind::steady_pitch, 0.02, 0.0, 0);
    changed[2].performance_shape = shape(pitch_motion_articulation_kind::glide_candidate, 5.0, -5.0, 0);
    const auto changed_profile = make_part_motif_profile(changed);

    const auto same = compare_part_motif_profiles(same_profile, same_profile);
    const auto different_shape = compare_part_motif_profiles(same_profile, changed_profile);
    assert(same.pitch_comparable);
    assert(same.performance_shape_comparable);
    assert(same.performance_shape_similarity.has_value());
    assert(std::fabs(*same.performance_shape_similarity - 1.0) < 1e-12);
    assert(std::fabs(same.combined_similarity - 1.0) < 1e-12);
    assert(different_shape.pitch_comparable);
    assert(different_shape.performance_shape_comparable);
    assert(different_shape.performance_shape_similarity.has_value());
    assert(*different_shape.performance_shape_similarity < 1.0);
    assert(different_shape.combined_similarity < same.combined_similarity);

    auto unresolved = first;
    unresolved[1].performance_shape.reset();
    const auto unresolved_profile = make_part_motif_profile(unresolved);
    assert(!unresolved_profile.performance_shapes.has_value());
    const auto missing_shape = compare_part_motif_profiles(same_profile, unresolved_profile);
    assert(missing_shape.pitch_comparable);
    assert(!missing_shape.performance_shape_comparable);
    assert(!missing_shape.performance_shape_similarity.has_value());

    return 0;
}
'''
    write_exact("tests/model/performed_part_motif_shape_test.cpp", shape_test)

    cmake = Path("cmake/semantic_model_tests.cmake")
    text = cmake.read_text(encoding="utf-8")
    if "performed_part_motif_shape_test" not in text:
        text += '''

# Performed-trajectory shape evidence feeds the existing motif engine.
list(APPEND GAMEAUDIO_TEST_TARGETS
    performed_part_motif_shape_test
)

add_executable(
    performed_part_motif_shape_test
    tests/model/performed_part_motif_shape_test.cpp
)

target_include_directories(
    performed_part_motif_shape_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME performed_part_motif_shape
    COMMAND performed_part_motif_shape_test
)
'''
        cmake.write_text(text, encoding="utf-8")

    genesis_test = Path("tests/vgm/genesis_persistent_performance_adapter_test.cpp")
    text = genesis_test.read_text(encoding="utf-8")
    if '#include "../../components/vgm/enhancement/genesis_part_motif_adapter.h"' not in text:
        text = text.replace(
            '#include "../../components/vgm/enhancement/genesis_persistent_performance_adapter.h"',
            '#include "../../components/vgm/enhancement/genesis_part_motif_adapter.h"',
            1,
        )
    if "trajectory_motif" not in text:
        old = '''        assert(from_part.has_value());
        assert(from_part->identity.subject_nodes == episodes);
        assert(from_part->segments.size() == 3);

        const auto discovered = discover_genesis_ym2612_persistent_performances('''
        new = '''        assert(from_part.has_value());
        assert(from_part->identity.subject_nodes == episodes);
        assert(from_part->segments.size() == 3);

        const auto trajectory_motif = make_genesis_performance_trajectory_motif_profile(
            graph,
            part_id,
            clocks,
            "part-motif-test",
            continuity);
        assert(trajectory_motif.has_value());
        assert(trajectory_motif->interval_octaves.has_value());
        // The first episode has only one trustworthy pitch state. Its motion is
        // unresolved, so the whole motif window withholds shape comparison.
        assert(!trajectory_motif->performance_shapes.has_value());

        const auto discovered = discover_genesis_ym2612_persistent_performances('''
        if old not in text:
            raise SystemExit("Genesis trajectory motif test anchor not found")
        text = text.replace(old, new, 1)
    genesis_test.write_text(text, encoding="utf-8")

    spc_test = Path("tests/spc/spc_persistent_performance_adapter_test.cpp")
    text = spc_test.read_text(encoding="utf-8")
    if '#include "components/spc/spc_part_motif_adapter.h"' not in text:
        text = text.replace(
            '#include "components/spc/spc_persistent_performance_adapter.h"\n',
            '#include "components/spc/spc_persistent_performance_adapter.h"\n#include "components/spc/spc_part_motif_adapter.h"\n',
            1,
        )
    if "trajectory_motif" not in text:
        old = '''        assert(performance.rearticulation_boundaries[1].physical_episode_id == value.second);
        assert(performance.rearticulation_boundaries[1].next_physical_episode_id == value.third);
    }
'''
        new = '''        assert(performance.rearticulation_boundaries[1].physical_episode_id == value.second);
        assert(performance.rearticulation_boundaries[1].next_physical_episode_id == value.third);

        const auto parts = value.graph.nodes_of_kind(node_kind::part);
        assert(parts.size() == 1);
        const auto trajectory_motif = make_spc_performance_trajectory_motif_profile(
            value.graph,
            parts.front()->id,
            "spc-trajectory-motif-test",
            policy());
        assert(trajectory_motif.has_value());
        assert(trajectory_motif->interval_octaves.has_value());
        // Current SPC capture supplies one trustworthy pitch state per episode.
        // Unresolved motion is absence of evidence, never a matching shape.
        assert(!trajectory_motif->performance_shapes.has_value());
    }
'''
        if old not in text:
            raise SystemExit("SPC trajectory motif test anchor not found")
        text = text.replace(old, new, 1)
    spc_test.write_text(text, encoding="utf-8")

    print("performed-trajectory motif bridge staged")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
