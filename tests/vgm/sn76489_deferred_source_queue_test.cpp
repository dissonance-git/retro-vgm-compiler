#include "components/vgm/enhancement/sn76489_deferred_source_queue.h"
#include "components/vgm/enhancement/sn76489_enhanced_source_block.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {
using namespace gameaudio::vgm;

std::int64_t expected_side(
    const sn76489_enhanced_source_block_storage<128>& block,
    std::size_t frame,
    bool left)
{
    std::int64_t total = 0;
    for (std::size_t channel = 0; channel < sn76489_enhanced::stem_count; ++channel) {
        const auto source = block.source(channel);
        const float value = left ? source.left[frame] : source.right[frame];
        total += static_cast<std::int64_t>(std::llround(value));
    }
    return total;
}
}

int main() {
    using namespace gameaudio::vgm;

    sn76489_enhanced::config cfg;
    cfg.chip_clock_hz = 3579545.0;
    cfg.sample_rate_hz = 48000.0;
    cfg.white_noise_feedback = 0x0009;
    cfg.shift_register_width = 16;
    cfg.clock_divider = 8;
    cfg.oversample = 4;
    cfg.sega_style_psg = true;

    sn76489_enhanced block_synth(cfg);
    sn76489_enhanced stream_synth(cfg);
    assert(block_synth.supported() && stream_synth.supported());

    const std::array<sn76489_timed_write, 10> writes{{
        {0u, sn76489_write_kind::register_write, 0x84u},
        {0u, sn76489_write_kind::register_write, 0x12u},
        {0u, sn76489_write_kind::register_write, 0x90u},
        {7u, sn76489_write_kind::register_write, 0xA8u},
        {7u, sn76489_write_kind::register_write, 0x08u},
        {7u, sn76489_write_kind::register_write, 0xB3u},
        {11u, sn76489_write_kind::stereo_mask, 0xF1u},
        {31u, sn76489_write_kind::register_write, 0xE4u},
        {31u, sn76489_write_kind::register_write, 0xF2u},
        {64u, sn76489_write_kind::stereo_mask, 0xFFu},
    }};

    constexpr std::size_t frame_count = 64u;
    constexpr std::int16_t volume_left = 192;
    constexpr std::int16_t volume_right = 160;

    sn76489_enhanced_source_block_storage<128> block;
    assert(block.render(
        block_synth,
        writes.data(),
        writes.size(),
        false,
        frame_count,
        volume_left,
        volume_right));

    sn76489_deferred_source_queue<128, 9> queue;
    constexpr std::uint64_t base = 1000u;
    queue.reset(base);

    // Reproduce command-observer semantics: generate the command-free interval
    // before every event, then mutate the synth at that exact absolute ordinal.
    for (const auto& write : writes) {
        const std::uint64_t ordinal = base + write.sample_offset;
        assert(queue.render_until(stream_synth, ordinal, volume_left, volume_right));
        if (write.kind == sn76489_write_kind::stereo_mask)
            stream_synth.write_stereo_mask(write.data);
        else
            stream_synth.write(write.data);
    }
    assert(queue.render_until(
        stream_synth, base + frame_count, volume_left, volume_right));
    assert(queue.size() == frame_count);

    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        sn76489_deferred_source_frame streamed{};
        assert(queue.pop_expected(base + frame, streamed));
        assert(streamed.left == expected_side(block, frame, true));
        assert(streamed.right == expected_side(block, frame, false));
    }
    assert(queue.size() == 0);

    // The offset==frames write affected both synth states after the compared
    // audio. The following interval must therefore remain equivalent too.
    sn76489_enhanced_source_block_storage<128> next_block;
    assert(next_block.render(
        block_synth,
        nullptr,
        0,
        false,
        8,
        volume_left,
        volume_right));
    assert(queue.render_until(stream_synth, base + frame_count + 8u,
        volume_left, volume_right));
    for (std::size_t frame = 0; frame < 8; ++frame) {
        sn76489_deferred_source_frame streamed{};
        assert(queue.pop_expected(base + frame_count + frame, streamed));
        assert(streamed.left == expected_side(next_block, frame, true));
        assert(streamed.right == expected_side(next_block, frame, false));
    }

    // Capacity failure is checked before synthesis advances. It invalidates the
    // queue so a caller can keep the protected reference family instead.
    sn76489_enhanced small_synth(cfg);
    sn76489_deferred_source_queue<4> small;
    small.reset(77u);
    assert(!small.render_until(small_synth, 82u, volume_left, volume_right));
    assert(!small.valid());
    assert(small.size() == 0);

    // Absolute ordinals are identity, not decoration.
    queue.reset(500u);
    assert(queue.render_until(stream_synth, 501u, volume_left, volume_right));
    sn76489_deferred_source_frame wrong{};
    assert(!queue.pop_expected(499u, wrong));
    assert(!queue.valid());

    return 0;
}
