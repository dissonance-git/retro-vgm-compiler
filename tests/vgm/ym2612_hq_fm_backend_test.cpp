#include "components/vgm/enhancement/ym2612_hq_fm_backend.h"

#include <array>
#include <cassert>
#include <cmath>

namespace {

using gameaudio::vgm::ym2612_timed_write;

std::array<ym2612_timed_write, 32> make_basic_patch(std::size_t& count) {
    std::array<ym2612_timed_write, 32> writes{};
    auto push = [&](std::uint8_t port, std::uint8_t reg, std::uint8_t data) {
        assert(count < writes.size());
        writes[count++] = {0, port, reg, data};
    };

    // Channel 1, algorithm 7: four carriers. Fast attack, no decay/sustain
    // attenuation, equal ratios. This is intentionally simple enough that the
    // regression tests the transport/synthesis boundary rather than patch lore.
    constexpr std::uint8_t op_offsets[4] = {0x00, 0x08, 0x04, 0x0c};
    for (std::uint8_t offset : op_offsets) {
        push(0, static_cast<std::uint8_t>(0x30 + offset), 0x01); // ratio 1
        push(0, static_cast<std::uint8_t>(0x40 + offset), 0x00); // TL 0
        push(0, static_cast<std::uint8_t>(0x50 + offset), 0x1f); // fast AR
        push(0, static_cast<std::uint8_t>(0x60 + offset), 0x00); // DR 0
        push(0, static_cast<std::uint8_t>(0x70 + offset), 0x00); // SR 0
        push(0, static_cast<std::uint8_t>(0x80 + offset), 0x00); // SL 0, RR 0
    }
    push(0, 0xb0, 0x07);       // algorithm 7, feedback 0
    push(0, 0xb4, 0xc0);       // authored L+R route, no LFO sensitivity
    push(0, 0xa4, 0x22);       // block 4 + high FNUM bits
    push(0, 0xa0, 0x69);       // pitch commit
    push(0, 0x28, 0xf0);       // key on all four operators, channel 1
    return writes;
}

} // namespace

int main() {
    using namespace gameaudio::vgm;

    // The normal Enhanced contract stays inside the OPN2 family rather than
    // silently upgrading the composition to a different Yamaha synth topology.
    const auto reference_traits = traits_for(
        ym2612_enhanced_realization::reference_ym2612);
    const auto ym3438_traits = traits_for(
        ym2612_enhanced_realization::ym3438_opn2c);
    const auto ymf276_traits = traits_for(
        ym2612_enhanced_realization::ymf276_opn2l);
    assert(reference_traits.fm_channels == 6);
    assert(reference_traits.operators_per_channel == 4);
    assert(reference_traits.nominal_intermediate_bits == 9);
    assert(reference_traits.ym2612_dac_discontinuity);
    assert(!ym3438_traits.ym2612_dac_discontinuity);
    assert(ym3438_traits.nominal_intermediate_bits == 9);
    assert(ymf276_traits.fm_channels == 6);
    assert(ymf276_traits.operators_per_channel == 4);
    assert(ymf276_traits.algorithms == 8);
    assert(ymf276_traits.nominal_intermediate_bits == 14);
    assert(!ymf276_traits.ym2612_dac_discontinuity);
    assert(ymf276_traits.proper_channel_mix);
    assert(ymf276_traits.hardware_descendant);
    assert(default_ym2612_enhanced_realization
        == ym2612_enhanced_realization::ymf276_opn2l);

    ym2612_hq_fm_profile profile;
    profile.internal_oversample = 8;
    assert(profile.realization == ym2612_enhanced_realization::ymf276_opn2l);
    ym2612_hq_fm_backend backend(profile);
    assert(backend.configure({7670453, 44100, 48000}));
    assert(backend.configured());
    assert(backend.profile().automatic_enhanced_safe_shape());

    std::size_t write_count = 0;
    auto writes = make_basic_patch(write_count);
    assert(write_count == 29);

    constexpr std::size_t frames = 1024;
    std::array<std::array<float, frames>, ym2612_fm_backend::channel_count> storage{};
    float* outputs[ym2612_fm_backend::channel_count]{};
    for (std::size_t channel = 0; channel < ym2612_fm_backend::channel_count; ++channel)
        outputs[channel] = storage[channel].data();

    backend.render_timed(writes.data(), write_count, outputs, frames);
    assert(backend.semantic_coverage_complete());

    bool heard_channel0 = false;
    for (std::size_t frame = 0; frame < frames; ++frame) {
        for (std::size_t channel = 0; channel < ym2612_fm_backend::channel_count; ++channel)
            assert(std::isfinite(storage[channel][frame]));
        if (std::abs(storage[0][frame]) > 1.0e-5f)
            heard_channel0 = true;
        for (std::size_t channel = 1; channel < ym2612_fm_backend::channel_count; ++channel)
            assert(storage[channel][frame] == 0.0f);
    }
    assert(heard_channel0);

    // Null output pointers must not stop clock/synthesis-state advancement.
    backend.reset();
    assert(backend.configure({7670453, 44100, 48000}));
    float* no_outputs[ym2612_fm_backend::channel_count]{};
    backend.render_timed(writes.data(), write_count, no_outputs, 64);
    assert(backend.semantic_coverage_complete());

    // The first candidate fails closed on OPN semantics it has not yet matched
    // cycle-for-cycle. An enabled LFO therefore prevents automatic replacement.
    backend.reset();
    assert(backend.configure({7670453, 44100, 48000}));
    const ym2612_timed_write lfo[] = {{0, 0, 0x22, 0x0f}};
    backend.render_timed(lfo, 1, no_outputs, 1);
    assert(!backend.semantic_coverage_complete());

    // Channel 3 special mode is similarly fenced until its three independent
    // operator-frequency coordinates are rendered by the HQ backend.
    backend.reset();
    assert(backend.configure({7670453, 44100, 48000}));
    const ym2612_timed_write ch3_special[] = {{0, 0, 0x27, 0x40}};
    backend.render_timed(ch3_special, 1, no_outputs, 1);
    assert(!backend.semantic_coverage_complete());

    // Expanded modern-FM topology is a separate experiment, not normal
    // Enhanced playback.
    auto expanded = make_experimental_expanded_yamaha_fm_profile();
    assert(expanded.realization
        == ym2612_enhanced_realization::studio_precision_opn2);
    ym2612_hq_fm_backend expanded_backend(expanded);
    assert(expanded_backend.configure({7670453, 44100, 96000}));
    assert(!expanded_backend.profile().automatic_enhanced_safe_shape());

    return 0;
}