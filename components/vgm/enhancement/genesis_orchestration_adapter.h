#pragma once

#include "genesis_part_evidence.h"
#include "../../../model/orchestration_transition_hypothesis.h"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace gameaudio::vgm {

inline bool genesis_orchestration_span_overlap(
    const vgmtooling::model::time_span& first,
    const vgmtooling::model::time_span& second) noexcept {
    using namespace vgmtooling::model;
    if (!first.end.has_value() || !second.end.has_value())
        return false;
    if (!part_role_same_time_basis(first.start, second.start) ||
        !part_role_same_time_basis(*first.end, *second.end)) {
        return false;
    }
    return first.start.tick < second.end->tick && second.start.tick < first.end->tick;
}

inline std::string genesis_orchestration_hex(std::uint64_t value) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << value;
    return stream.str();
}

struct genesis_part_orchestration_summary {
    std::vector<vgmtooling::model::node_id> episode_ids;
    std::set<std::uint64_t> fm_program_fingerprints;
    std::optional<double> register_center_log2_relative{};
    bool all_relevant_fm_episodes_have_program_identity = true;
};

inline genesis_part_orchestration_summary summarize_genesis_part_orchestration(
    const vgmtooling::model::musical_execution_graph& graph,
    const vgmtooling::model::musical_part_role_hypothesis& role) {
    using namespace vgmtooling::model;

    const node* part = graph.find_node(role.part_id);
    if (part == nullptr || !is_persistent_musical_part_node(*part))
        throw std::invalid_argument("Genesis orchestration summary requires a persistent musical part");

    genesis_part_orchestration_summary result;
    std::vector<double> log_pitch;

    for (const edge* relation : graph.edges_to(role.part_id, edge_kind::groups_into)) {
        const node* episode = graph.find_node(relation->from);
        if (episode == nullptr || episode->kind != node_kind::voice_instance ||
            !episode->active.has_value() ||
            !genesis_orchestration_span_overlap(*episode->active, role.active)) {
            continue;
        }

        const attribute* family_item = find_genesis_part_attribute(*episode, "device_family");
        const auto* family = family_item == nullptr
            ? nullptr
            : std::get_if<std::string>(&family_item->value);
        if (family == nullptr || *family != "YM2612")
            continue;

        result.episode_ids.push_back(episode->id);

        const attribute* program_item = find_genesis_part_attribute(
            *episode,
            "instrument_program_fingerprint");
        const auto* program = program_item == nullptr
            ? nullptr
            : std::get_if<std::uint64_t>(&program_item->value);
        if (program == nullptr) {
            result.all_relevant_fm_episodes_have_program_identity = false;
        } else {
            result.fm_program_fingerprints.insert(*program);
        }

        const node* onset = genesis_episode_onset_event(graph, episode->id);
        if (onset != nullptr) {
            const auto coordinate = genesis_relative_pitch_coordinate(*onset);
            if (coordinate.has_value() && *coordinate > 0.0)
                log_pitch.push_back(std::log2(*coordinate));
        }
    }

    if (!log_pitch.empty()) {
        double sum = 0.0;
        for (double value : log_pitch)
            sum += value;
        result.register_center_log2_relative = sum / static_cast<double>(log_pitch.size());
    }
    return result;
}

inline vgmtooling::model::part_orchestration_state make_genesis_part_orchestration_state(
    const vgmtooling::model::musical_execution_graph& graph,
    const vgmtooling::model::musical_part_role_hypothesis& role,
    std::string source) {
    using namespace vgmtooling::model;
    if (source.empty())
        throw std::invalid_argument("Genesis orchestration state requires a source");

    const auto summary = summarize_genesis_part_orchestration(graph, role);
    std::optional<orchestration_realization> realization;
    if (summary.all_relevant_fm_episodes_have_program_identity &&
        summary.fm_program_fingerprints.size() == 1) {
        realization = orchestration_realization{
            "ym2612_program_fingerprint",
            genesis_orchestration_hex(*summary.fm_program_fingerprints.begin()),
            evidence_status::derived,
            1.0,
            source,
        };
    }

    return make_part_orchestration_state(
        role,
        std::move(realization),
        summary.register_center_log2_relative,
        summary.register_center_log2_relative.has_value()
            ? std::string{"genesis_device_relative_log2_pitch"}
            : std::string{});
}

} // namespace gameaudio::vgm
