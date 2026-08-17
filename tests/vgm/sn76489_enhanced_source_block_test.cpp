#include "components/vgm/enhancement/sn76489_enhanced_source_block.h"

#include <cassert>
#include <cmath>

int main() {
    using namespace gameaudio::vgm;

    sn76489_enhanced::config cfg;
    cfg.chip_clock_hz = 3579545.0;
    cfg.sample_rate_hz = 48000.0;
    cfg.clock_divider = 8;
    cfg.oversample = 8;
    cfg.sega_style_psg = true;
    cfg.negate_output = true;
    sn76489_enhanced synth(cfg);
    assert(synth.supported());

    // Tone 0 period 0x100, attenuation 0. Halfway through the block the exact
    // Game Gear stereo mask moves that source from both sides to right only.
    const sn76489_timed_write writes[] = {
        {0, sn76489_write_kind::data, 0x80},
        {0, sn76489_write_kind::data, 0x10},
        {0, sn76489_write_kind::data, 0x90},
        {8, sn76489_write_kind::stereo_mask, 0x01},
    };

    sn76489_enhanced_source_block_storage<16> block;
    assert(block.render(synth, writes, 4, false, 16, 256, 128));
    assert(block.valid());
    assert(block.frame_count() == 16);

    const auto tone0 = block.source(0);
    assert(tone0.left != nullptr && tone0.right != nullptr);
    const float* mono0 = block.mono(0);
    assert(mono0 != nullptr);

    bool heard_before_mask = false;
    bool heard_after_mask = false;
    for (std::size_t frame = 0; frame < 8; ++frame) {
        // Both sides are enabled before the mask transition. The exact libvgm
        // device volumes differ 2:1, so their source-domain amplitudes do too.
        assert(std::abs(tone0.left[frame] - mono0[frame] * 4096.0f * 256.0f) < 0.125f);
        assert(std::abs(tone0.right[frame] - mono0[frame] * 4096.0f * 128.0f) < 0.125f);
        if (std::abs(mono0[frame]) > 1.0e-5f)
            heard_before_mask = true;
    }
    for (std::size_t frame = 8; frame < 16; ++frame) {
        assert(tone0.left[frame] == 0.0f);
        assert(std::abs(tone0.right[frame] - mono0[frame] * 4096.0f * 128.0f) < 0.125f);
        if (std::abs(tone0.right[frame]) > 1.0e-5f)
            heard_after_mask = true;
    }
    assert(heard_before_mask);
    assert(heard_after_mask);

    // No other source was opened by the same writes.
    for (std::size_t source = 1; source < sn76489_enhanced::stem_count; ++source) {
        const auto lane = block.source(source);
        for (std::size_t frame = 0; frame < 16; ++frame) {
            assert(lane.left[frame] == 0.0f);
            assert(lane.right[frame] == 0.0f);
        }
    }

    // Capture overflow is an evidence failure. Do not synthesize a plausible
    // block from a known-incomplete register trajectory.
    sn76489_enhanced overflowed(cfg);
    assert(!block.render(overflowed, writes, 4, true, 16, 256, 256));
    assert(block.last_error() == sn76489_enhanced_source_block_error::capture_overflow);

    // Timed writes are causal. An out-of-order capture is rejected rather than
    // silently sorted into a different execution history.
    const sn76489_timed_write unsorted[] = {
        {4, sn76489_write_kind::data, 0x90},
        {3, sn76489_write_kind::data, 0x9F},
    };
    sn76489_enhanced bad_order(cfg);
    assert(!block.render(bad_order, unsorted, 2, false, 16, 256, 256));
    assert(block.last_error() == sn76489_enhanced_source_block_error::unsorted_write);

    return 0;
}
