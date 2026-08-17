#include "components/vgm/enhancement/ym2612_hq_source_calibration.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>

int main() {
    using namespace gameaudio::vgm;

    constexpr std::size_t frames = 8;
    const float hq[frames] = {-0.5f, -0.25f, 0.0f, 0.25f, 0.5f, 0.25f, 0.0f, -0.25f};
    std::int32_t left[frames]{};
    std::int32_t right[frames]{};
    for (std::size_t i = 0; i < frames; ++i) {
        left[i] = static_cast<std::int32_t>(std::lround(hq[i] * 20000.0));
        right[i] = static_cast<std::int32_t>(std::lround(hq[i] * 10000.0));
    }

    const auto calibration = calibrate_ym2612_hq_source(hq, left, right, frames);
    assert(calibration.valid);
    assert(calibration.active_frames == 6);
    assert(std::abs(calibration.left_gain - 20000.0) < 1.0);
    assert(std::abs(calibration.right_gain - 10000.0) < 1.0);

    ym2612_hq_calibrated_source_storage<frames> storage;
    assert(storage.build(hq, left, right, frames));
    assert(storage.valid());
    for (std::size_t i = 0; i < frames; ++i) {
        assert(std::abs(storage.left()[i] - static_cast<float>(left[i])) < 1.0f);
        assert(std::abs(storage.right()[i] - static_cast<float>(right[i])) < 1.0f);
    }

    const float silent[frames]{};
    assert(!storage.build(silent, left, right, frames));

    float bad[frames]{};
    bad[3] = NAN;
    assert(!storage.build(bad, left, right, frames));

    return 0;
}
