#include "components/vgm/enhancement/genesis_source_spread_7_1.h"

#include <cassert>
#include <cmath>
#include <cstddef>

namespace {

using namespace gameaudio::vgm;
using vgmtooling::model::surround_7_1_bed_storage;
using vgmtooling::model::surround_7_1_channel;

bool near(float a, float b, float tolerance = 1.0e-5f) {
    return std::fabs(a - b) <= tolerance;
}

std::size_t channel_index(surround_7_1_channel channel) {
    return surround_7_1_bed_storage<1>::index(channel);
}

} // namespace

int main() {
    // Every Genesis implementation lane receives exactly the same front-anchor
    // law. The source index selects only a low-discrepancy side/back split.
    for (std::size_t source_index = 0;
         source_index < genesis_recomposition_source_count;
         ++source_index) {
        genesis_selected_source_queue<4> queue;
        queue.reset(100u);

        genesis_selected_source_frame frame{};
        frame.ordinal = 100u;
        frame.source[source_index] = {1.0, 0.5, true, true};
        assert(queue.push_reference(frame));

        genesis_selected_source_block_storage<1> selected;
        assert(selected.consume(queue, 100u, 1u));

        const double reference[2] = {1.0, 0.5};
        surround_7_1_bed_storage<1> bed;
        assert(project_genesis_source_spread_7_1(
            selected, reference, 1u, bed));

        const auto gains = genesis_source_spread_gains_for(source_index);
        const float* out = bed.data();
        assert(out != nullptr);

        assert(near(out[channel_index(surround_7_1_channel::front_left)], gains.front));
        assert(near(out[channel_index(surround_7_1_channel::front_right)], 0.5f * gains.front));
        assert(near(out[channel_index(surround_7_1_channel::side_left)], gains.side));
        assert(near(out[channel_index(surround_7_1_channel::side_right)], 0.5f * gains.side));
        assert(near(out[channel_index(surround_7_1_channel::back_left)], gains.back));
        assert(near(out[channel_index(surround_7_1_channel::back_right)], 0.5f * gains.back));
        assert(near(out[channel_index(surround_7_1_channel::front_center)], 0.0f));
        assert(near(out[channel_index(surround_7_1_channel::lfe)], 0.0f));

        const float power =
            gains.front * gains.front
            + gains.side * gains.side
            + gains.back * gains.back;
        assert(near(power, 1.0f));
        assert(gains.front > 0.0f);
    }

    return 0;
}
