#include "../../components/vgm/enhancement/qsound_reference_recomposition.h"

#include <cstdint>
#include <limits>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    auto r = qsound_recompose_reference_channel(0, 0);
    CHECK(r.status == qsound_recomposition_status::exact);
    CHECK(r.output == 0);

    r = qsound_recompose_reference_channel(8191, 0);
    CHECK(r.output == 0);
    r = qsound_recompose_reference_channel(8192, 0);
    CHECK(r.output == 1);
    r = qsound_recompose_reference_channel(-8192, 0);
    CHECK(r.output == 0);
    r = qsound_recompose_reference_channel(-8193, 0);
    CHECK(r.output == -1);

    r = qsound_recompose_reference_channel(1000000000, 0);
    CHECK(r.status == qsound_recomposition_status::exact);
    CHECK(r.output == std::numeric_limits<std::int16_t>::max());
    r = qsound_recompose_reference_channel(-1000000000, 0);
    CHECK(r.status == qsound_recomposition_status::exact);
    CHECK(r.output == std::numeric_limits<std::int16_t>::min());

    r = qsound_recompose_reference_channel(std::numeric_limits<std::int32_t>::max(), 0);
    CHECK(r.status == qsound_recomposition_status::historical_int32_overflow_domain);
    r = qsound_recompose_reference_channel(std::numeric_limits<std::int32_t>::min(), -1);
    CHECK(r.status == qsound_recomposition_status::historical_int32_overflow_domain);

    qsound_native_mix_frame frame;
    frame.wet_post_delay = {{8192, -8193}};
    frame.dry_post_delay = {{0, 0}};
    frame.reference_output = {{1, -1}};
    CHECK(qsound_reference_frame_recomposes_exactly(frame));

    frame.reference_output[1] = 0;
    CHECK(!qsound_reference_frame_recomposes_exactly(frame));

    frame.reference_output[1] = -1;
    frame.wet_post_delay[0] = std::numeric_limits<std::int32_t>::max();
    CHECK(!qsound_reference_frame_recomposes_exactly(frame));

    return 0;
}
