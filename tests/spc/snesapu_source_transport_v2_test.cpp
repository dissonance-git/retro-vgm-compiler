#include "components/spc/snesapu_source_transport_v2.h"

#include <array>
#include <cassert>

int main() {
    using gameaudio::spc::snesapu_source_transport_v2;
    using gameaudio::spc::spc_source_bus;

    snesapu_source_transport_v2::header header{};
    header.magic = snesapu_source_transport_v2::magic;
    header.version = snesapu_source_transport_v2::version;
    header.header_size = sizeof(header);
    header.block_samples = 3;
    header.plane_count = snesapu_source_transport_v2::plane_count;
    header.sample_format = snesapu_source_transport_v2::format_float32;
    header.audio_lanes = snesapu_source_transport_v2::audio_lane_count;

    std::array<float, snesapu_source_transport_v2::plane_count * 3> planes{};
    for (std::size_t plane = 0; plane < snesapu_source_transport_v2::plane_count; ++plane) {
        for (std::size_t frame = 0; frame < 3; ++frame)
            planes[plane * 3 + frame] = static_cast<float>(plane * 10 + frame);
    }

    const snesapu_source_transport_v2::view view{&header, planes.data()};
    assert(view.valid());
    assert(view.frame_count() == 3);
    assert(view.dry_voice(0)[2] == 2.0f);
    assert(view.dry_voice(7)[1] == 71.0f);
    assert(view.gain_left(0)[0] == 80.0f);
    assert(view.gain_left(7)[2] == 152.0f);
    assert(view.gain_right(0)[1] == 161.0f);
    assert(view.gain_right(7)[0] == 230.0f);
    assert(view.echo_left()[2] == 242.0f);
    assert(view.echo_right()[1] == 251.0f);
    assert(view.dry_voice(8) == nullptr);

    const auto bus = view.source_bus_view();
    assert(bus.frame_count == 3);
    assert(bus.dry_voice[0] == view.dry_voice(0));
    assert(bus.dry_voice[7] == view.dry_voice(7));
    assert(bus.echo_left == view.echo_left());
    assert(bus.echo_right == view.echo_right());

    // SRCE v2 wet planes are already post-EVOL. Preserve EVOL as signed route
    // evidence without applying the same gain a second time downstream.
    const auto wet_l = spc_source_bus::make_preapplied_echo_source(
        spc_source_bus::echo_side::left, 5, -64);
    const auto wet_r = spc_source_bus::make_preapplied_echo_source(
        spc_source_bus::echo_side::right, 5, 96);
    assert(wet_l.stereo_route.gain_preapplied);
    assert(wet_r.stereo_route.gain_preapplied);
    assert(wet_l.stereo_route.left_gain < 0.0f);
    assert(wet_l.stereo_route.right_gain == 0.0f);
    assert(wet_r.stereo_route.left_gain == 0.0f);
    assert(wet_r.stereo_route.right_gain > 0.0f);
    assert(wet_l.persistent_part_id == wet_r.persistent_part_id);

    auto bad = header;
    bad.version = 1;
    const snesapu_source_transport_v2::view wrong_version{&bad, planes.data()};
    assert(!wrong_version.valid());
    assert(wrong_version.frame_count() == 0);
    assert(wrong_version.plane(0) == nullptr);

    bad = header;
    bad.block_samples = snesapu_source_transport_v2::max_frames + 1;
    const snesapu_source_transport_v2::view too_large{&bad, planes.data()};
    assert(!too_large.valid());

    const snesapu_source_transport_v2::view no_audio{&header, nullptr};
    assert(!no_audio.valid());

    return 0;
}
