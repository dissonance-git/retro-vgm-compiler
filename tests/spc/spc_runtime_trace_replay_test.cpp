#include "components/spc/spc_runtime_trace_replay.h"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace gameaudio::spc;
using namespace vgmtooling::model;

namespace {

spc_runtime_capture_record key_on(
    std::uint64_t trace_index,
    std::int64_t tick,
    std::uint64_t ram_write_serial,
    std::uint32_t pitch_rate) {
    spc_runtime_capture_record record;
    record.kind = spc_voice_runtime_event_kind::key_on_accepted;
    record.fields =
        spc_runtime_capture_field::voice |
        spc_runtime_capture_field::source_index |
        spc_runtime_capture_field::pitch_rate;
    record.trace_index = trace_index;
    record.tick = tick;
    record.tick_rate = 32000;
    record.ram_write_serial = ram_write_serial;
    record.voice = 0;
    record.source_index = 7;
    record.pitch_rate = pitch_rate;
    return record;
}

spc_runtime_capture_record sample_phase(
    std::uint64_t trace_index,
    std::int64_t tick,
    std::uint64_t ram_write_serial) {
    spc_runtime_capture_record record;
    record.kind = spc_voice_runtime_event_kind::sample_phase_started;
    record.fields =
        spc_runtime_capture_field::voice |
        spc_runtime_capture_field::source_index |
        spc_runtime_capture_field::brr_address;
    record.trace_index = trace_index;
    record.tick = tick;
    record.tick_rate = 32000;
    record.ram_write_serial = ram_write_serial;
    record.voice = 0;
    record.source_index = 7;
    record.brr_address = 0x2000;
    return record;
}

spc_runtime_trace make_trace() {
    spc_runtime_trace trace;
    trace.ram_writes.push_back({1, 0x2001, {0x55}});

    spc_runtime_trace_window window;
    window.records.push_back(key_on(0, 0, 0, 0x1000));
    window.records.push_back(sample_phase(1, 1, 0));
    window.records.push_back(key_on(2, 4000, 1, 0x1100));
    window.records.push_back(sample_phase(3, 4001, 1));
    window.next_trace_index = 4;
    trace.windows.push_back(std::move(window));
    return trace;
}

spc_snapshot make_snapshot() {
    spc_snapshot snapshot;
    snapshot.source_size = spc_min_file_size;
    // One complete BRR block at 0x2000. The second byte will be rewritten later.
    snapshot.ram[0x2000] = 0x01;
    return snapshot;
}

} // namespace

int main() {
    {
        auto snapshot = make_snapshot();
        musical_execution_graph graph;
        const auto snapshot_graph = materialize_spc_snapshot(
            graph,
            snapshot,
            "synthetic-spc");
        auto runtime = begin_spc_runtime_voice_trace(
            graph,
            snapshot_graph,
            "synthetic-runtime",
            to_flags(provenance_flag::runtime_capture));
        auto samples = begin_spc_runtime_sample_graph(
            "synthetic-runtime",
            to_flags(provenance_flag::runtime_capture));

        const auto trace = make_trace();
        const auto replayed = replay_spc_runtime_trace(
            graph,
            runtime,
            samples,
            snapshot,
            trace);

        assert(replayed.windows_replayed == 1);
        assert(replayed.ram_writes_applied == 1);
        assert(replayed.final_ram_write_serial == 1);
        assert(replayed.records_materialized == 4);
        assert(replayed.continuity_breaks == 0);
        assert(replayed.samples_materialized == 2);
        assert(replayed.samples_reused == 0);
        assert(replayed.samples_deferred == 0);
        assert(samples.cache.size() == 2);
        assert(samples.cache[0].sample_id != samples.cache[1].sample_id);
        assert(samples.cache[0].scan.start_address == 0x2000);
        assert(samples.cache[1].scan.start_address == 0x2000);
        assert(samples.cache[0].scan.compressed_bytes[1] == 0x00);
        assert(samples.cache[1].scan.compressed_bytes[1] == 0x55);
    }

    {
        // An event claiming unseen RAM-write time must fail closed instead of
        // materializing a sample from whatever bytes happen to be available.
        auto snapshot = make_snapshot();
        musical_execution_graph graph;
        const auto snapshot_graph = materialize_spc_snapshot(
            graph,
            snapshot,
            "synthetic-spc");
        auto runtime = begin_spc_runtime_voice_trace(
            graph,
            snapshot_graph,
            "synthetic-runtime",
            to_flags(provenance_flag::runtime_capture));
        auto samples = begin_spc_runtime_sample_graph(
            "synthetic-runtime",
            to_flags(provenance_flag::runtime_capture));

        auto trace = make_trace();
        trace.windows.front().records.back().ram_write_serial = 2;
        bool threw = false;
        try {
            (void)replay_spc_runtime_trace(graph, runtime, samples, snapshot, trace);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    {
        // Corrupt capture ordering is not converted into a synthetic timeline.
        auto snapshot = make_snapshot();
        musical_execution_graph graph;
        const auto snapshot_graph = materialize_spc_snapshot(
            graph,
            snapshot,
            "synthetic-spc");
        auto runtime = begin_spc_runtime_voice_trace(
            graph,
            snapshot_graph,
            "synthetic-runtime",
            to_flags(provenance_flag::runtime_capture));
        auto samples = begin_spc_runtime_sample_graph(
            "synthetic-runtime",
            to_flags(provenance_flag::runtime_capture));

        auto trace = make_trace();
        trace.windows.front().records[2].trace_index = 1;
        bool threw = false;
        try {
            (void)replay_spc_runtime_trace(graph, runtime, samples, snapshot, trace);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    return 0;
}
