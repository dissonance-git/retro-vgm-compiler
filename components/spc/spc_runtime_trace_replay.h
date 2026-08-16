#pragma once

#include "spc_runtime_capture_adapter.h"
#include "spc_snapshot.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gameaudio::spc {

// Offline trace contract for deterministic SPC corpus analysis. DSP observations
// alone are insufficient: event-time BRR identity also depends on every APURAM
// mutation after the SPC snapshot. Preserve source-write boundaries and serials
// so the RAM generation tracker can reconstruct exactly what each event saw.
struct spc_runtime_trace_ram_write {
    std::uint64_t serial = 0;
    std::uint16_t address = 0;
    std::vector<std::uint8_t> bytes;
};

struct spc_runtime_trace_window {
    std::vector<spc_runtime_capture_record> records;
    bool overflowed = false;
    std::uint64_t dropped = 0;
    std::optional<spc_runtime_capture_record> first_dropped{};
    std::uint64_t next_trace_index = 0;
};

struct spc_runtime_trace {
    std::array<std::uint8_t, spc_runtime_ram_size> initial_ram{};
    std::vector<spc_runtime_trace_ram_write> ram_writes;
    std::vector<spc_runtime_trace_window> windows;
};

inline spc_runtime_trace begin_spc_runtime_trace_from_snapshot(
    const spc_snapshot& snapshot) {
    spc_runtime_trace trace;
    trace.initial_ram = snapshot.ram;
    return trace;
}

struct spc_runtime_trace_replay_result {
    std::size_t windows_replayed = 0;
    std::size_t ram_writes_applied = 0;
    std::size_t records_materialized = 0;
    std::size_t continuity_breaks = 0;
    std::size_t samples_materialized = 0;
    std::size_t samples_reused = 0;
    std::size_t samples_deferred = 0;
    std::uint64_t final_ram_write_serial = 0;
};

namespace detail {

inline void apply_spc_trace_ram_write(
    std::array<std::uint8_t, spc_runtime_ram_size>& live_ram,
    spc_ram_generation_tracker& tracker,
    const spc_runtime_trace_ram_write& write) {
    if (write.bytes.empty())
        throw std::invalid_argument("SPC runtime trace RAM write cannot be empty");
    if (write.bytes.size() > spc_runtime_ram_size)
        throw std::invalid_argument("SPC runtime trace RAM write cannot exceed one APURAM image");
    if (write.serial != tracker.write_serial() + 1u)
        throw std::invalid_argument("SPC runtime trace RAM write serial is not contiguous");

    for (std::size_t index = 0; index < write.bytes.size(); ++index) {
        const auto address = static_cast<std::uint16_t>(
            static_cast<std::uint32_t>(write.address) +
            static_cast<std::uint32_t>(index));
        live_ram[address] = write.bytes[index];
    }
    tracker.mark_write(write.address, write.bytes.size());
    if (tracker.write_serial() != write.serial)
        throw std::logic_error("SPC RAM generation tracker diverged from trace serial");
}

inline void accumulate_spc_trace_materialization(
    const spc_runtime_capture_materialization_result& materialized,
    spc_runtime_trace_replay_result& result) noexcept {
    result.records_materialized += materialized.records_materialized;
    result.continuity_breaks += materialized.continuity_breaks;
    result.samples_materialized += materialized.samples_materialized;
    result.samples_reused += materialized.samples_reused;
    result.samples_deferred += materialized.samples_deferred;
}

} // namespace detail

inline spc_runtime_trace_replay_result replay_spc_runtime_trace(
    vgmtooling::model::musical_execution_graph& graph,
    spc_runtime_voice_graph_handle& runtime,
    spc_runtime_sample_graph_handle& samples,
    const spc_runtime_trace& trace) {
    std::array<std::uint8_t, spc_runtime_ram_size> live_ram = trace.initial_ram;
    spc_ram_generation_tracker tracker;
    spc_ram_shadow shadow;
    shadow.synchronize(live_ram.data(), tracker);

    spc_runtime_capture_materializer_state materializer_state;
    spc_runtime_trace_replay_result result;
    std::size_t next_write = 0;

    const auto synchronize_to_serial = [&](std::uint64_t target_serial) {
        if (target_serial < tracker.write_serial())
            throw std::invalid_argument("SPC runtime trace event moves backward in RAM-write time");

        while (tracker.write_serial() < target_serial) {
            if (next_write >= trace.ram_writes.size())
                throw std::invalid_argument("SPC runtime trace is missing APURAM writes required by an event");
            const auto& write = trace.ram_writes[next_write];
            if (write.serial > target_serial)
                throw std::invalid_argument("SPC runtime trace has a RAM-write serial gap before an event");
            detail::apply_spc_trace_ram_write(live_ram, tracker, write);
            ++next_write;
            ++result.ram_writes_applied;
        }

        shadow.synchronize(live_ram.data(), tracker);
    };

    for (const auto& window : trace.windows) {
        ++result.windows_replayed;
        if (window.overflowed && !window.first_dropped.has_value())
            throw std::invalid_argument("overflowed SPC runtime trace window requires first-dropped boundary");
        if (!window.overflowed && (window.dropped != 0 || window.first_dropped.has_value()))
            throw std::invalid_argument("non-overflowed SPC runtime trace window cannot report dropped records");

        for (const auto& record : window.records) {
            synchronize_to_serial(record.ram_write_serial);
            const spc_runtime_capture_window_view single{
                &record,
                1,
                false,
                0,
                nullptr,
                record.trace_index + 1u,
            };
            const auto materialized = materialize_spc_runtime_capture_window(
                graph,
                runtime,
                materializer_state,
                single,
                {&samples, &shadow});
            detail::accumulate_spc_trace_materialization(materialized, result);
        }

        if (window.overflowed) {
            const auto& dropped = *window.first_dropped;
            synchronize_to_serial(dropped.ram_write_serial);
            const spc_runtime_capture_window_view gap{
                nullptr,
                0,
                true,
                window.dropped,
                &dropped,
                window.next_trace_index,
            };
            const auto materialized = materialize_spc_runtime_capture_window(
                graph,
                runtime,
                materializer_state,
                gap,
                {&samples, &shadow});
            detail::accumulate_spc_trace_materialization(materialized, result);
        } else if (window.records.empty()) {
            const spc_runtime_capture_window_view empty{
                nullptr,
                0,
                false,
                0,
                nullptr,
                window.next_trace_index,
            };
            const auto materialized = materialize_spc_runtime_capture_window(
                graph,
                runtime,
                materializer_state,
                empty,
                {&samples, &shadow});
            detail::accumulate_spc_trace_materialization(materialized, result);
        }
    }

    result.final_ram_write_serial = tracker.write_serial();
    return result;
}

} // namespace gameaudio::spc
