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

    // 0x80 is exact zero. One frame into the 0x80 -> 0xC0 ramp is 0.125
    // normalized source level, then converted into the ideal no-ladder OPN2
    // source coordinate and libvgm device gain.
    CHECK(block.left()[0] == 0.0);
    CHECK(std::abs(block.left()[1] - 0.125 * 8448.0 * 2.0) < 1.0e-9);
    CHECK(block.right()[1] == 0.0);

    // 0xC0 is +0.5 in the preserved asymmetric 8-bit DAC code mapping.
    CHECK(std::abs(block.left()[4] - 0.5 * 8448.0 * 2.0) < 1.0e-9);

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
