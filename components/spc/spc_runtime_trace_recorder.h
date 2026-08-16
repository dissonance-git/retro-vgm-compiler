#pragma once

#include "spc_ram_generation.h"
#include "spc_runtime_instrumentation_sink.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace gameaudio::spc {

class spc_runtime_trace_recorder final : public spc_runtime_instrumentation_sink {
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

    std::uint64_t observe_apuram_write(
        spc_runtime_ram_write_origin origin,
        std::int64_t tick,
        std::uint64_t tick_rate,
        std::uint16_t address,
        const std::uint8_t* bytes,
        std::size_t byte_count) override {
        if (bytes == nullptr)
            throw std::invalid_argument("SPC runtime RAM observation requires bytes");
        if (byte_count == 0 || byte_count > spc_runtime_ram_size)
            throw std::invalid_argument("SPC runtime RAM observation size must be in [1, 65536]");
        if (tick_rate == 0)
            throw std::invalid_argument("SPC runtime RAM observation requires a non-zero tick rate");

        spc_runtime_trace_ram_write write;
        write.serial = ram_generation_.write_serial() + 1u;
        write.tick = tick;
        write.tick_rate = tick_rate;
        write.origin = origin;
        write.address = address;
        write.bytes.assign(bytes, bytes + byte_count);

        trace_.ram_writes.push_back(std::move(write));
        ram_generation_.mark_write(address, byte_count);
        return ram_generation_.write_serial();
    }

    std::uint64_t observe_apuram_write_byte(
        spc_runtime_ram_write_origin origin,
        std::int64_t tick,
        std::uint64_t tick_rate,
        std::uint16_t address,
        std::uint8_t value) {
        return observe_apuram_write(origin, tick, tick_rate, address, &value, 1);
    }

    std::uint64_t observe_apuram_write_le16(
        spc_runtime_ram_write_origin origin,
        std::int64_t tick,
        std::uint64_t tick_rate,
        std::uint16_t address,
        std::uint16_t value) {
        const std::uint8_t bytes[2] = {
            static_cast<std::uint8_t>(value & 0xFFu),
            static_cast<std::uint8_t>((value >> 8u) & 0xFFu),
        };
        return observe_apuram_write(origin, tick, tick_rate, address, bytes, 2);
    }

    void observe_voice_event(spc_runtime_capture_record record) override {
        if (record.tick_rate == 0)
            throw std::invalid_argument("SPC runtime voice observation requires a non-zero tick rate");
        record.ram_write_serial = ram_generation_.write_serial();
        capture_.observe(record);
    }

    bool flush_window() {
        if (capture_.count() == 0 && !capture_.overflowed()) {
            capture_.begin_window();
            return false;
        }

        spc_runtime_trace_window window;
        window.records.assign(capture_.records(), capture_.records() + capture_.count());
        window.overflowed = capture_.overflowed();
        window.dropped = capture_.dropped();
        if (const auto* first = capture_.first_dropped_record())
            window.first_dropped = *first;
        window.next_trace_index = capture_.next_trace_index();
        trace_.windows.push_back(std::move(window));
        capture_.begin_window();
        return true;
    }

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
