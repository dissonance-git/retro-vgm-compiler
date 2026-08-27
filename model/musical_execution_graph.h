#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace vgmtooling::model {

using node_id = std::uint64_t;
using edge_id = std::uint64_t;

enum class evidence_status : std::uint8_t {
    exact = 0,
    derived = 1,
    hypothesis = 2,
};

enum class semantic_layer : std::uint8_t {
    source_representation = 0,
    authored_program,
    driver_execution,
    synthesis,
    musical_performance,
    musical_structure,
    acoustic_realization,
    auditory_interpretation,
    listener_response,
    musicological_context,
};

enum class flow_kind : std::uint8_t {
    none = 0,
    event,
    value,
    control,
    stream,
};

enum class node_kind : std::uint8_t {
    source_object = 0,
    section,
    pattern,
    part,
    musical_event,
    musical_relation,
    instrument_definition,
    synthesis_object,
    voice_instance,
    physical_slot,
    parameter,
    sample_buffer,
    effect,
    bus,
    acoustic_contribution,
    auditory_event,
    auditory_stream,
    projection,
    logical_process,
    program_point,
    execution_trace,
    trace_event,
};

enum class edge_kind : std::uint8_t {
    contains = 0,
    references,
    causes,
    schedules,
    instantiates,
    realizes,
    occupies,
    controls,
    routes_to,
    transforms,
    contributes_to,
    groups_into,
    derived_from,
    same_identity_as,
    repeats,
    projects_to,
    control_flows_to,
    maps_time_to,
};

enum class time_domain : std::uint8_t {
    source = 0,
    authored,
    driver,
    device,
    sample,
    acoustic,
    perceptual,
};

enum class provenance_flag : std::uint32_t {
    none = 0,
    runtime_capture = 1u << 0u,
    transformed = 1u << 1u,
    incomplete = 1u << 2u,
    suspected_artifact = 1u << 3u,
    external_annotation = 1u << 4u,
};

using provenance_flags = std::uint32_t;

constexpr provenance_flags to_flags(provenance_flag flag) noexcept {
    return static_cast<provenance_flags>(flag);
}

constexpr provenance_flags operator|(provenance_flag lhs, provenance_flag rhs) noexcept {
    return to_flags(lhs) | to_flags(rhs);
}

constexpr provenance_flags operator|(provenance_flags lhs, provenance_flag rhs) noexcept {
    return lhs | to_flags(rhs);
}

constexpr bool has_flag(provenance_flags flags, provenance_flag flag) noexcept {
    return (flags & to_flags(flag)) != 0;
}

// Exact integer time coordinate in a declared clock domain.
//
// `tick_rate` is units per second when that mapping is known. A value of zero
// means the native clock is intentionally left uninterpreted. Do not silently
// convert one clock domain into another; an adapter must supply that mapping.
struct time_coordinate {
    time_domain domain = time_domain::source;
    std::int64_t tick = 0;
    std::uint64_t tick_rate = 0;
    std::int64_t loop_iteration = 0;

    friend bool operator==(const time_coordinate& lhs, const time_coordinate& rhs) noexcept {
        return lhs.domain == rhs.domain && lhs.tick == rhs.tick && lhs.tick_rate == rhs.tick_rate &&
               lhs.loop_iteration == rhs.loop_iteration;
    }

    friend bool operator!=(const time_coordinate& lhs, const time_coordinate& rhs) noexcept {
        return !(lhs == rhs);
    }
};

struct time_span {
    time_coordinate start{};
    std::optional<time_coordinate> end{};
};

// Explicit relation between two clock domains. Mappings are normally
// piecewise: tempo changes, scheduler jitter, resampling and score-performance
// alignment should create additional mappings rather than flattening time into
// one global rate.
struct time_mapping {
    time_span from{};
    time_span to{};
};

// Evidence status and observation quality are deliberately orthogonal.
//
// A register write can be exact with respect to a VGM file while the VGM is
// still a runtime capture that begins late or contains a transformed trace.
struct provenance_ref {
    evidence_status status = evidence_status::exact;
    double confidence = 1.0;
    std::string source;
    std::optional<std::uint64_t> byte_offset{};
    std::string detail;
    provenance_flags flags = to_flags(provenance_flag::none);
};

using attribute_value = std::variant<bool, std::int64_t, std::uint64_t, double, std::string>;

struct attribute {
    std::string key;
    attribute_value value;
    evidence_status status = evidence_status::exact;
    double confidence = 1.0;
    std::string unit;
};

struct node {
    node_id id = 0;
    node_kind kind = node_kind::source_object;
    semantic_layer layer = semantic_layer::source_representation;
    flow_kind flow = flow_kind::none;
    std::string label;
    std::optional<time_span> active{};
    std::vector<attribute> attributes;
    std::vector<provenance_ref> provenance;
};

struct edge {
    edge_id id = 0;
    edge_kind kind = edge_kind::contains;
    node_id from = 0;
    node_id to = 0;
    std::optional<time_span> active{};
    std::vector<provenance_ref> provenance;
    std::vector<attribute> attributes;
    std::optional<time_mapping> time_map{};
};

class musical_execution_graph {
public:
    node_id add_node(node value) {
        validate_span(value.active);
        value.id = next_node_id_++;
        nodes_.push_back(std::move(value));
        outgoing_edge_ids_.emplace_back();
        incoming_edge_ids_.emplace_back();
        return nodes_.back().id;
    }

    edge_id add_edge(edge value) {
        if (find_node(value.from) == nullptr || find_node(value.to) == nullptr) {
            throw std::invalid_argument("musical execution edge references an unknown node");
        }

        validate_span(value.active);
        if (value.kind == edge_kind::maps_time_to) {
            if (!value.time_map.has_value()) {
                throw std::invalid_argument("time-mapping edge requires an explicit time mapping");
            }
            validate_time_mapping(*value.time_map);
        } else if (value.time_map.has_value()) {
            throw std::invalid_argument("explicit time mapping requires a maps_time_to edge");
        }

        const node_id from = value.from;
        const node_id to = value.to;
        value.id = next_edge_id_++;
        edges_.push_back(std::move(value));
        const edge_id id = edges_.back().id;
        outgoing_edge_ids_[static_cast<std::size_t>(from - 1u)].push_back(id);
        incoming_edge_ids_[static_cast<std::size_t>(to - 1u)].push_back(id);
        return id;
    }

    // IDs are append-only, 1-based dense positions. Preserve that identity
    // directly instead of linearly rescanning a runtime-sized graph.
    const node* find_node(node_id id) const noexcept {
        if (id == 0 || id > nodes_.size())
            return nullptr;
        const node& value = nodes_[static_cast<std::size_t>(id - 1u)];
        return value.id == id ? &value : nullptr;
    }

    node* find_node(node_id id) noexcept {
        if (id == 0 || id > nodes_.size())
            return nullptr;
        node& value = nodes_[static_cast<std::size_t>(id - 1u)];
        return value.id == id ? &value : nullptr;
    }

    const edge* find_edge(edge_id id) const noexcept {
        if (id == 0 || id > edges_.size())
            return nullptr;
        const edge& value = edges_[static_cast<std::size_t>(id - 1u)];
        return value.id == id ? &value : nullptr;
    }

    std::vector<const node*> nodes_of_kind(node_kind kind) const {
        std::vector<const node*> result;
        for (const auto& value : nodes_) {
            if (value.kind == kind) {
                result.push_back(&value);
            }
        }
        return result;
    }

    std::vector<const edge*> edges_from(node_id from, std::optional<edge_kind> kind = std::nullopt) const {
        std::vector<const edge*> result;
        if (from == 0 || from > outgoing_edge_ids_.size())
            return result;

        const auto& ids = outgoing_edge_ids_[static_cast<std::size_t>(from - 1u)];
        result.reserve(ids.size());
        for (edge_id id : ids) {
            const edge& value = edges_[static_cast<std::size_t>(id - 1u)];
            if (!kind.has_value() || value.kind == *kind)
                result.push_back(&value);
        }
        return result;
    }

    std::vector<const edge*> edges_to(node_id to, std::optional<edge_kind> kind = std::nullopt) const {
        std::vector<const edge*> result;
        if (to == 0 || to > incoming_edge_ids_.size())
            return result;

        const auto& ids = incoming_edge_ids_[static_cast<std::size_t>(to - 1u)];
        result.reserve(ids.size());
        for (edge_id id : ids) {
            const edge& value = edges_[static_cast<std::size_t>(id - 1u)];
            if (!kind.has_value() || value.kind == *kind)
                result.push_back(&value);
        }
        return result;
    }

    const std::vector<node>& nodes() const noexcept { return nodes_; }
    const std::vector<edge>& edges() const noexcept { return edges_; }

private:
    static void validate_span(const std::optional<time_span>& span) {
        if (!span.has_value()) {
            return;
        }
        validate_span(*span);
    }

    static void validate_span(const time_span& span) {
        if (span.end.has_value() && span.end->domain != span.start.domain) {
            throw std::invalid_argument("time span cannot cross clock domains");
        }
    }

    static void validate_time_mapping(const time_mapping& mapping) {
        validate_span(mapping.from);
        validate_span(mapping.to);
    }

    node_id next_node_id_ = 1;
    edge_id next_edge_id_ = 1;
    std::vector<node> nodes_;
    std::vector<edge> edges_;
    std::vector<std::vector<edge_id>> outgoing_edge_ids_;
    std::vector<std::vector<edge_id>> incoming_edge_ids_;
};

} // namespace vgmtooling::model
