#include "SNES_SPC.h"

#include "components/spc/spc_label_blind_corpus_features.h"
#include "components/spc/spc_runtime_sample_adapter.h"
#include "components/spc/spc_runtime_trace_recorder.h"
#include "components/spc/spc_runtime_trace_replay.h"
#include "components/spc/spc_snapshot.h"
#include "components/spc/spc_snapshot_graph_adapter.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

namespace {

constexpr std::size_t scalar_samples_per_second = 64000;
constexpr int playback_chunk_scalar_samples = 4096;

std::vector<std::uint8_t> read_binary(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("could not open SPC selectivity fixture");
    input.seekg(0, std::ios::end);
    const std::streamoff end = input.tellg();
    if (end < 0)
        throw std::runtime_error("could not determine SPC selectivity fixture size");
    input.seekg(0, std::ios::beg);
    if (static_cast<std::uint64_t>(end) >
        static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        throw std::runtime_error("SPC selectivity fixture is too large");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(end));
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input)
            throw std::runtime_error("could not read complete SPC selectivity fixture");
    }
    return bytes;
}

std::uint64_t parse_seconds(const char* text) {
    if (text == nullptr || *text == '\0')
        throw std::invalid_argument("seconds must be positive");
    char* end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (end == text || *end != '\0' || value == 0 || value > 60)
        throw std::invalid_argument("seconds must be in [1, 60]");
    return static_cast<std::uint64_t>(value);
}

struct cross_voice_context_result {
    std::size_t bundle_candidate_count = 0;
    std::size_t unique_outgoing_count = 0;
    std::size_t unique_incoming_count = 0;
    std::size_t bidirectionally_unique_count = 0;
    std::size_t left_flanked_count = 0;
    std::size_t right_flanked_count = 0;
    std::size_t two_sided_flanked_count = 0;
    std::size_t two_sided_unique_count = 0;
    std::size_t boundary_safe_bidirectionally_unique_count = 0;
    std::size_t boundary_safe_two_sided_unique_count = 0;
};

struct cross_voice_candidate_edge {
    node_id first = 0;
    node_id second = 0;
};

struct cross_voice_control_result {
    std::size_t candidate_count = 0;
    std::size_t inferred_count = 0;
    std::size_t no_hypothesis_count = 0;
    std::size_t strong_count = 0;
    std::size_t rejected_count = 0;

    std::size_t source_identity_support = 0;
    std::size_t temporal_adjacency_support = 0;
    std::size_t pitch_trajectory_support = 0;
    std::size_t physical_slot_support = 0;
    std::size_t identity_discontinuity_counter = 0;
    std::size_t simultaneous_conflict_counter = 0;

    std::size_t strong_source_identity_support = 0;
    std::size_t strong_temporal_adjacency_support = 0;
    std::size_t strong_pitch_trajectory_support = 0;
    std::size_t strong_physical_slot_support = 0;
    std::size_t strong_identity_discontinuity_counter = 0;
    std::size_t strong_source_temporal_pitch_bundle = 0;

    double strong_confidence_sum = 0.0;
    double strong_confidence_min = 1.0;
    double strong_confidence_max = 0.0;
};

bool has_evidence(
    const persistent_part_hypothesis& hypothesis,
    persistent_part_evidence_kind kind,
    persistent_part_evidence_polarity polarity) {
    for (const auto& evidence : hypothesis.evidence) {
        if (evidence.kind == kind && evidence.polarity == polarity)
            return true;
    }
    return false;
}

bool same_episode_time_basis(
    const time_coordinate& first,
    const time_coordinate& second) noexcept {
    return first.domain == second.domain &&
        first.tick_rate != 0 &&
        first.tick_rate == second.tick_rate &&
        first.loop_iteration == second.loop_iteration;
}

bool within_part_gap(
    const node& first,
    const node& second,
    const spc_part_continuity_policy& policy) noexcept {
    if (!first.active.has_value() || !first.active->end.has_value() ||
        !second.active.has_value())
        return false;
    const auto& end = *first.active->end;
    const auto& start = second.active->start;
    if (!same_episode_time_basis(end, start))
        return false;
    const std::int64_t gap = start.tick - end.tick;
    if (gap < 0)
        return false;
    const double gap_seconds = static_cast<double>(gap) /
        static_cast<double>(end.tick_rate);
    return gap_seconds <= policy.max_gap_seconds;
}

bool same_physical_voice(const node& first, const node& second) noexcept {
    const auto* first_voice = detail::spc_episode_physical_voice(first);
    const auto* second_voice = detail::spc_episode_physical_voice(second);
    return first_voice != nullptr && second_voice != nullptr &&
        *first_voice == *second_voice;
}

bool cross_voice_bundle_candidate(
    const musical_execution_graph& graph,
    const node& first,
    const node& second,
    const spc_part_continuity_policy& policy) {
    if (same_physical_voice(first, second) || !within_part_gap(first, second, policy))
        return false;

    try {
        const auto hypothesis = infer_spc_persistent_part(
            graph,
            first.id,
            second.id,
            "spc-cross-voice-context",
            policy);

        const bool source = has_evidence(
            hypothesis,
            persistent_part_evidence_kind::source_identity,
            persistent_part_evidence_polarity::supports);
        const bool temporal = has_evidence(
            hypothesis,
            persistent_part_evidence_kind::temporal_adjacency,
            persistent_part_evidence_polarity::supports);
        const bool pitch = has_evidence(
            hypothesis,
            persistent_part_evidence_kind::pitch_trajectory_continuity,
            persistent_part_evidence_polarity::supports);
        const bool discontinuity = has_evidence(
            hypothesis,
            persistent_part_evidence_kind::identity_discontinuity,
            persistent_part_evidence_polarity::counters);
        const bool overlap = has_evidence(
            hypothesis,
            persistent_part_evidence_kind::simultaneous_conflict,
            persistent_part_evidence_polarity::counters);

        // This is deliberately the exact bundle that the prior ablation proved
        // non-selective when used pairwise. The measurement asks whether graph
        // context can discriminate it without silently treating context as a new
        // independent evidence domain.
        return source && temporal && pitch &&
            !discontinuity && !overlap &&
            hypothesis.proposed_confidence >= persistent_part_trajectory_link_threshold &&
            hypothesis.confidence < persistent_part_trajectory_link_threshold;
    } catch (const std::invalid_argument&) {
        return false;
    }
}

const node* nearest_same_voice_predecessor(
    const std::vector<const node*>& episodes,
    const node& target) noexcept {
    if (!target.active.has_value())
        return nullptr;
    const auto* target_voice = detail::spc_episode_physical_voice(target);
    if (target_voice == nullptr)
        return nullptr;

    const node* best = nullptr;
    std::int64_t best_end = std::numeric_limits<std::int64_t>::min();
    for (const node* candidate : episodes) {
        if (candidate == nullptr || candidate->id == target.id ||
            !candidate->active.has_value() || !candidate->active->end.has_value())
            continue;
        const auto* voice = detail::spc_episode_physical_voice(*candidate);
        if (voice == nullptr || *voice != *target_voice)
            continue;
        const auto& end = *candidate->active->end;
        const auto& start = target.active->start;
        if (!same_episode_time_basis(end, start) || end.tick > start.tick)
            continue;
        if (end.tick > best_end || (end.tick == best_end &&
            (best == nullptr || candidate->id > best->id))) {
            best = candidate;
            best_end = end.tick;
        }
    }
    return best;
}

const node* nearest_same_voice_successor(
    const std::vector<const node*>& episodes,
    const node& target) noexcept {
    if (!target.active.has_value() || !target.active->end.has_value())
        return nullptr;
    const auto* target_voice = detail::spc_episode_physical_voice(target);
    if (target_voice == nullptr)
        return nullptr;

    const node* best = nullptr;
    std::int64_t best_start = std::numeric_limits<std::int64_t>::max();
    for (const node* candidate : episodes) {
        if (candidate == nullptr || candidate->id == target.id ||
            !candidate->active.has_value())
            continue;
        const auto* voice = detail::spc_episode_physical_voice(*candidate);
        if (voice == nullptr || *voice != *target_voice)
            continue;
        const auto& end = *target.active->end;
        const auto& start = candidate->active->start;
        if (!same_episode_time_basis(end, start) || start.tick < end.tick)
            continue;
        if (start.tick < best_start || (start.tick == best_start &&
            (best == nullptr || candidate->id < best->id))) {
            best = candidate;
            best_start = start.tick;
        }
    }
    return best;
}

bool strong_same_voice_link(
    const musical_execution_graph& graph,
    const node* first,
    const node* second,
    const spc_part_continuity_policy& policy) {
    if (first == nullptr || second == nullptr || !same_physical_voice(*first, *second))
        return false;
    try {
        return strong_persistent_part_transition(infer_spc_persistent_part(
            graph,
            first->id,
            second->id,
            "spc-cross-voice-context-flank",
            policy));
    } catch (const std::invalid_argument&) {
        return false;
    }
}

bool handoff_is_capture_boundary_safe(
    const node& first,
    const node& second,
    const spc_part_continuity_policy& policy,
    std::uint64_t execution_seconds) noexcept {
    if (!first.active.has_value() || !first.active->end.has_value() ||
        !second.active.has_value())
        return false;

    const auto& first_end = *first.active->end;
    const auto& second_start = second.active->start;
    if (!same_episode_time_basis(first_end, second_start))
        return false;

    const auto margin_ticks = static_cast<std::int64_t>(
        policy.max_gap_seconds * static_cast<double>(first_end.tick_rate));
    const auto capture_end_tick = static_cast<std::int64_t>(
        execution_seconds * first_end.tick_rate);

    // Uniqueness can only be trusted against truncation when the complete
    // association horizon exists before the target and after the source.
    // Otherwise an unseen earlier/later competitor could make one-in/one-out
    // uniqueness a capture-window artifact.
    return second_start.tick >= margin_ticks &&
        first_end.tick <= capture_end_tick - margin_ticks;
}

cross_voice_context_result measure_cross_voice_context(
    const musical_execution_graph& graph,
    const spc_part_continuity_policy& policy,
    std::uint64_t execution_seconds) {
    cross_voice_context_result result;
    const auto episodes = graph.nodes_of_kind(node_kind::voice_instance);
    std::vector<cross_voice_candidate_edge> edges;

    // Multi-trajectory tracking / voice-separation pressure: preserve all
    // plausible cross-voice edges first, then ask whether local trajectory
    // context makes an edge structurally unique. Pairwise similarity alone has
    // already failed its corpus null and remains capped.
    for (const node* first : episodes) {
        if (first == nullptr || !spc_episode_allows_part_successor(*first))
            continue;
        for (const node* second : episodes) {
            if (second == nullptr || first->id == second->id)
                continue;
            if (cross_voice_bundle_candidate(graph, *first, *second, policy))
                edges.push_back({first->id, second->id});
        }
    }

    result.bundle_candidate_count = edges.size();
    std::vector<std::size_t> outgoing(graph.nodes().size() + 1u, 0u);
    std::vector<std::size_t> incoming(graph.nodes().size() + 1u, 0u);
    for (const auto& edge : edges) {
        ++outgoing[static_cast<std::size_t>(edge.first)];
        ++incoming[static_cast<std::size_t>(edge.second)];
    }

    for (const auto& edge : edges) {
        const node* first = graph.find_node(edge.first);
        const node* second = graph.find_node(edge.second);
        if (first == nullptr || second == nullptr)
            throw std::logic_error("SPC handoff context edge references an unknown episode");

        const bool unique_out = outgoing[static_cast<std::size_t>(edge.first)] == 1u;
        const bool unique_in = incoming[static_cast<std::size_t>(edge.second)] == 1u;
        result.unique_outgoing_count += unique_out ? 1u : 0u;
        result.unique_incoming_count += unique_in ? 1u : 0u;
        result.bidirectionally_unique_count += unique_out && unique_in ? 1u : 0u;

        const node* predecessor = nearest_same_voice_predecessor(episodes, *first);
        const node* successor = nearest_same_voice_successor(episodes, *second);
        const bool left_flanked = strong_same_voice_link(
            graph, predecessor, first, policy);
        const bool right_flanked = strong_same_voice_link(
            graph, second, successor, policy);

        result.left_flanked_count += left_flanked ? 1u : 0u;
        result.right_flanked_count += right_flanked ? 1u : 0u;
        const bool boundary_safe = handoff_is_capture_boundary_safe(
            *first,
            *second,
            policy,
            execution_seconds);
        const bool bidirectionally_unique = unique_out && unique_in;
        const bool two_sided_unique =
            left_flanked && right_flanked && bidirectionally_unique;

        result.two_sided_flanked_count += left_flanked && right_flanked ? 1u : 0u;
        result.two_sided_unique_count += two_sided_unique ? 1u : 0u;
        result.boundary_safe_bidirectionally_unique_count +=
            boundary_safe && bidirectionally_unique ? 1u : 0u;
        result.boundary_safe_two_sided_unique_count +=
            boundary_safe && two_sided_unique ? 1u : 0u;
    }

    return result;
}

void observe_hypothesis_evidence(
    const persistent_part_hypothesis& hypothesis,
    bool strong,
    cross_voice_control_result& result) {
    const bool source = has_evidence(
        hypothesis,
        persistent_part_evidence_kind::source_identity,
        persistent_part_evidence_polarity::supports);
    const bool temporal = has_evidence(
        hypothesis,
        persistent_part_evidence_kind::temporal_adjacency,
        persistent_part_evidence_polarity::supports);
    const bool pitch = has_evidence(
        hypothesis,
        persistent_part_evidence_kind::pitch_trajectory_continuity,
        persistent_part_evidence_polarity::supports);
    const bool slot = has_evidence(
        hypothesis,
        persistent_part_evidence_kind::physical_slot_continuity,
        persistent_part_evidence_polarity::supports);
    const bool discontinuity = has_evidence(
        hypothesis,
        persistent_part_evidence_kind::identity_discontinuity,
        persistent_part_evidence_polarity::counters);
    const bool overlap = has_evidence(
        hypothesis,
        persistent_part_evidence_kind::simultaneous_conflict,
        persistent_part_evidence_polarity::counters);

    result.source_identity_support += source ? 1u : 0u;
    result.temporal_adjacency_support += temporal ? 1u : 0u;
    result.pitch_trajectory_support += pitch ? 1u : 0u;
    result.physical_slot_support += slot ? 1u : 0u;
    result.identity_discontinuity_counter += discontinuity ? 1u : 0u;
    result.simultaneous_conflict_counter += overlap ? 1u : 0u;

    if (!strong)
        return;

    result.strong_source_identity_support += source ? 1u : 0u;
    result.strong_temporal_adjacency_support += temporal ? 1u : 0u;
    result.strong_pitch_trajectory_support += pitch ? 1u : 0u;
    result.strong_physical_slot_support += slot ? 1u : 0u;
    result.strong_identity_discontinuity_counter += discontinuity ? 1u : 0u;
    result.strong_source_temporal_pitch_bundle += source && temporal && pitch ? 1u : 0u;
    result.strong_confidence_sum += hypothesis.confidence;
    result.strong_confidence_min = std::min(result.strong_confidence_min, hypothesis.confidence);
    result.strong_confidence_max = std::max(result.strong_confidence_max, hypothesis.confidence);
}

cross_voice_control_result measure_cross_voice_control(
    const musical_execution_graph& graph,
    const spc_part_continuity_policy& policy) {
    cross_voice_control_result result;
    const auto episodes = graph.nodes_of_kind(node_kind::voice_instance);

    for (const node* first : episodes) {
        if (first == nullptr || !first->active.has_value() || !first->active->end.has_value())
            continue;
        if (!spc_episode_allows_part_successor(*first))
            continue;
        const auto* first_voice = detail::spc_episode_physical_voice(*first);
        if (first_voice == nullptr)
            continue;

        const auto& first_end = *first->active->end;
        const node* best = nullptr;
        std::int64_t best_gap = std::numeric_limits<std::int64_t>::max();

        for (const node* second : episodes) {
            if (second == nullptr || second == first || !second->active.has_value())
                continue;
            const auto* second_voice = detail::spc_episode_physical_voice(*second);
            if (second_voice == nullptr || *second_voice == *first_voice)
                continue;

            const auto& second_start = second->active->start;
            if (second_start.domain != first_end.domain ||
                second_start.tick_rate == 0 ||
                second_start.tick_rate != first_end.tick_rate ||
                second_start.loop_iteration != first_end.loop_iteration)
                continue;

            const std::int64_t gap = second_start.tick - first_end.tick;
            if (gap < 0)
                continue;
            const double gap_seconds = static_cast<double>(gap) /
                static_cast<double>(first_end.tick_rate);
            if (gap_seconds > policy.max_gap_seconds)
                continue;
            if (gap < best_gap || (gap == best_gap && (best == nullptr || second->id < best->id))) {
                best_gap = gap;
                best = second;
            }
        }

        if (best == nullptr)
            continue;

        ++result.candidate_count;
        try {
            const auto hypothesis = infer_spc_persistent_part(
                graph,
                first->id,
                best->id,
                "spc-cross-voice-null",
                policy);
            ++result.inferred_count;
            const bool strong = strong_persistent_part_transition(hypothesis);
            observe_hypothesis_evidence(hypothesis, strong, result);
            if (strong) {
                ++result.strong_count;
                continue;
            }
        } catch (const std::invalid_argument&) {
            ++result.no_hypothesis_count;
            // Rejection is the expected null outcome when evidence is selective.
        }
        ++result.rejected_count;
    }

    return result;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            std::cerr << "usage: spc_part_selectivity_control <input.spc> <seconds>\n";
            return 2;
        }

        const auto bytes = read_binary(argv[1]);
        if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<long>::max()))
            throw std::runtime_error("SPC selectivity fixture exceeds snes_spc load size range");
        const std::uint64_t seconds = parse_seconds(argv[2]);
        const auto snapshot = parse_spc_snapshot(bytes.data(), bytes.size());

        SNES_SPC emulator;
        if (const char* error = emulator.init())
            throw std::runtime_error(std::string{"snes_spc init failed: "} + error);
        spc_runtime_trace_recorder recorder;
        emulator.set_runtime_instrumentation_sink(&recorder);
        if (const char* error = emulator.load_spc(bytes.data(), static_cast<long>(bytes.size())))
            throw std::runtime_error(std::string{"snes_spc load failed: "} + error);

        std::uint64_t remaining = seconds * scalar_samples_per_second;
        while (remaining != 0) {
            const int block = static_cast<int>(std::min<std::uint64_t>(
                remaining,
                static_cast<std::uint64_t>(playback_chunk_scalar_samples)));
            if ((block & 1) != 0)
                throw std::logic_error("SPC selectivity playback block must be stereo-even");
            if (const char* error = emulator.play(block, nullptr))
                throw std::runtime_error(std::string{"snes_spc execution failed: "} + error);
            recorder.flush_window();
            remaining -= static_cast<std::uint64_t>(block);
        }
        const auto trace = recorder.finish();

        musical_execution_graph graph;
        const auto snapshot_graph = materialize_spc_snapshot(graph, snapshot, "spc-selectivity-fixture");
        auto runtime = begin_spc_runtime_voice_trace(
            graph,
            snapshot_graph,
            "spc-selectivity-runtime",
            to_flags(provenance_flag::runtime_capture));
        auto samples = begin_spc_runtime_sample_graph(
            "spc-selectivity-runtime",
            to_flags(provenance_flag::runtime_capture));
        const auto replay = replay_spc_runtime_trace(
            graph,
            runtime,
            samples,
            snapshot,
            trace);
        if (replay.continuity_breaks != 0)
            throw std::runtime_error("SPC selectivity control requires a contiguous runtime trace");

        spc_part_continuity_policy policy;
        const auto control = measure_cross_voice_control(graph, policy);
        const auto context = measure_cross_voice_context(graph, policy, seconds);
        const auto observed = extract_spc_label_blind_corpus_features(
            graph,
            "spc-selectivity-runtime",
            policy);

        // A broad corpus panel may legitimately contain no admitted adjacent
        // candidates. That is an earned null, not a harness failure; the fixed
        // four-fixture runtime-pressure gate owns positive-path coverage.
        if (observed.strong_transition_count + observed.rejected_transition_count !=
            observed.candidate_transition_count)
            throw std::logic_error("SPC selectivity observed accounting is inconsistent");
        if (control.strong_count + control.rejected_count != control.candidate_count)
            throw std::logic_error("SPC selectivity null accounting is inconsistent");
        if (control.inferred_count + control.no_hypothesis_count != control.candidate_count)
            throw std::logic_error("SPC selectivity inference accounting is inconsistent");
        if (control.strong_physical_slot_support != 0)
            throw std::logic_error("cross-voice null cannot contain physical-slot continuity support");
        if (context.unique_outgoing_count > context.bundle_candidate_count ||
            context.unique_incoming_count > context.bundle_candidate_count ||
            context.bidirectionally_unique_count > context.bundle_candidate_count ||
            context.left_flanked_count > context.bundle_candidate_count ||
            context.right_flanked_count > context.bundle_candidate_count ||
            context.two_sided_flanked_count > context.bundle_candidate_count ||
            context.two_sided_unique_count > context.bundle_candidate_count ||
            context.boundary_safe_bidirectionally_unique_count > context.bundle_candidate_count ||
            context.boundary_safe_two_sided_unique_count > context.bundle_candidate_count)
            throw std::logic_error("SPC handoff-context accounting exceeds candidate count");
        if (context.two_sided_unique_count > context.two_sided_flanked_count ||
            context.two_sided_unique_count > context.bidirectionally_unique_count ||
            context.boundary_safe_bidirectionally_unique_count >
                context.bidirectionally_unique_count ||
            context.boundary_safe_two_sided_unique_count >
                context.boundary_safe_bidirectionally_unique_count ||
            context.boundary_safe_two_sided_unique_count >
                context.two_sided_unique_count)
            throw std::logic_error("SPC handoff-context intersection accounting is inconsistent");

        const double observed_rate = observed.candidate_transition_count == 0
            ? 0.0
            : static_cast<double>(observed.strong_transition_count) /
                static_cast<double>(observed.candidate_transition_count);
        const double control_rate = control.candidate_count == 0
            ? 0.0
            : static_cast<double>(control.strong_count) /
                static_cast<double>(control.candidate_count);
        const double strong_confidence_mean = control.strong_count == 0
            ? 0.0
            : control.strong_confidence_sum / static_cast<double>(control.strong_count);
        const double strong_confidence_min = control.strong_count == 0
            ? 0.0 : control.strong_confidence_min;

        std::cout
            << "SPC_PART_SELECTIVITY_CONTROL"
            << " observed_candidates=" << observed.candidate_transition_count
            << " observed_strong=" << observed.strong_transition_count
            << " observed_rate=" << observed_rate
            << " observed_empty=" << (observed.candidate_transition_count == 0 ? 1 : 0)
            << " cross_voice_candidates=" << control.candidate_count
            << " cross_voice_inferred=" << control.inferred_count
            << " cross_voice_no_hypothesis=" << control.no_hypothesis_count
            << " cross_voice_strong=" << control.strong_count
            << " cross_voice_rejected=" << control.rejected_count
            << " cross_voice_rate=" << control_rate
            << " evidence_source=" << control.source_identity_support
            << " evidence_temporal=" << control.temporal_adjacency_support
            << " evidence_pitch=" << control.pitch_trajectory_support
            << " evidence_slot=" << control.physical_slot_support
            << " counter_discontinuity=" << control.identity_discontinuity_counter
            << " counter_overlap=" << control.simultaneous_conflict_counter
            << " strong_source=" << control.strong_source_identity_support
            << " strong_temporal=" << control.strong_temporal_adjacency_support
            << " strong_pitch=" << control.strong_pitch_trajectory_support
            << " strong_discontinuity=" << control.strong_identity_discontinuity_counter
            << " strong_source_temporal_pitch=" << control.strong_source_temporal_pitch_bundle
            << " strong_confidence_min=" << strong_confidence_min
            << " strong_confidence_mean=" << strong_confidence_mean
            << " strong_confidence_max=" << control.strong_confidence_max
            << " handoff_bundle_candidates=" << context.bundle_candidate_count
            << " handoff_unique_outgoing=" << context.unique_outgoing_count
            << " handoff_unique_incoming=" << context.unique_incoming_count
            << " handoff_bidirectional_unique=" << context.bidirectionally_unique_count
            << " handoff_left_flanked=" << context.left_flanked_count
            << " handoff_right_flanked=" << context.right_flanked_count
            << " handoff_two_sided_flanked=" << context.two_sided_flanked_count
            << " handoff_two_sided_unique=" << context.two_sided_unique_count
            << " handoff_boundary_safe_bidirectional_unique="
            << context.boundary_safe_bidirectionally_unique_count
            << " handoff_boundary_safe_two_sided_unique="
            << context.boundary_safe_two_sided_unique_count
            << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "spc_part_selectivity_control: " << error.what() << '\n';
        return 1;
    }
}
