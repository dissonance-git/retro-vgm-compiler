#pragma once

#include "vgm_command_event.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Allocation-free copy of the command metadata needed to reconstruct a source
// execution trace later. The first two payload bytes are retained because many
// register-oriented commands are completely described by one or two bytes.
// Larger payloads remain explicitly partial and are recoverable through the
// file offset/source route rather than being silently truncated as if complete.
struct command_trace_record {
    static constexpr std::size_t payload_prefix_capacity = 2;

    command_event_kind kind = command_event_kind::command;
    std::uint64_t tick = 0;
    std::uint32_t file_offset = 0;
    std::uint8_t command = 0;
    std::uint32_t payload_size = 0;
    std::array<std::uint8_t, payload_prefix_capacity> payload_prefix{};
    std::uint8_t payload_prefix_size = 0;
    bool payload_truncated = false;
};

inline command_trace_record make_command_trace_record(const command_event& event) noexcept {
    command_trace_record record;
    record.kind = event.kind;
    record.tick = event.tick;
    record.file_offset = event.file_offset;
    record.command = event.command;
    record.payload_size = event.payload_size;

    if (event.payload != nullptr) {
        const std::size_t available = static_cast<std::size_t>(event.payload_size);
        const std::size_t copied = available < command_trace_record::payload_prefix_capacity
            ? available
            : command_trace_record::payload_prefix_capacity;
        for (std::size_t i = 0; i < copied; ++i)
            record.payload_prefix[i] = event.payload[i];
        record.payload_prefix_size = static_cast<std::uint8_t>(copied);
        record.payload_truncated = available > copied;
    } else {
        record.payload_truncated = event.payload_size != 0;
    }

    return record;
}

constexpr bool has_complete_payload(const command_trace_record& record) noexcept {
    return !record.payload_truncated &&
        record.payload_prefix_size == record.payload_size;
}

// Fixed-capacity capture window suitable for genesis_state::event_tap and other
// realtime command observers. Drain/materialize it off the audio thread, then
// begin a new window. Overflow is explicit and must become incomplete
// provenance in any graph materialized from the window.
class command_trace_capture {
public:
    static constexpr std::size_t capacity = 16384;

    void begin_window() noexcept {
        count_ = 0;
        overflow_ = false;
        dropped_ = 0;
    }

    void observe(const command_event& event) noexcept {
        if (count_ >= capacity) {
            overflow_ = true;
            ++dropped_;
            return;
        }

        records_[count_] = make_command_trace_record(event);
        ++count_;
    }

    static void tap(void* user, const command_event& event) noexcept {
        if (user != nullptr)
            static_cast<command_trace_capture*>(user)->observe(event);
    }

    const command_trace_record* records() const noexcept { return records_.data(); }
    std::size_t count() const noexcept { return count_; }
    bool overflowed() const noexcept { return overflow_; }
    std::uint64_t dropped() const noexcept { return dropped_; }

private:
    std::array<command_trace_record, capacity> records_{};
    std::size_t count_ = 0;
    bool overflow_ = false;
    std::uint64_t dropped_ = 0;
};

} // namespace gameaudio::vgm
