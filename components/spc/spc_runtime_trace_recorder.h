#pragma once

#include "spc_ram_generation.h"
#include "spc_runtime_trace.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gameaudio::spc {

// Helix-owned sink for instrumented SPC execution. This recorder is intended for
// offline forensic/corpus runs, so RAM-write byte copies may allocate. The vendor
// core should remain a thin sensor and pass exact post-mutation APURAM bytes plus
// exact device events into this object. No catalog or attribution metadata enters
// this layer.
class spc_runtime_trace_recorder {
public:
    void reset() {
        capture_.reset_trace();
        ram_generation_ = spc_ram_generation_tracker{};
        trace_ = spc_runtime_trace{};
    }

    std::uint64_t ram_write_serial() const noexcept {
        return ram_generation_.write_serial();
    }

    std::uint64_t next_trace_index() const noexcept {
        return capture_.next_trace_index();
    }

    const spc_runtime_trace& trace() const noexcept {
        return trace_;
    }

    // Record one source mutation boundary. `bytes` must contain the exact APURAM
    // values visible *after* that mutation. Address arithmetic wraps at 16 bits,
    // matching SPC700 APURAM. A bulk overlay/copy should be one call so it receives
    // one generation serial; two independent hardware writes should be two calls.
    std::uint64_t observe_apuram_write(
        std::uint16_t address,
        const std::uint8_t* bytes,
        std::size_t byte_count) {
        if (bytes == nullptr)
            throw std::invalid_argument("SPC runtime RAM observation requires bytes");
        if (byte_count == 0 || byte_count > spc_runtime_ram_size)
            throw std::invalid_argument("SPC runtime RAM observation size must be in [1, 65536]");

        spc_runtime_trace_ram_write write;
        write.serial = ram_generation_.write_serial() + 1u;
        write.address = address;
        write.bytes.assign(bytes, bytes + byte_count);

        // Publish the owned bytes before advancing the generation clock. If the
        // allocation above fails, the recorder remains at the previous serial.
        trace_.ram_writes.push_back(std::move(write));
        ram_generation_.mark_write(address, byte_count);
        return ram_generation_.write_serial();
    }

    std::uint64_t observe_apuram_write_byte(
        std::uint16_t address,
        std::uint8_t value) {
        return observe_apuram_write(address, &value, 1);
    }

    std::uint64_t observe_apuram_write_le16(
        std::uint16_t address,
        std::uint16_t value) {
        const std::uint8_t bytes[2] = {
            static_cast<std::uint8_t>(value & 0xFFu),
            static_cast<std::uint8_t>((value >> 8u) & 0xFFu),
        };
        return observe_apuram_write(address, bytes, 2);
    }

    // The caller supplies device facts only. RAM generation is stamped here so a
    // vendor hook cannot accidentally report an event against an arbitrary memory
    // version. `spc_runtime_capture` owns the monotonically increasing trace index.
    void observe_voice_event(spc_runtime_capture_record record) {
        if (record.tick_rate == 0)
            throw std::invalid_argument("SPC runtime voice observation requires a non-zero tick rate");
        record.ram_write_serial = ram_generation_.write_serial();
        capture_.observe(record);
    }

    // Drain the fixed-capacity DSP window into durable offline trace storage.
    // Overflow details are copied verbatim and later become a hard continuity
    // barrier. Empty, non-overflowed windows carry no information and are skipped.
    bool flush_window() {
        if (capture_.count() == 0 && !capture_.overflowed()) {
            capture_.begin_window();
            return false;
        }

        spc_runtime_trace_window window;
        window.records.assign(
            capture_.records(),
            capture_.records() + capture_.count());
        window.overflowed = capture_.overflowed();
        window.dropped = capture_.dropped();
        if (const auto* first = capture_.first_dropped_record())
            window.first_dropped = *first;
        window.next_trace_index = capture_.next_trace_index();
        trace_.windows.push_back(std::move(window));
        capture_.begin_window();
        return true;
    }

    // Flush the final non-empty capture window and transfer ownership. The
    // recorder is reset to a clean execution so accidental reuse cannot append to
    // the just-finished trace.
    spc_runtime_trace finish() {
        flush_window();
        spc_runtime_trace completed = std::move(trace_);
        reset();
        return completed;
    }

private:
    spc_runtime_capture capture_{};
    spc_ram_generation_tracker ram_generation_{};
    spc_runtime_trace trace_{};
};

} // namespace gameaudio::spc
