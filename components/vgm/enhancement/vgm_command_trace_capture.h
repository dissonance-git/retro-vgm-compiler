#pragma once

#include "vgm_command_event.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::vgm {

// Allocation-free copy of the command metadata needed to reconstruct a source
// execution trace later. Payload bytes are intentionally not copied on the
// realtime thread; file_offset and payload_size preserve the route back to the
// exact source bytes.
struct command_trace_record {
    command_event_kind kind = command_event_kind::command;
    std::uint64_t tick = 0;
    std::uint32_t file_offset = 0;
    std::uint8_t command = 0;
    std::uint32_t payload_size = 0;
};

constexpr command_trace_record make_command_trace_record(const command_event& event) noexcept {
    return {
        event.kind,
        event.tick,
        event.file_offset,
        event.command,
        event.payload_size,
    };
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
