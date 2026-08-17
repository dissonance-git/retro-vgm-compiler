#include "components/vgm/enhancement/ym2612_dac_enhanced_source_block.h"

#include <cmath>
#include <cstddef>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    ym2612_dac_enhanced state;
    ym2612_dac_enhanced_source_block_storage<16> block;

    const ym2612_dac_timed_event events[] = {
        {0, ym2612_dac_event_kind::enable, 0x80},
        {0, ym2612_dac_event_kind::data, 0x80},
        {4, ym2612_dac_event_kind::data, 0xC0},
    };

    CHECK(block.render(state, events, 3, 8, true, false, 2, 3));
    CHECK(block.valid());
    CHECK(block.frames() == 8);

    // Direct writes retain their exact hold semantics. The first code is zero
    // through frame 3; the new +0.5 code begins exactly at frame 4.
    for (std::size_t frame = 0; frame < 4; ++frame) {
        CHECK(block.left()[frame] == 0.0);
        CHECK(block.right()[frame] == 0.0);
    }
    CHECK(std::abs(block.left()[4] - 0.5 * 8448.0 * 2.0) < 1.0e-9);
    CHECK(block.right()[4] == 0.0);

    // Stereo routing is explicit source evidence, not inferred from the
    // distorted reference lane.
    state.reset();
    CHECK(block.render(state, events, 3, 8, false, true, 2, 3));
    CHECK(block.left()[4] == 0.0);
    CHECK(std::abs(block.right()[4] - 0.5 * 8448.0 * 3.0) < 1.0e-9);

    // Capacity and malformed event input fail closed.
    CHECK(!block.render(state, events, 3, 17, true, true, 1, 1));
    CHECK(!block.valid());
    CHECK(!block.render(state, nullptr, 1, 8, true, true, 1, 1));
    CHECK(!block.valid());

    return 0;
}
