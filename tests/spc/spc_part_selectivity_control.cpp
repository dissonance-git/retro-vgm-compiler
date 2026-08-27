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
        const auto observed = extract_spc_label_blind_corpus_features(
            graph,
            "spc-selectivity-runtime",
            policy);

        if (observed.candidate_transition_count == 0)
            throw std::runtime_error("SPC selectivity fixture produced no admitted adjacent candidates");
        if (observed.strong_transition_count + observed.rejected_transition_count !=
            observed.candidate_transition_count)
            throw std::logic_error("SPC selectivity observed accounting is inconsistent");
        if (control.strong_count + control.rejected_count != control.candidate_count)
            throw std::logic_error("SPC selectivity null accounting is inconsistent");
        if (control.inferred_count + control.no_hypothesis_count != control.candidate_count)
            throw std::logic_error("SPC selectivity inference accounting is inconsistent");
        if (control.strong_physical_slot_support != 0)
            throw std::logic_error("cross-voice null cannot contain physical-slot continuity support");

        const double observed_rate = static_cast<double>(observed.strong_transition_count) /
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
            << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "spc_part_selectivity_control: " << error.what() << '\n';
        return 1;
    }
}
