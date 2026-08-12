#include "../../components/vgm/enhancement/authored_stereo_route.h"

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

    return 0;
}
