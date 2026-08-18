#pragma once

#include "execution_semantic_dialect.h"

#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace vgmtooling::model {

enum class execution_semantic_scope_kind : std::uint8_t {
    global = 0,
    logical_track,
    physical_channel,
    fm_channel,
    dac_channel,
    psg_tone_channel,
    psg_noise_channel,
    song_slot,
    driver_queue,
};

enum class execution_timing_domain_kind : std::uint8_t {
    unspecified = 0,
    driver_update,
    vblank_update,
    pal_compensated_update,
    global_tempo_accumulator,
    track_tempo_divider,
    ym_timer_b,
    vgm_sample_clock,
    source_sample_clock,
};

struct execution_semantic_scope {
    execution_semantic_scope_kind kind = execution_semantic_scope_kind::global;
    // -1 means the whole scope family rather than one numbered lane/slot.
    std::int32_t index = -1;
    std::string detail;
};

struct execution_timing_domain {
    execution_timing_domain_kind kind = execution_timing_domain_kind::unspecified;
    std::optional<double> nominal_hz{};
    std::optional<std::uint32_t> integer_divider{};
    std::string detail;
};

struct scoped_execution_semantic_observation {
    node_id dialect = 0;
    execution_semantic_observation semantic;
    execution_semantic_scope scope;
    execution_timing_domain timing;
};

struct scoped_execution_dialect_capability_observation {
    execution_dialect_capability_observation capability;
    execution_semantic_scope scope;
    execution_timing_domain timing;
};

inline const char* execution_semantic_scope_kind_name(execution_semantic_scope_kind kind) noexcept {
    switch (kind) {
    case execution_semantic_scope_kind::global: return "global";
    case execution_semantic_scope_kind::logical_track: return "logical_track";
    case execution_semantic_scope_kind::physical_channel: return "physical_channel";
    case execution_semantic_scope_kind::fm_channel: return "fm_channel";
    case execution_semantic_scope_kind::dac_channel: return "dac_channel";
    case execution_semantic_scope_kind::psg_tone_channel: return "psg_tone_channel";
    case execution_semantic_scope_kind::psg_noise_channel: return "psg_noise_channel";
    case execution_semantic_scope_kind::song_slot: return "song_slot";
    case execution_semantic_scope_kind::driver_queue: return "driver_queue";
    }
    return "unknown";
}

inline const char* execution_timing_domain_kind_name(execution_timing_domain_kind kind) noexcept {
    switch (kind) {
    case execution_timing_domain_kind::unspecified: return "unspecified";
    case execution_timing_domain_kind::driver_update: return "driver_update";
    case execution_timing_domain_kind::vblank_update: return "vblank_update";
    case execution_timing_domain_kind::pal_compensated_update: return "pal_compensated_update";
    case execution_timing_domain_kind::global_tempo_accumulator: return "global_tempo_accumulator";
    case execution_timing_domain_kind::track_tempo_divider: return "track_tempo_divider";
    case execution_timing_domain_kind::ym_timer_b: return "ym_timer_b";
    case execution_timing_domain_kind::vgm_sample_clock: return "vgm_sample_clock";
    case execution_timing_domain_kind::source_sample_clock: return "source_sample_clock";
    }
    return "unknown";
}

inline void validate_execution_semantic_scope(const execution_semantic_scope& scope) {
    if (scope.index < -1)
        throw std::invalid_argument("execution semantic scope index must be -1 or nonnegative");
    if (scope.kind == execution_semantic_scope_kind::global && scope.index != -1)
        throw std::invalid_argument("global execution semantic scope cannot have a lane index");
}

inline void validate_execution_timing_domain(const execution_timing_domain& timing) {
    if (timing.nominal_hz.has_value() &&
        (!std::isfinite(*timing.nominal_hz) || *timing.nominal_hz <= 0.0)) {
        throw std::invalid_argument("execution timing nominal frequency must be finite and positive");
    }
    if (timing.integer_divider.has_value() && *timing.integer_divider == 0)
        throw std::invalid_argument("execution timing integer divider must be positive");
    if (timing.kind == execution_timing_domain_kind::unspecified &&
        (timing.nominal_hz.has_value() || timing.integer_divider.has_value())) {
        throw std::invalid_argument("unspecified timing cannot carry a concrete rate or divider");
    }
}

inline void append_execution_scope_attributes(
    node& stored,
    const execution_semantic_scope& scope,
    const execution_timing_domain& timing,
    evidence_status status,
    double confidence) {
    validate_execution_semantic_scope(scope);
    validate_execution_timing_domain(timing);

    stored.attributes.push_back({
        "semantic_scope",
        std::string{execution_semantic_scope_kind_name(scope.kind)},
        status,
        confidence,
        "",
    });
    if (scope.index >= 0) {
        stored.attributes.push_back({
            "semantic_scope_index",
            static_cast<std::int64_t>(scope.index),
            status,
            confidence,
            "",
        });
    }
    if (!scope.detail.empty()) {
        stored.attributes.push_back({
            "semantic_scope_detail",
            scope.detail,
            status,
            confidence,
            "",
        });
    }

    stored.attributes.push_back({
        "timing_domain",
        std::string{execution_timing_domain_kind_name(timing.kind)},
        status,
        confidence,
        "",
    });
    if (timing.nominal_hz.has_value()) {
        stored.attributes.push_back({
            "timing_nominal_hz",
            *timing.nominal_hz,
            status,
            confidence,
            "",
        });
    }
    if (timing.integer_divider.has_value()) {
        stored.attributes.push_back({
            "timing_integer_divider",
            static_cast<std::int64_t>(*timing.integer_divider),
            status,
            confidence,
            "",
        });
    }
    if (!timing.detail.empty()) {
        stored.attributes.push_back({
            "timing_detail",
            timing.detail,
            status,
            confidence,
            "",
        });
    }
}

inline node_id append_scoped_execution_semantic_observation(
    musical_execution_graph& graph,
    scoped_execution_semantic_observation observation) {
    const node* dialect = graph.find_node(observation.dialect);
    if (dialect == nullptr || !is_execution_dialect_identity_node(*dialect))
        throw std::invalid_argument("scoped execution semantic observation requires a dialect identity");

    validate_execution_semantic_scope(observation.scope);
    validate_execution_timing_domain(observation.timing);
    const evidence_status status = observation.semantic.status;
    const double confidence = observation.semantic.confidence;
    const std::string ancestry_source = observation.semantic.source;
    const std::string ancestry_detail = observation.semantic.detail;
    const node_id id = append_execution_semantic_observation(graph, std::move(observation.semantic));

    node* stored = graph.find_node(id);
    if (stored == nullptr)
        throw std::logic_error("new scoped execution semantic observation was not stored");
    append_execution_scope_attributes(*stored, observation.scope, observation.timing, status, confidence);
    link_execution_semantic_ancestry(
        graph,
        observation.dialect,
        id,
        status,
        confidence,
        ancestry_source,
        ancestry_detail);
    return id;
}

inline node_id append_scoped_execution_dialect_capability(
    musical_execution_graph& graph,
    scoped_execution_dialect_capability_observation observation) {
    validate_execution_semantic_scope(observation.scope);
    validate_execution_timing_domain(observation.timing);
    const evidence_status status = observation.capability.status;
    const double confidence = observation.capability.confidence;
    const node_id id = append_execution_dialect_capability(graph, std::move(observation.capability));

    node* stored = graph.find_node(id);
    if (stored == nullptr)
        throw std::logic_error("new scoped execution dialect capability was not stored");
    append_execution_scope_attributes(*stored, observation.scope, observation.timing, status, confidence);
    return id;
}

} // namespace vgmtooling::model
