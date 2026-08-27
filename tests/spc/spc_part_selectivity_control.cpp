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
    std::size_t strong_count = 0;
    std::size_t rejected_count = 0;
};

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
            if (strong_persistent_part_transition(hypothesis)) {
                ++result.strong_count;
                continue;
            }
        } catch (const std::invalid_argument&) {
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

        const double observed_rate = static_cast<double>(observed.strong_transition_count) /
            static_cast<double>(observed.candidate_transition_count);
        const double control_rate = control.candidate_count == 0
            ? 0.0
            : static_cast<double>(control.strong_count) /
                static_cast<double>(control.candidate_count);

        std::cout
            << "SPC_PART_SELECTIVITY_CONTROL"
            << " observed_candidates=" << observed.candidate_transition_count
            << " observed_strong=" << observed.strong_transition_count
            << " observed_rate=" << observed_rate
            << " cross_voice_candidates=" << control.candidate_count
            << " cross_voice_strong=" << control.strong_count
            << " cross_voice_rejected=" << control.rejected_count
            << " cross_voice_rate=" << control_rate
            << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "spc_part_selectivity_control: " << error.what() << '\n';
        return 1;
    }
}
