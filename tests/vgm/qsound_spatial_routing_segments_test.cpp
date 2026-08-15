#include "../../components/vgm/enhancement/qsound_spatial_routing_segments.h"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
    using namespace gameaudio::vgm;

    qsound_control_state initial;
    std::array<qsound_timed_source_control, 4> writes{{
        {2, {qsound_source_control_kind::pan, 0, 0x110}},
        {3, {qsound_source_control_kind::pan, 1, 0x130}},
        {4, {qsound_source_control_kind::pcm_echo_contribution, 0, 0x0200}},
        {6, {qsound_source_control_kind::pan, 0, 0x130}},
    }};
    std::array<qsound_source_spatial_segment, 8> segments{};

    const auto result = build_qsound_source_spatial_segments(
        initial, 0, 0, 7, writes.data(), writes.size(), 8,
        segments.data(), segments.size());

    assert(result.structurally_valid);
    assert(result.valid_order);
    assert(!result.overflowed);
    assert(result.segment_count == 4);

    assert(segments[0].begin_frame == 0 && segments[0].end_frame == 2);
    assert(segments[0].source.route.raw_pan == 0x120);
    assert(segments[0].source.route.dry_left == 1.0f);
    assert(segments[0].source.route.dry_right == 1.0f);
    assert(!segments[0].source.evidence.effect_send_enabled);

    assert(segments[1].begin_frame == 2 && segments[1].end_frame == 4);
    assert(segments[1].source.route.raw_pan == 0x110);
    assert(segments[1].source.route.dry_left == 1.0f);
    assert(segments[1].source.route.dry_right == 0.0f);
    assert(segments[1].source.route.wet_left == 0.0f);
    assert(segments[1].source.route.wet_right == 1.0f);
    assert(!segments[1].source.evidence.effect_send_enabled);

    assert(segments[2].begin_frame == 4 && segments[2].end_frame == 6);
    assert(segments[2].source.route.raw_pan == 0x110);
    assert(segments[2].source.echo_contribution_known);
    assert(segments[2].source.echo_contribution_raw == static_cast<std::int16_t>(0x0200));
    assert(segments[2].source.evidence.effect_send_enabled);

    assert(segments[3].begin_frame == 6 && segments[3].end_frame == 8);
    assert(segments[3].source.route.raw_pan == 0x130);
    assert(segments[3].source.route.dry_left == 0.0f);
    assert(segments[3].source.route.dry_right == 1.0f);
    assert(segments[3].source.route.wet_left == 1.0f);
    assert(segments[3].source.route.wet_right == 0.0f);
    assert(segments[3].source.evidence.effect_send_enabled);

    // The write for physical source 1 must not split source 0's timeline, but it
    // still belongs to the final shared control state.
    assert(result.final_state.pan(1) == 0x130);
    assert(result.final_state.pan(0) == 0x130);
    assert(result.final_state.pcm_echo_contribution(0) == static_cast<std::int16_t>(0x0200));

    // Same-sample writes collapse into one following state rather than emitting
    // zero-length segments.
    std::array<qsound_timed_source_control, 2> same_sample{{
        {2, {qsound_source_control_kind::pan, 0, 0x110}},
        {2, {qsound_source_control_kind::pcm_echo_contribution, 0, 0x0100}},
    }};
    const auto collapsed = build_qsound_source_spatial_segments(
        initial, 0, 0, 0, same_sample.data(), same_sample.size(), 4,
        segments.data(), segments.size());
    assert(collapsed.structurally_valid);
    assert(collapsed.segment_count == 2);
    assert(segments[0].begin_frame == 0 && segments[0].end_frame == 2);
    assert(segments[1].begin_frame == 2 && segments[1].end_frame == 4);
    assert(segments[1].source.route.raw_pan == 0x110);
    assert(segments[1].source.echo_contribution_raw == static_cast<std::int16_t>(0x0100));

    // A write exactly at the block end updates the carry state but does not
    // create a zero-length segment in the completed block.
    std::array<qsound_timed_source_control, 1> at_end{{
        {4, {qsound_source_control_kind::pan, 0, 0x130}},
    }};
    const auto end_write = build_qsound_source_spatial_segments(
        initial, 0, 0, 0, at_end.data(), at_end.size(), 4,
        segments.data(), segments.size());
    assert(end_write.segment_count == 1);
    assert(segments[0].begin_frame == 0 && segments[0].end_frame == 4);
    assert(segments[0].source.route.raw_pan == 0x120);
    assert(end_write.final_state.pan(0) == 0x130);

    // Out-of-order timing and insufficient output capacity fail visibly.
    std::array<qsound_timed_source_control, 2> out_of_order{{
        {3, {qsound_source_control_kind::pan, 0, 0x110}},
        {2, {qsound_source_control_kind::pan, 0, 0x130}},
    }};
    const auto bad_order = build_qsound_source_spatial_segments(
        initial, 0, 0, 0, out_of_order.data(), out_of_order.size(), 4,
        segments.data(), segments.size());
    assert(!bad_order.structurally_valid);
    assert(!bad_order.valid_order);

    const auto overflow = build_qsound_source_spatial_segments(
        initial, 0, 0, 0, writes.data(), writes.size(), 8,
        segments.data(), 1);
    assert(overflow.overflowed);

    const auto invalid_source = build_qsound_source_spatial_segments(
        initial, 0, static_cast<std::uint8_t>(qsound_source_count), 0,
        nullptr, 0, 4, segments.data(), segments.size());
    assert(!invalid_source.structurally_valid);
    assert(!invalid_source.valid_source);

    return 0;
}
