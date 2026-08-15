#include "../../components/vgm/enhancement/authored_stereo_route.h"

using gameaudio::vgm::sn76489_authored_route;
using gameaudio::vgm::ym2612_authored_route;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    const auto mute = ym2612_authored_route(false, false);
    CHECK(mute.left == 0.0f && mute.right == 0.0f);

    const auto left = ym2612_authored_route(true, false);
    CHECK(left.left == 1.0f && left.right == 0.0f);

    const auto right = ym2612_authored_route(false, true);
    CHECK(right.left == 0.0f && right.right == 1.0f);

    const auto both = ym2612_authored_route(true, true);
    CHECK(both.left == 1.0f && both.right == 1.0f);

    // Game Gear/SN76489 routing: high nibble = left channels 0..3,
    // low nibble = right channels 0..3.
    const auto psg0_left = sn76489_authored_route(0x10, 0);
    CHECK(psg0_left.left == 1.0f && psg0_left.right == 0.0f);

    const auto psg0_right = sn76489_authored_route(0x01, 0);
    CHECK(psg0_right.left == 0.0f && psg0_right.right == 1.0f);

    const auto psg2_both = sn76489_authored_route(0x44, 2);
    CHECK(psg2_both.left == 1.0f && psg2_both.right == 1.0f);

    const auto noise_muted = sn76489_authored_route(0x77, 3);
    CHECK(noise_muted.left == 0.0f && noise_muted.right == 0.0f);

    const auto noise_both = sn76489_authored_route(0x88, 3);
    CHECK(noise_both.left == 1.0f && noise_both.right == 1.0f);

    // Source index wraps only to the four physical PSG outputs.
    const auto wrapped = sn76489_authored_route(0x22, 5);
    CHECK(wrapped.left == 1.0f && wrapped.right == 1.0f);

    return 0;
}
