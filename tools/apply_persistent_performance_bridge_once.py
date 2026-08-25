#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one anchor, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


pitch_header = ROOT / "model" / "pitch_motion_articulation.h"
replace_once(
    pitch_header,
    "    node_id physical_episode_id = 0;\n    std::vector<node_id> source_nodes;\n",
    "    node_id physical_episode_id = 0;\n    node_id next_physical_episode_id = 0;\n    std::vector<node_id> source_nodes;\n",
    "pitch articulation boundary identity field",
)
replace_once(
    pitch_header,
    "    result.kind = pitch_motion_articulation_kind::rearticulation_boundary;\n    result.rearticulation_supported = true;\n    result.confidence = confidence;\n",
    "    result.kind = pitch_motion_articulation_kind::rearticulation_boundary;\n    result.physical_episode_id = ending_episode;\n    result.next_physical_episode_id = starting_episode;\n    result.rearticulation_supported = true;\n    result.confidence = confidence;\n",
    "rearticulation preserves both episode ids",
)

pitch_test = ROOT / "tests" / "model" / "pitch_motion_articulation_test.cpp"
replace_once(
    pitch_test,
    "    CHECK(rearticulation.rearticulation_supported);\n    CHECK(close_enough(rearticulation.confidence, 0.96));\n",
    "    CHECK(rearticulation.rearticulation_supported);\n    CHECK(rearticulation.physical_episode_id == 3);\n    CHECK(rearticulation.next_physical_episode_id == 4);\n    CHECK(close_enough(rearticulation.confidence, 0.96));\n",
    "pitch articulation test boundary ids",
)

bridge_header = ROOT / "model" / "persistent_part_performance_trajectory.h"
if bridge_header.exists():
    raise SystemExit("persistent_part_performance_trajectory.h already exists")
bridge_header.write_text(r'''#pragma once

#include "persistent_part_trajectory.h"
#include "pitch_motion_articulation.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace vgmtooling::model {

struct persistent_part_performance_segment {
    node_id physical_episode_id = 0;
    std::vector<pitch_motion_sample> samples;
    pitch_motion_articulation_hypothesis articulation;
};

struct persistent_part_performance_trajectory {
    persistent_part_trajectory identity;
    std::vector<persistent_part_performance_segment> segments;
    std::vector<pitch_motion_articulation_hypothesis> rearticulation_boundaries;
    double confidence = 0.0;
};

inline persistent_part_performance_segment make_persistent_part_performance_segment(
    std::vector<pitch_motion_sample> samples,
    double confidence,
    pitch_motion_analysis_policy policy = {}) {
    if (samples.empty())
        throw std::invalid_argument("persistent-part performance segment requires pitch samples");

    persistent_part_performance_segment result;
    result.physical_episode_id = samples.front().physical_episode_id;
    result.articulation = analyze_in_episode_pitch_motion(samples, confidence, policy);
    result.samples = std::move(samples);
    return result;
}

inline persistent_part_performance_trajectory make_persistent_part_performance_trajectory(
    persistent_part_trajectory identity,
    std::vector<persistent_part_performance_segment> segments,
    std::vector<pitch_motion_articulation_hypothesis> rearticulation_boundaries) {
    if (identity.subject_nodes.size() < 2 || identity.transitions.empty())
        throw std::invalid_argument("persistent-part performance trajectory requires persistent identity");
    if (segments.size() != identity.subject_nodes.size())
        throw std::invalid_argument("persistent-part performance trajectory requires one segment per identity episode");
    if (rearticulation_boundaries.size() + 1 != identity.subject_nodes.size())
        throw std::invalid_argument("persistent-part performance trajectory requires one boundary between adjacent episodes");
    if (identity.confidence < 0.0 || identity.confidence > 1.0)
        throw std::invalid_argument("persistent-part performance identity confidence must be in [0, 1]");

    persistent_part_performance_trajectory result;
    result.identity = std::move(identity);
    result.segments = std::move(segments);
    result.rearticulation_boundaries = std::move(rearticulation_boundaries);
    result.confidence = result.identity.confidence;

    for (std::size_t index = 0; index < result.segments.size(); ++index) {
        const node_id expected_episode = result.identity.subject_nodes[index];
        const auto& segment = result.segments[index];
        const auto& articulation = segment.articulation;
        if (segment.physical_episode_id != expected_episode ||
            articulation.physical_episode_id != expected_episode) {
            throw std::invalid_argument("persistent-part performance segment does not match identity order");
        }
        if (segment.samples.empty())
            throw std::invalid_argument("persistent-part performance segment lost its pitch trajectory");
        if (articulation.kind == pitch_motion_articulation_kind::rearticulation_boundary ||
            articulation.rearticulation_supported ||
            articulation.next_physical_episode_id != 0) {
            throw std::invalid_argument("persistent-part performance segment cannot impersonate a rearticulation boundary");
        }
        if (articulation.confidence < 0.0 || articulation.confidence > 1.0)
            throw std::invalid_argument("persistent-part performance confidence must be in [0, 1]");
        if (articulation.source_nodes.size() != segment.samples.size())
            throw std::invalid_argument("persistent-part performance segment lost source-node correspondence");

        for (std::size_t sample_index = 0; sample_index < segment.samples.size(); ++sample_index) {
            const auto& sample = segment.samples[sample_index];
            if (sample.physical_episode_id != expected_episode ||
                sample.source_node != articulation.source_nodes[sample_index]) {
                throw std::invalid_argument("persistent-part performance samples no longer match their articulation evidence");
            }
        }
        result.confidence = std::min(result.confidence, articulation.confidence);
    }

    for (std::size_t index = 0; index < result.rearticulation_boundaries.size(); ++index) {
        const auto& boundary = result.rearticulation_boundaries[index];
        if (boundary.kind != pitch_motion_articulation_kind::rearticulation_boundary ||
            !boundary.rearticulation_supported) {
            throw std::invalid_argument("persistent-part performance boundary lacks rearticulation evidence");
        }
        if (boundary.physical_episode_id != result.identity.subject_nodes[index] ||
            boundary.next_physical_episode_id != result.identity.subject_nodes[index + 1]) {
            throw std::invalid_argument("persistent-part performance boundary does not match identity adjacency");
        }
        if (boundary.confidence < 0.0 || boundary.confidence > 1.0)
            throw std::invalid_argument("persistent-part rearticulation confidence must be in [0, 1]");
        result.confidence = std::min(result.confidence, boundary.confidence);
    }

    return result;
}

} // namespace vgmtooling::model
''', encoding="utf-8")

bridge_test = ROOT / "tests" / "model" / "persistent_part_performance_trajectory_test.cpp"
if bridge_test.exists():
    raise SystemExit("persistent_part_performance_trajectory_test.cpp already exists")
bridge_test.write_text(r'''#include "model/persistent_part_performance_trajectory.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace vgmtooling::model;

namespace {

persistent_part_hypothesis strong_link(node_id first, node_id second, double confidence) {
    return make_persistent_part_hypothesis(
        confidence,
        {first, second},
        {
            {
                persistent_part_evidence_kind::source_identity,
                persistent_part_evidence_origin::synthesis_runtime,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.95,
                "performance-trajectory-fixture",
                "same source identity across adjacent physical episodes",
                {first, second},
            },
            {
                persistent_part_evidence_kind::temporal_adjacency,
                persistent_part_evidence_origin::musical_analysis,
                persistent_part_evidence_polarity::supports,
                evidence_status::derived,
                0.90,
                "performance-trajectory-fixture",
                "bounded physical episodes are adjacent",
                {first, second},
            },
        });
}

std::vector<pitch_motion_sample> samples(
    node_id episode,
    node_id source_base,
    const std::vector<double>& semitone_offsets) {
    std::vector<pitch_motion_sample> result;
    for (std::size_t index = 0; index < semitone_offsets.size(); ++index) {
        result.push_back({
            static_cast<node_id>(source_base + index),
            episode,
            time_coordinate{
                time_domain::source,
                static_cast<std::int64_t>(index * 10),
                0,
                0,
            },
            semitone_offsets[index] / 12.0,
            "shared-log2-frequency",
            "log2_frequency_ratio_octaves",
        });
    }
    return result;
}

bool throws_invalid(const std::function<void()>& action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

bool close_enough(double first, double second) {
    return std::fabs(first - second) < 1e-9;
}

} // namespace

int main() {
    const auto identity = make_persistent_part_trajectory({
        strong_link(10, 20, 0.91),
        strong_link(20, 30, 0.88),
    });

    const auto first = make_persistent_part_performance_segment(
        samples(10, 100, {0.0, 0.0}),
        0.94);
    const auto second = make_persistent_part_performance_segment(
        samples(20, 200, {0.0, 0.5, 1.0, 1.5}),
        0.86);
    const auto third = make_persistent_part_performance_segment(
        samples(30, 300, {0.0, 0.5, -0.5, 0.5, 0.0}),
        0.92);

    assert(first.articulation.kind == pitch_motion_articulation_kind::steady_pitch);
    assert(second.articulation.kind == pitch_motion_articulation_kind::glide_candidate);
    assert(third.articulation.kind == pitch_motion_articulation_kind::periodic_modulation_candidate);

    const auto boundary_one = make_rearticulation_boundary(10, 20, 0.95);
    const auto boundary_two = make_rearticulation_boundary(20, 30, 0.84);
    const auto performance = make_persistent_part_performance_trajectory(
        identity,
        {first, second, third},
        {boundary_one, boundary_two});

    assert(performance.identity.subject_nodes.size() == 3);
    assert(performance.segments.size() == 3);
    assert(performance.rearticulation_boundaries.size() == 2);
    assert(performance.segments[1].physical_episode_id == 20);
    assert(performance.segments[1].samples.size() == 4);
    assert(performance.rearticulation_boundaries[0].physical_episode_id == 10);
    assert(performance.rearticulation_boundaries[0].next_physical_episode_id == 20);
    assert(close_enough(performance.confidence, 0.84));

    assert(throws_invalid([&] {
        (void)make_persistent_part_performance_trajectory(
            identity,
            {second, first, third},
            {boundary_one, boundary_two});
    }));

    assert(throws_invalid([&] {
        (void)make_persistent_part_performance_trajectory(
            identity,
            {first, second, third},
            {make_rearticulation_boundary(10, 30, 0.95), boundary_two});
    }));

    assert(throws_invalid([&] {
        auto malformed = first;
        malformed.articulation.next_physical_episode_id = 20;
        (void)make_persistent_part_performance_trajectory(
            identity,
            {malformed, second, third},
            {boundary_one, boundary_two});
    }));

    assert(throws_invalid([&] {
        auto malformed = second;
        malformed.samples.front().source_node += 1000;
        (void)make_persistent_part_performance_trajectory(
            identity,
            {first, malformed, third},
            {boundary_one, boundary_two});
    }));

    return 0;
}
''', encoding="utf-8")

cmake = ROOT / "cmake" / "semantic_model_tests.cmake"
cmake_text = cmake.read_text(encoding="utf-8")
marker = "# Persistent identity + performed pitch/articulation compiler bridge."
if marker in cmake_text:
    raise SystemExit("persistent performance CMake registration already exists")
cmake.write_text(
    cmake_text
    + r'''

# Persistent identity + performed pitch/articulation compiler bridge.
list(APPEND GAMEAUDIO_TEST_TARGETS
    pitch_motion_articulation_test
    persistent_part_performance_trajectory_test
)

add_executable(
    pitch_motion_articulation_test
    tests/model/pitch_motion_articulation_test.cpp
)
add_executable(
    persistent_part_performance_trajectory_test
    tests/model/persistent_part_performance_trajectory_test.cpp
)

target_include_directories(
    pitch_motion_articulation_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    persistent_part_performance_trajectory_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
    NAME pitch_motion_articulation
    COMMAND pitch_motion_articulation_test
)
add_test(
    NAME persistent_part_performance_trajectory
    COMMAND persistent_part_performance_trajectory_test
)
''',
    encoding="utf-8",
)
