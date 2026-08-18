#include "../../components/spc/spc_native_routed_source_projection.h"
#include "../../components/spc/spc_runtime_spatial_adapter.h"
#include "../../components/spc/spc_snapshot_spatial_seed.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

namespace {

bool near(float left, float right, float tolerance = 1.0e-6f)
{
    return std::fabs(left - right) <= tolerance;
}

} // namespace

int main()
{
    using namespace gameaudio::spc;
    using namespace vgmtooling::model;

    constexpr std::uint64_t tick_rate = 1024000;
    constexpr std::uint64_t sample_rate = 32000;
    constexpr std::size_t frames = 4;

    spc_snapshot snapshot{};
    // Voice 0: full negative left, silence right. Voice 1: both zero. Other
    // voices use equal +64 routes. EON proves effect-send state seeds separately
    // from route magnitude.
    snapshot.dsp[0x00] = 0x80;
    snapshot.dsp[0x01] = 0x00;
    snapshot.dsp[0x10] = 0x00;
    snapshot.dsp[0x11] = 0x00;
    for (std::size_t voice = 2; voice < 8; ++voice) {
        snapshot.dsp[voice * 0x10u] = 0x40;
        snapshot.dsp[voice * 0x10u + 1u] = 0x40;
    }
    snapshot.dsp[0x4D] = 0x05; // voices 0 and 2 echo-send enabled.

    const auto seed = make_spc_snapshot_spatial_seed(snapshot, tick_rate);
    assert(seed[0].route_gain_left == -128);
    assert(seed[0].route_gain_right == 0);
    assert(seed[0].echo_send_enabled);
    assert(seed[1].route_gain_left == 0);
    assert(seed[1].route_gain_right == 0);
    assert(!seed[1].echo_send_enabled);
    assert(seed[2].route_gain_left == 64);
    assert(seed[2].route_gain_right == 64);
    assert(seed[2].echo_send_enabled);

    // Feed the exact snapshot seed through the same runtime adapter used by the
    // corpus runner. spc_runtime_capture owns trace ordinals; the seed records do
    // not get a parallel identity system.
    spc_runtime_capture capture{};
    capture.reset_trace();
    for (const auto& record : seed)
        capture.observe(record);

    spc_runtime_spatial_adapter<16, 32> adapter{};
    const spc_runtime_spatial_capture_view view{
        capture.records(),
        capture.count(),
        capture.overflowed(),
    };
    assert(adapter.build_window(view, 0, tick_rate, sample_rate, frames));
    assert(adapter.segment_count() == 1);
    const auto& segment = adapter.segments()[0];
    assert(segment.sources.lane_count == 8);
    assert(segment.sources.frame_count == frames);
    assert(segment.sources.lanes[0].evidence.stereo_route.present);
    assert(near(segment.sources.lanes[0].evidence.stereo_route.left_gain, -1.0f));
    assert(near(segment.sources.lanes[0].evidence.stereo_route.right_gain, 0.0f));
    assert(segment.sources.lanes[0].evidence.effect_send_known);
    assert(segment.sources.lanes[0].evidence.effect_send_enabled);

    std::array<std::array<float, frames>, 8> dry{};
    for (std::size_t voice = 0; voice < dry.size(); ++voice) {
        for (std::size_t frame = 0; frame < frames; ++frame)
            dry[voice][frame] = 1.0f;
    }
    std::array<const float*, 8> dry_view{};
    for (std::size_t voice = 0; voice < dry.size(); ++voice)
        dry_view[voice] = dry[voice].data();

    spc_native_routed_source_projection_storage<frames, 32> projection{};
    assert(projection.build(segment.sources, dry_view));
    const auto& routed = projection.block();
    assert(routed.lane_count == 8);

    // [-1,0] has energy-equivalent mono gain sqrt(1/2). [0,0] is exactly
    // inaudible. [+0.5,+0.5] preserves 0.5. Signed side evidence remains intact
    // and is explicitly marked preapplied rather than multiplied twice later.
    assert(near(routed.lanes[0].mono_pcm[0], static_cast<float>(std::sqrt(0.5))));
    assert(near(routed.lanes[1].mono_pcm[0], 0.0f));
    assert(near(routed.lanes[2].mono_pcm[0], 0.5f));
    assert(routed.lanes[0].evidence.stereo_route.gain_preapplied);
    assert(near(routed.lanes[0].evidence.stereo_route.left_gain, -1.0f));

    // A timed hardware route change affects samples at and after its exact frame
    // boundary, while the outgoing event carries the same signed route evidence
    // with amplitude marked preapplied.
    spatial_source_evidence_event event{};
    event.frame_offset = 2;
    event.lane_index = 0;
    event.evidence = segment.sources.lanes[0].evidence;
    event.evidence.stereo_route.left_gain = 0.0f;
    event.evidence.stereo_route.right_gain = 0.0f;

    auto timed_segment = segment.sources;
    timed_segment.evidence_events = &event;
    timed_segment.evidence_event_count = 1;
    assert(projection.build(timed_segment, dry_view));
    const auto& timed = projection.block();
    assert(timed.lanes[0].mono_pcm[0] > 0.70f);
    assert(timed.lanes[0].mono_pcm[1] > 0.70f);
    assert(timed.lanes[0].mono_pcm[2] == 0.0f);
    assert(timed.lanes[0].mono_pcm[3] == 0.0f);
    assert(timed.evidence_event_count == 1);
    assert(timed.evidence_events[0].evidence.stereo_route.gain_preapplied);

    return 0;
}
