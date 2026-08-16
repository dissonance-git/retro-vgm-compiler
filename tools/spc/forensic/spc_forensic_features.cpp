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
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef RETRO_VGM_COMPILER_FORENSIC_COMMIT
#define RETRO_VGM_COMPILER_FORENSIC_COMMIT "unknown"
#endif

#ifndef SNES_SPC_FORENSIC_COMMIT
#define SNES_SPC_FORENSIC_COMMIT "unknown"
#endif

#ifndef SNES_SPC_FORENSIC_PATCH_CONTRACT
#define SNES_SPC_FORENSIC_PATCH_CONTRACT "unknown"
#endif

namespace {

using gameaudio::spc::spc_label_blind_corpus_features;
using gameaudio::spc::spc_runtime_ram_write_origin;
using gameaudio::spc::spc_runtime_trace;
using vgmtooling::model::evidence_status;
using vgmtooling::model::part_motif_profile;

constexpr std::uint64_t spc_clock_rate = 1024000;
constexpr std::size_t scalar_samples_per_second = 64000;
constexpr int playback_chunk_scalar_samples = 4096;

const char* evidence_status_name(evidence_status status) noexcept {
    switch (status) {
    case evidence_status::exact:
        return "exact";
    case evidence_status::derived:
        return "derived";
    case evidence_status::hypothesis:
        return "hypothesis";
    }
    return "unknown";
}

std::vector<std::uint8_t> read_binary(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("could not open SPC input");

    input.seekg(0, std::ios::end);
    const std::streamoff end = input.tellg();
    if (end < 0)
        throw std::runtime_error("could not determine SPC input size");
    input.seekg(0, std::ios::beg);

    const auto size = static_cast<std::uint64_t>(end);
    if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
        throw std::runtime_error("SPC input is too large for this process");

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        input.read(
            reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
        if (!input)
            throw std::runtime_error("could not read complete SPC input");
    }
    return bytes;
}

std::uint64_t parse_seconds(const char* text) {
    if (text == nullptr || *text == '\0')
        throw std::invalid_argument("seconds must be a positive integer");
    char* end = nullptr;
    const unsigned long long value = std::strtoull(text, &end, 10);
    if (end == text || *end != '\0' || value == 0 || value > 600)
        throw std::invalid_argument("seconds must be an integer in [1, 600]");
    return static_cast<std::uint64_t>(value);
}

std::size_t count_events(const spc_runtime_trace& trace) {
    std::size_t total = 0;
    for (const auto& window : trace.windows)
        total += window.records.size();
    return total;
}

std::uint64_t count_dropped_events(const spc_runtime_trace& trace) {
    std::uint64_t total = 0;
    for (const auto& window : trace.windows)
        total += window.dropped;
    return total;
}

std::size_t count_overflowed_windows(const spc_runtime_trace& trace) {
    std::size_t total = 0;
    for (const auto& window : trace.windows)
        total += window.overflowed ? 1u : 0u;
    return total;
}

std::size_t count_ram_origin(
    const spc_runtime_trace& trace,
    spc_runtime_ram_write_origin origin) {
    std::size_t total = 0;
    for (const auto& write : trace.ram_writes)
        total += write.origin == origin ? 1u : 0u;
    return total;
}

void write_double_array(std::ostream& out, const std::vector<double>& values) {
    out << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0)
            out << ',';
        out << values[index];
    }
    out << ']';
}

void write_int8_array(std::ostream& out, const std::vector<std::int8_t>& values) {
    out << '[';
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index != 0)
            out << ',';
        out << static_cast<int>(values[index]);
    }
    out << ']';
}

void write_optional_double_array(
    std::ostream& out,
    const std::optional<std::vector<double>>& values) {
    if (!values.has_value()) {
        out << "null";
        return;
    }
    write_double_array(out, *values);
}

void write_optional_int8_array(
    std::ostream& out,
    const std::optional<std::vector<std::int8_t>>& values) {
    if (!values.has_value()) {
        out << "null";
        return;
    }
    write_int8_array(out, *values);
}

void write_optional_double(std::ostream& out, const std::optional<double>& value) {
    if (value.has_value())
        out << *value;
    else
        out << "null";
}

void write_profile(
    std::ostream& out,
    const part_motif_profile& profile,
    std::size_t index) {
    out << "    {\n";
    out << "      \"profile_index\": " << index << ",\n";
    out << "      \"gesture_count\": " << profile.source_nodes.size() << ",\n";
    out << "      \"normalized_inter_onset_intervals\": ";
    write_double_array(out, profile.normalized_inter_onset_intervals);
    out << ",\n";
    out << "      \"interval_octaves\": ";
    write_optional_double_array(out, profile.interval_octaves);
    out << ",\n";
    out << "      \"pitch_contour\": ";
    write_optional_int8_array(out, profile.pitch_contour);
    out << ",\n";
    out << "      \"interval_semantics\": \"" << profile.interval_semantics << "\",\n";
    out << "      \"pitch_range_octaves\": ";
    write_optional_double(out, profile.pitch_range_octaves);
    out << ",\n";
    out << "      \"evidence_status\": \""
        << evidence_status_name(profile.status) << "\",\n";
    out << "      \"evidence_confidence\": " << profile.evidence_confidence << "\n";
    out << "    }";
}

void write_sidecar(
    const std::string& output_path,
    std::size_t source_size,
    std::uint64_t requested_seconds,
    const spc_runtime_trace& trace,
    const gameaudio::spc::spc_runtime_trace_replay_result& replay,
    const spc_label_blind_corpus_features& features) {
    std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
    if (!out)
        throw std::runtime_error("could not open feature sidecar output");
    out << std::setprecision(17);

    out << "{\n";
    out << "  \"model\": \"label-blind SPC forensic feature sidecar\",\n";
    out << "  \"claim_boundary\": \"Runtime/device facts and derived musical geometry only; ID666, catalog titles, game names, creator labels, and external attribution tags are not feature inputs.\",\n";
    out << "  \"provenance\": {\n";
    out << "    \"retro_vgm_compiler_commit\": \""
        << RETRO_VGM_COMPILER_FORENSIC_COMMIT << "\",\n";
    out << "    \"snes_spc_repository\": \"blarggs-audio-libraries/snes_spc\",\n";
    out << "    \"snes_spc_commit\": \"" << SNES_SPC_FORENSIC_COMMIT << "\",\n";
    out << "    \"instrumentation_patch_contract\": \""
        << SNES_SPC_FORENSIC_PATCH_CONTRACT << "\",\n";
    out << "    \"device_tick_rate\": " << spc_clock_rate << "\n";
    out << "  },\n";
    out << "  \"controlled_execution\": {\n";
    out << "    \"source_bytes\": " << source_size << ",\n";
    out << "    \"requested_seconds\": " << requested_seconds << ",\n";
    out << "    \"requested_device_clocks\": "
        << requested_seconds * spc_clock_rate << "\n";
    out << "  },\n";
    out << "  \"capture\": {\n";
    out << "    \"ram_write_count\": " << trace.ram_writes.size() << ",\n";
    out << "    \"ram_writes_spc700_cpu\": "
        << count_ram_origin(trace, spc_runtime_ram_write_origin::spc700_cpu) << ",\n";
    out << "    \"ram_writes_dsp_echo\": "
        << count_ram_origin(trace, spc_runtime_ram_write_origin::dsp_echo) << ",\n";
    out << "    \"ram_writes_ipl_rom_overlay\": "
        << count_ram_origin(trace, spc_runtime_ram_write_origin::ipl_rom_overlay) << ",\n";
    out << "    \"window_count\": " << trace.windows.size() << ",\n";
    out << "    \"stored_event_count\": " << count_events(trace) << ",\n";
    out << "    \"dropped_event_count\": " << count_dropped_events(trace) << ",\n";
    out << "    \"overflowed_window_count\": " << count_overflowed_windows(trace) << "\n";
    out << "  },\n";
    out << "  \"replay\": {\n";
    out << "    \"windows_replayed\": " << replay.windows_replayed << ",\n";
    out << "    \"ram_writes_applied\": " << replay.ram_writes_applied << ",\n";
    out << "    \"records_materialized\": " << replay.records_materialized << ",\n";
    out << "    \"continuity_breaks\": " << replay.continuity_breaks << ",\n";
    out << "    \"samples_materialized\": " << replay.samples_materialized << ",\n";
    out << "    \"samples_reused\": " << replay.samples_reused << ",\n";
    out << "    \"samples_deferred\": " << replay.samples_deferred << ",\n";
    out << "    \"final_ram_write_serial\": " << replay.final_ram_write_serial << "\n";
    out << "  },\n";
    out << "  \"features\": {\n";
    out << "    \"voice_episode_count\": " << features.voice_episode_count << ",\n";
    out << "    \"eligible_episode_count\": " << features.eligible_episode_count << ",\n";
    out << "    \"candidate_transition_count\": " << features.candidate_transition_count << ",\n";
    out << "    \"strong_transition_count\": " << features.strong_transition_count << ",\n";
    out << "    \"rejected_transition_count\": " << features.rejected_transition_count << ",\n";
    out << "    \"continuity_barrier_count\": " << features.continuity_barrier_count << ",\n";
    out << "    \"emitted_part_count\": " << features.emitted_part_count << ",\n";
    out << "    \"part_profile_count\": " << features.part_profiles.size() << ",\n";
    out << "    \"part_profiles\": [\n";
    for (std::size_t index = 0; index < features.part_profiles.size(); ++index) {
        write_profile(out, features.part_profiles[index], index);
        out << (index + 1 == features.part_profiles.size() ? "\n" : ",\n");
    }
    out << "    ]\n";
    out << "  }\n";
    out << "}\n";

    if (!out)
        throw std::runtime_error("failed while writing feature sidecar");
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 3 || argc > 4) {
            std::cerr
                << "usage: spc_forensic_features <input.spc> <output.json> [seconds]\n";
            return 2;
        }

        const std::uint64_t seconds = argc == 4 ? parse_seconds(argv[3]) : 10u;
        const auto bytes = read_binary(argv[1]);
        if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<long>::max()))
            throw std::runtime_error("SPC input exceeds snes_spc load size range");

        const auto snapshot = gameaudio::spc::parse_spc_snapshot(
            bytes.data(),
            bytes.size());

        SNES_SPC emulator;
        if (const char* error = emulator.init())
            throw std::runtime_error(std::string{"snes_spc init failed: "} + error);

        gameaudio::spc::spc_runtime_trace_recorder recorder;

        // Attach before load: load_spc() starts from the exact SPC RAM image and
        // then deterministically applies saved machine visibility such as IPL ROM
        // overlay. The sink sees only memory/device facts, never ID666 text.
        emulator.set_runtime_instrumentation_sink(&recorder);
        if (const char* error = emulator.load_spc(
                bytes.data(),
                static_cast<long>(bytes.size()))) {
            throw std::runtime_error(std::string{"snes_spc load failed: "} + error);
        }

        std::uint64_t scalar_samples_remaining = seconds * scalar_samples_per_second;
        while (scalar_samples_remaining != 0) {
            const int block = static_cast<int>(std::min<std::uint64_t>(
                scalar_samples_remaining,
                static_cast<std::uint64_t>(playback_chunk_scalar_samples)));
            if ((block & 1) != 0)
                throw std::logic_error("SPC forensic playback block must be stereo-even");
            if (const char* error = emulator.play(block, nullptr)) {
                throw std::runtime_error(std::string{"snes_spc execution failed: "} + error);
            }
            recorder.flush_window();
            scalar_samples_remaining -= static_cast<std::uint64_t>(block);
        }

        const auto trace = recorder.finish();

        vgmtooling::model::musical_execution_graph graph;
        const auto snapshot_graph = gameaudio::spc::materialize_spc_snapshot(
            graph,
            snapshot,
            "spc-fixture");
        auto runtime = gameaudio::spc::begin_spc_runtime_voice_trace(
            graph,
            snapshot_graph,
            "instrumented-snes-spc-runtime",
            vgmtooling::model::to_flags(
                vgmtooling::model::provenance_flag::runtime_capture));
        auto samples = gameaudio::spc::begin_spc_runtime_sample_graph(
            "instrumented-snes-spc-runtime",
            vgmtooling::model::to_flags(
                vgmtooling::model::provenance_flag::runtime_capture));

        const auto replay = gameaudio::spc::replay_spc_runtime_trace(
            graph,
            runtime,
            samples,
            snapshot,
            trace);
        const auto features = gameaudio::spc::extract_spc_label_blind_corpus_features(
            graph,
            "instrumented-snes-spc-runtime");

        write_sidecar(
            argv[2],
            bytes.size(),
            seconds,
            trace,
            replay,
            features);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "spc_forensic_features: " << error.what() << '\n';
        return 1;
    }
}
