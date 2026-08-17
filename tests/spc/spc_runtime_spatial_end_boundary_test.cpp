#include "components/spc/spc_runtime_spatial_adapter.h"

using namespace gameaudio::spc;
using namespace vgmtooling::model;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    spc_runtime_spatial_adapter<4, 8> adapter;
    adapter.reset();

    spc_runtime_capture_record route;
    route.kind = spc_voice_runtime_event_kind::routing_state_changed;
    route.trace_index = 0;
    route.tick = 4;
    route.tick_rate = 1000;
    route.voice = 1;
    route.fields = to_fields(spc_runtime_capture_field::voice) |
        spc_runtime_capture_field::route_gain_left |
        spc_runtime_capture_field::route_gain_right |
        spc_runtime_capture_field::echo_send_enabled;
    route.route_gain_left = 32;
    route.route_gain_right = -64;
    route.echo_send_enabled = true;

    // The observation is exactly one-past-the-last frame of this host window.
    CHECK(adapter.build_window({&route, 1, false}, 0, 1000, 1000, 4));
    CHECK(adapter.segment_count() == 1);
    const auto& prior = adapter.segments()[0].sources.lanes[1].evidence;
    CHECK(!prior.stereo_route.present);
    CHECK(!prior.effect_send_known);

    // But the persistent device state must carry forward into the next window.
    CHECK(adapter.state().voices[1].route_left_known);
    CHECK(adapter.state().voices[1].route_right_known);
    CHECK(adapter.state().voices[1].echo_send_known);

    CHECK(adapter.build_window({nullptr, 0, false}, 4, 1000, 1000, 4));
    CHECK(adapter.segment_count() == 1);
    const auto& next = adapter.segments()[0].sources.lanes[1].evidence;
    CHECK(next.stereo_route.present);
    CHECK(next.stereo_route.left_gain == 0.25f);
    CHECK(next.stereo_route.right_gain == -0.5f);
    CHECK(next.effect_send_known);
    CHECK(next.effect_send_enabled);
    CHECK(!may_claim_authored_3d(next));

    return 0;
}
