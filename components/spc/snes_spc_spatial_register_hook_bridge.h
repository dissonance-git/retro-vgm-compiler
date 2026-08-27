#pragma once

#include "spc_runtime_instrumentation_sink.h"

#include <cstdint>

namespace gameaudio::spc {

constexpr std::int8_t snes_spc_signed_register_byte(std::uint8_t value) noexcept
{
    return value < 0x80u
        ? static_cast<std::int8_t>(value)
        : static_cast<std::int8_t>(static_cast<int>(value) - 0x100);
}

// Exact S-DSP spatial control state after a real DSP register write has taken
// effect. The patched dependency passes the complete current VOLL/VOLR/EON state
// for the affected physical voice, so downstream code never has to infer the
// complementary channel or effect-send bit from a partial write.
inline void observe_snes_spc_spatial_route_state(
    spc_runtime_instrumentation_sink* sink,
    std::int64_t tick,
    std::uint64_t tick_rate,
    std::uint8_t voice,
    std::uint8_t route_gain_left,
    std::uint8_t route_gain_right,
    bool echo_send_enabled)
{
    if (sink == nullptr || voice >= 8)
        return;

    spc_runtime_capture_record record{};
    record.kind = spc_voice_runtime_event_kind::routing_state_changed;
    record.fields = spc_runtime_capture_field::voice
        | spc_runtime_capture_field::route_gain_left
        | spc_runtime_capture_field::route_gain_right
        | spc_runtime_capture_field::echo_send_enabled;
    record.tick = tick;
    record.tick_rate = tick_rate;
    record.voice = voice;
    record.route_gain_left = snes_spc_signed_register_byte(route_gain_left);
    record.route_gain_right = snes_spc_signed_register_byte(route_gain_right);
    record.echo_send_enabled = echo_send_enabled;
    sink->observe_voice_event(record);
}

} // namespace gameaudio::spc
