#include "components/spc/snes_spc_spatial_register_hook_bridge.h"
#include "components/spc/spc_runtime_trace_recorder.h"

#include <cassert>
#include <cstdint>

using namespace gameaudio::spc;

int main() {
    constexpr std::uint64_t tick_rate = 1024000;

    spc_runtime_trace_recorder recorder;
    observe_snes_spc_spatial_route_state(
        &recorder,
        123,
        tick_rate,
        3,
        0xD0,
        0x40,
        true);

    recorder.flush_window();
    const auto& trace = recorder.trace();
    assert(trace.windows.size() == 1);
    assert(trace.windows[0].records.size() == 1);

    const auto& record = trace.windows[0].records[0];
    assert(record.kind == spc_voice_runtime_event_kind::routing_state_changed);
    assert(record.tick == 123);
    assert(record.tick_rate == tick_rate);
    assert(record.voice == 3);
    assert(record.route_gain_left == -48);
    assert(record.route_gain_right == 64);
    assert(record.echo_send_enabled);
    assert(has_field(record.fields, spc_runtime_capture_field::voice));
    assert(has_field(record.fields, spc_runtime_capture_field::route_gain_left));
    assert(has_field(record.fields, spc_runtime_capture_field::route_gain_right));
    assert(has_field(record.fields, spc_runtime_capture_field::echo_send_enabled));

    // Route-state callbacks are sourced from the SPC700-side DSP register write
    // boundary. They must not masquerade as a DSP-side SRCN/source-latch event,
    // because producer-clock lane identity is part of the runtime trace contract.
    assert(spc_runtime_clock_lane_for_event(record.kind) ==
           spc_runtime_clock_lane::spc700);

    return 0;
}
