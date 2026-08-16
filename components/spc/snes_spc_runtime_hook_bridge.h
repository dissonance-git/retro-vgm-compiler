#pragma once

#include "spc_runtime_instrumentation_sink.h"

#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

inline void observe_snes_spc_apuram_mutation(
    spc_runtime_instrumentation_sink* sink,
    spc_runtime_ram_write_origin origin,
    std::int64_t tick,
    std::uint64_t tick_rate,
    std::uint16_t address,
    const std::uint8_t* bytes,
    std::size_t byte_count) {
    if (sink == nullptr)
        return;
    sink->observe_apuram_write(origin, tick, tick_rate, address, bytes, byte_count);
}

inline void observe_snes_spc_key_on_accepted(
    spc_runtime_instrumentation_sink* sink,
    std::int64_t tick,
    std::uint64_t tick_rate,
    std::uint8_t voice,
    std::uint32_t pitch_rate,
    std::int8_t route_gain_left,
    std::int8_t route_gain_right,
    bool noise_enabled,
    bool echo_send_enabled) {
    if (sink == nullptr)
        return;

    spc_runtime_capture_record record;
    record.kind = spc_voice_runtime_event_kind::key_on_accepted;
    record.fields =
        spc_runtime_capture_field::voice |
        spc_runtime_capture_field::pitch_rate |
        spc_runtime_capture_field::key_on_delay |
        spc_runtime_capture_field::noise_enabled |
        spc_runtime_capture_field::route_gain_left |
        spc_runtime_capture_field::route_gain_right |
        spc_runtime_capture_field::echo_send_enabled;
    record.tick = tick;
    record.tick_rate = tick_rate;
    record.voice = voice;
    record.pitch_rate = pitch_rate;
    record.key_on_delay = 5;
    record.noise_enabled = noise_enabled;
    record.route_gain_left = route_gain_left;
    record.route_gain_right = route_gain_right;
    record.echo_send_enabled = echo_send_enabled;
    sink->observe_voice_event(record);
}

inline void observe_snes_spc_sample_phase_started(
    spc_runtime_instrumentation_sink* sink,
    std::int64_t tick,
    std::uint64_t tick_rate,
    std::uint8_t voice,
    std::uint8_t source_index,
    std::uint16_t brr_address,
    std::uint16_t directory_loop_address,
    std::uint32_t pitch_rate,
    std::int8_t route_gain_left,
    std::int8_t route_gain_right,
    bool noise_enabled,
    bool echo_send_enabled) {
    if (sink == nullptr)
        return;

    spc_runtime_capture_record record;
    record.kind = spc_voice_runtime_event_kind::sample_phase_started;
    record.fields =
        spc_runtime_capture_field::voice |
        spc_runtime_capture_field::source_index |
        spc_runtime_capture_field::brr_address |
        spc_runtime_capture_field::directory_loop_address |
        spc_runtime_capture_field::pitch_rate |
        spc_runtime_capture_field::key_on_delay |
        spc_runtime_capture_field::noise_enabled |
        spc_runtime_capture_field::route_gain_left |
        spc_runtime_capture_field::route_gain_right |
        spc_runtime_capture_field::echo_send_enabled;
    record.tick = tick;
    record.tick_rate = tick_rate;
    record.voice = voice;
    record.source_index = source_index;
    record.brr_address = brr_address;
    record.directory_loop_address = directory_loop_address;
    record.pitch_rate = pitch_rate;
    record.key_on_delay = 5;
    record.noise_enabled = noise_enabled;
    record.route_gain_left = route_gain_left;
    record.route_gain_right = route_gain_right;
    record.echo_send_enabled = echo_send_enabled;
    sink->observe_voice_event(record);
}

inline void observe_snes_spc_release_entered(
    spc_runtime_instrumentation_sink* sink,
    std::int64_t tick,
    std::uint64_t tick_rate,
    std::uint8_t voice,
    std::uint32_t envelope_value) {
    if (sink == nullptr)
        return;

    spc_runtime_capture_record record;
    record.kind = spc_voice_runtime_event_kind::release_entered;
    record.fields =
        spc_runtime_capture_field::voice |
        spc_runtime_capture_field::envelope_value;
    record.tick = tick;
    record.tick_rate = tick_rate;
    record.voice = voice;
    record.envelope_value = envelope_value;
    sink->observe_voice_event(record);
}

inline void observe_snes_spc_became_inactive(
    spc_runtime_instrumentation_sink* sink,
    std::int64_t tick,
    std::uint64_t tick_rate,
    std::uint8_t voice) {
    if (sink == nullptr)
        return;

    spc_runtime_capture_record record;
    record.kind = spc_voice_runtime_event_kind::became_inactive;
    record.fields =
        spc_runtime_capture_field::voice |
        spc_runtime_capture_field::envelope_value;
    record.tick = tick;
    record.tick_rate = tick_rate;
    record.voice = voice;
    record.envelope_value = 0;
    sink->observe_voice_event(record);
}

inline void observe_snes_spc_execution_reset(
    spc_runtime_instrumentation_sink* sink,
    std::int64_t tick,
    std::uint64_t tick_rate) {
    if (sink == nullptr)
        return;

    spc_runtime_capture_record record;
    record.kind = spc_voice_runtime_event_kind::execution_reset;
    record.tick = tick;
    record.tick_rate = tick_rate;
    sink->observe_voice_event(record);
}

} // namespace gameaudio::spc
