#include "yamaha_opm_register.h"
#include "ym2151_authority_state.h"
#include "ym2151_enhanced_recomposition.h"
#include "ym2151_selected_source_transport.h"
#include "ym2151_spatial_route_transport.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

using namespace gameaudio::vgm;

namespace {
command_event opm_command(std::uint8_t opcode, const std::uint8_t* payload) {
    command_event event{};
    event.kind = command_event_kind::command;
    event.command = opcode;
    event.payload = payload;
    event.payload_size = 2u;
    return event;
}
}

int main() {
    // OPM key writes select one of 8 channels directly in bits 0-2 and carry
    // the four-operator key mask in bits 3-6. This is not OPN key encoding.
    static_assert(opm_key_channel(0x5D) == 5);
    static_assert(opm_key_operator_mask(0x5D) == 0x0B);

    static_assert(opm_algorithm_feedback_register(0x20));
    static_assert(opm_algorithm_feedback_register(0x27));
    static_assert(!opm_algorithm_feedback_register(0x28));
    static_assert(opm_channel_register(0x25, 0x20).value() == 5);

    constexpr auto af = opm_algorithm_feedback(0x2D);
    static_assert(af.algorithm == 5);
    static_assert(af.feedback == 5);

    // OPM pitch is key-code + key-fraction, not FNUM + block.
    static_assert(opm_key_code_register(0x28));
    static_assert(opm_key_code_register(0x2F));
    static_assert(!opm_key_code_register(0x30));
    static_assert(opm_key_fraction_register(0x30));
    static_assert(opm_key_fraction_register(0x37));
    static_assert(!opm_key_fraction_register(0x38));

    constexpr auto pitch = decode_opm_programmed_pitch(0x5A, 0xFC);
    static_assert(pitch.key_code == 0x5A);
    static_assert(pitch.octave == 5);
    static_assert(pitch.note_code == 0x0A);
    static_assert(pitch.key_fraction == 63);
    static_assert(pitch.packed_block_frequency == ((0x5Au << 6) | 63u));

    // Bit 7 of the key-code source byte is not part of the seven-bit OPM code.
    constexpr auto masked_pitch = decode_opm_programmed_pitch(0xDA, 0xFF);
    static_assert(masked_pitch.key_code == 0x5A);
    static_assert(masked_pitch.key_fraction == 63);

    static_assert(opm_lfo_sensitivity_register(0x38));
    static_assert(opm_lfo_sensitivity_register(0x3F));
    static_assert(!opm_lfo_sensitivity_register(0x40));

    // OPM operator addressing uses channel bits 0-2 and slot bits 3-4.
    // The physical slot order 1,3,2,4 maps through the same cross-family
    // logical order now used by OPN.
    static_assert(opm_operator_register(0x40));
    static_assert(opm_operator_register(0xFF));
    static_assert(!opm_operator_register(0x3F));
    static_assert(opm_operator_channel(0x45) == 5);
    static_assert(opm_operator_from_register(0x40) == 0);
    static_assert(opm_operator_from_register(0x48) == 2);
    static_assert(opm_operator_from_register(0x50) == 1);
    static_assert(opm_operator_from_register(0x58) == 3);

    // OPM stereo bits are opposite address geometry from OPN but still retain
    // authored left/right routing explicitly.
    constexpr auto left_only = decode_opm_stereo_route(0x40);
    constexpr auto right_only = decode_opm_stereo_route(0x80);
    constexpr auto both = decode_opm_stereo_route(0xC0);
    static_assert(left_only.left && !left_only.right);
    static_assert(!right_only.left && right_only.right);
    static_assert(both.left && both.right);

    // The authority shadow records authored OPM controls without pretending to
    // own renderer-internal phase/envelope/timer evolution.
    ym2151_authority_state authority;
    authority.apply_register(0x20u, static_cast<std::uint8_t>(0x40u | (5u << 3u) | 6u));
    authority.apply_register(0x08u, static_cast<std::uint8_t>((0x0Bu << 3u) | 0u));
    authority.apply_register(0x40u, 0x7Fu);
    authority.apply_register(0x19u, 0x45u); // AMD
    authority.apply_register(0x19u, 0xC2u); // PMD
    assert(authority.channel(0).route_left && !authority.channel(0).route_right);
    assert(authority.channel(0).feedback == 5u && authority.channel(0).algorithm == 6u);
    assert(authority.channel(0).key_mask == 0x0Bu);
    assert(authority.channel(0).operators[0].dt1 == 7u);
    assert(authority.channel(0).operators[0].multiple == 15u);
    assert(authority.global().amd == 0x45u);
    assert(authority.global().pmd == 0x42u);

    // YM2151 is the first non-Genesis client of the generic source-family
    // transaction engine. Eight complete OPM channels are replacement sources;
    // the four operators inside a channel remain one coupled synthesis object.
    constexpr std::size_t frames = 2;
    const std::array<float, frames> reference_l{10.0f, 20.0f};
    const std::array<float, frames> reference_r{-10.0f, -20.0f};
    const std::array<float, frames> exact_l{1.0f, 2.0f};
    const std::array<float, frames> exact_r{-0.5f, -1.0f};
    const std::array<float, frames> enhanced_l{2.0f, 4.0f};
    const std::array<float, frames> enhanced_r{-1.0f, -2.0f};

    std::array<ym2151_source_replacement_view, ym2151_recomposition_source_count> sources{};
    auto& fm1 = sources[static_cast<std::size_t>(ym2151_recomposition_source::fm1)];
    fm1.reference = {exact_l.data(), exact_r.data(), true};
    fm1.enhanced = {enhanced_l.data(), enhanced_r.data(), true};
    fm1.replace = true;

    ym2151_enhanced_recomposition_storage<frames> recomposition;
    assert(recomposition.build_independent_families(
        reference_l.data(), reference_r.data(), frames, sources));
    assert(recomposition.valid());
    assert(recomposition.used_replacement());
    assert(!recomposition.had_family_failure());
    assert(recomposition.family_status(ym2151_recomposition_family::fm).applied);
    assert(std::fabs(recomposition.left()[0] - 11.0f) < 1.0e-6f);
    assert(std::fabs(recomposition.right()[0] + 10.5f) < 1.0e-6f);

    // Any invalid requested OPM channel fails the enhanced FM family closed to
    // the protected reference rather than applying a guessed partial family.
    auto failed = sources;
    failed[static_cast<std::size_t>(ym2151_recomposition_source::fm8)] = fm1;
    failed[static_cast<std::size_t>(ym2151_recomposition_source::fm8)].enhanced.exact = false;
    assert(recomposition.build_independent_families(
        reference_l.data(), reference_r.data(), frames, failed));
    assert(recomposition.valid());
    assert(!recomposition.used_replacement());
    assert(recomposition.had_family_failure());
    const auto failed_status = recomposition.family_status(ym2151_recomposition_family::fm);
    assert(failed_status.requested && !failed_status.applied);
    assert(failed_status.error ==
        ym2151_enhanced_recomposition_error::missing_enhanced_source);
    assert(recomposition.left()[0] == reference_l[0]);
    assert(recomposition.right()[0] == reference_r[0]);

    // The same eight channel identities survive the producer/render-ahead
    // boundary. The queue contains the already-selected source quality, while
    // the delivered block only enforces exact provenance and host-clock order.
    constexpr std::size_t fm1_index =
        static_cast<std::size_t>(ym2151_recomposition_source::fm1);
    constexpr std::size_t fm8_index =
        static_cast<std::size_t>(ym2151_recomposition_source::fm8);
    ym2151_selected_source_queue<4> queue;
    queue.reset(500u);
    ym2151_selected_source_frame source_frame{};
    source_frame.ordinal = 500u;
    source_frame.source[fm1_index] = {1.0, -1.0, true, true};
    source_frame.source[fm8_index] = {0.0, 0.0, true, true};
    assert(queue.push_reference(source_frame));
    assert(queue.replace_source(500u, fm1_index, 2.0, -2.0, true));

    ym2151_selected_source_block_storage<2> delivered;
    assert(delivered.consume(queue, 500u, 1u));
    assert(delivered.valid());
    assert(delivered.source_present(fm1_index));
    assert(!delivered.source_present(fm8_index));
    assert(delivered.sources()[fm1_index].left[0] == 2.0f);
    assert(delivered.sources()[fm1_index].right[0] == -2.0f);

    // Authored route evidence now crosses the same delivered clock. OPM starts
    // without guessed reset routing, so Spatial is incomplete until a real RL
    // register write or a proven seek-state seed establishes channel evidence.
    ym2151_spatial_route_transport<8, 4> routes;
    routes.reset();
    ym2151_spatial_route_transport<8, 4>::presence_array present{};
    present[fm1_index] = true;
    ym2151_spatial_route_transport<8, 4>::delivered_block route_block{};
    assert(routes.prepare_delivered_block(600u, 1u, present, route_block));
    assert(!route_block.routes_complete);

    const std::uint8_t fm1_left_route[] = {0x20u, 0x40u};
    routes.reset();
    assert(routes.observe(opm_command(0x54u, fm1_left_route), 700u));
    assert(routes.prepare_delivered_block(700u, 1u, present, route_block));
    assert(route_block.routes_complete);
    assert(route_block.initial_evidence[fm1_index].stereo_route.left_gain == 1.0f);
    assert(route_block.initial_evidence[fm1_index].stereo_route.right_gain == 0.0f);

    return 0;
}
