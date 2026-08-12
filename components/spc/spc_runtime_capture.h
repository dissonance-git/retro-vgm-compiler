#pragma once

#include "spc_runtime_voice_adapter.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

enum class spc_runtime_capture_field : std::uint16_t {
    none = 0,
    voice = 1u << 0u,
    source_index = 1u << 1u,
    brr_address = 1u << 2u,
    directory_loop_address = 1u << 3u,
    envelope_value = 1u << 4u,
    pitch_rate = 1u << 5u,
    key_on_delay = 1u << 6u,
    noise_enabled = 1u << 7u,
};

using spc_runtime_capture_fields = std::uint16_t;

constexpr spc_runtime_capture_fields to_fields(spc_runtime_capture_field field) noexcept {
    return static_cast<spc_runtime_capture_fields>(field);
}

constexpr spc_runtime_capture_fields operator|(
    spc_runtime_capture_field lhs,
    spc_runtime_capture_field rhs) noexcept {
    return to_fields(lhs) | to_fields(rhs);
}

constexpr spc_runtime_capture_fields operator|(
    spc_runtime_capture_fields lhs,
    spc_runtime_capture_field rhs) noexcept {
    return lhs | to_fields(rhs);
}

constexpr bool has_field(
    spc_runtime_capture_fields fields,
    spc_runtime_capture_field field) noexcept {
    return (fields & to_fields(field)) != 0;
}

// Plain fixed-size record copied at the instrumented DSP boundary. It contains
// only device/runtime facts available at the observation point. Interpretation,
// graph construction and RAM scanning happen after the realtime window drains.
struct spc_runtime_capture_record {
    spc_voice_runtime_event_kind kind = spc_voice_runtime_event_kind::source_latched;
    spc_runtime_capture_fields fields = to_fields(spc_runtime_capture_field::none);
    std::uint64_t trace_index = 0;
    std::int64_t tick = 0;
    std::uint64_t tick_rate = 0;
    std::uint64_t ram_write_serial = 0;
    std::uint8_t voice = 0;
    std::uint8_t source_index = 0;
    std::uint16_t brr_address = 0;
    std::uint16_t directory_loop_address = 0;
    std::uint32_t envelope_value = 0;
    std::uint32_t pitch_rate = 0;
    std::uint8_t key_on_delay = 0;
    bool noise_enabled = false;
};

inline spc_voice_runtime_event make_spc_voice_runtime_event(
    const spc_runtime_capture_record& record) {
    spc_voice_runtime_event event;
    event.kind = record.kind;
    event.tick = record.tick;
    event.tick_rate = record.tick_rate;

    if (has_field(record.fields, spc_runtime_capture_field::voice))
        event.voice = record.voice;
    if (has_field(record.fields, spc_runtime_capture_field::source_index))
        event.source_index = record.source_index;
    if (has_field(record.fields, spc_runtime_capture_field::brr_address))
        event.brr_address = record.brr_address;
    if (has_field(record.fields, spc_runtime_capture_field::envelope_value))
        event.envelope_value = record.envelope_value;
    if (has_field(record.fields, spc_runtime_capture_field::pitch_rate))
        event.pitch_rate = record.pitch_rate;
    if (has_field(record.fields, spc_runtime_capture_field::key_on_delay))
        event.key_on_delay = record.key_on_delay;
    if (has_field(record.fields, spc_runtime_capture_field::noise_enabled))
        event.noise_enabled = record.noise_enabled;

    return event;
}

// Fixed-capacity capture suitable for the realtime S-DSP observation path.
// begin_window() drains storage without resetting trace identity. reset_trace()
// is reserved for a new controlled execution. A record's trace_index advances
// even when the record is dropped, making overflow a visible ordinal gap rather
// than allowing later records to masquerade as contiguous observations.
class spc_runtime_capture {
public:
    static constexpr std::size_t capacity = 16384;

    void begin_window() noexcept {
        count_ = 0;
        overflow_ = false;
        dropped_ = 0;
    }

    void reset_trace() noexcept {
        begin_window();
        next_trace_index_ = 0;
    }

    void observe(spc_runtime_capture_record record) noexcept {
        record.trace_index = next_trace_index_++;

        if (count_ >= capacity) {
            overflow_ = true;
            ++dropped_;
            return;
        }

        records_[count_++] = record;
    }

    const spc_runtime_capture_record* records() const noexcept {
        return records_.data();
    }

    std::size_t count() const noexcept { return count_; }
    bool overflowed() const noexcept { return overflow_; }
    std::uint64_t dropped() const noexcept { return dropped_; }
    std::uint64_t next_trace_index() const noexcept { return next_trace_index_; }

private:
    std::array<spc_runtime_capture_record, capacity> records_{};
    std::size_t count_ = 0;
    bool overflow_ = false;
    std::uint64_t dropped_ = 0;
    std::uint64_t next_trace_index_ = 0;
};

} // namespace gameaudio::spc
