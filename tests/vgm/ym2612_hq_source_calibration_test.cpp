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

    // Unit calibration must not count a center-routed source twice merely
    // because the identical physical channel is enabled on both outputs.
    std::int32_t centered_left[frames]{};
    std::int32_t centered_right[frames]{};
    for (std::size_t i = 0; i < frames; ++i) {
        centered_left[i] = static_cast<std::int32_t>(std::lround(hq[i] * 20000.0));
        centered_right[i] = centered_left[i];
    }
    const auto center_unit = calibrate_ym2612_hq_unit_gain(
        hq, centered_left, centered_right, frames, ym2612_authored_route(true, true));
    assert(center_unit.valid);
    assert(std::abs(center_unit.gain - 20000.0) < 1.0);

    // A both-sides-disabled authored route is silence, not corrupt evidence.
    const auto muted_unit = calibrate_ym2612_hq_unit_gain(
        hq, centered_left, centered_right, frames, ym2612_authored_route(false, false));
    assert(!muted_unit.valid);

    // Realtime gain accumulates across decode blocks and freezes only after a
    // bounded amount of active exact-reference evidence exists.
    constexpr std::size_t half = frames / 2;
    ym2612_hq_frozen_unit_gain frozen(6);
    assert(frozen.observe(
        hq, centered_left, centered_right, half, ym2612_authored_route(true, true)));
    assert(!frozen.ready());
    assert(frozen.observe(
        hq + half, centered_left + half, centered_right + half, half,
        ym2612_authored_route(true, true)));
    assert(frozen.ready());
    const double frozen_gain = frozen.gain();
    assert(std::abs(frozen_gain - 20000.0) < 1.0);

    // Muted routing after calibration neither fails nor retunes the source.
    assert(frozen.observe(
        hq, centered_left, centered_right, frames, ym2612_authored_route(false, false)));
    assert(frozen.gain() == frozen_gain);

    // A later louder block must not make decode block boundaries behave like
    // an automatic leveler.
    std::int32_t louder_left[frames]{};
    std::int32_t louder_right[frames]{};
    for (std::size_t i = 0; i < frames; ++i) {
        louder_left[i] = static_cast<std::int32_t>(std::lround(hq[i] * 40000.0));
        louder_right[i] = louder_left[i];
    }
    assert(frozen.observe(
        hq, louder_left, louder_right, frames, ym2612_authored_route(true, true)));
    assert(frozen.gain() == frozen_gain);

    // Routing is authored state, not something inferred from RMS balance. The
    // frozen unit gain can therefore survive a later pan change without
    // changing the source's calibrated identity.
    ym2612_hq_frozen_source_storage<frames> frozen_storage;
    assert(frozen_storage.build(
        hq, frames, frozen, ym2612_authored_route(false, true)));
    for (std::size_t i = 0; i < frames; ++i) {
        assert(std::abs(frozen_storage.left()[i]) < 1.0e-6f);
        assert(std::abs(
            frozen_storage.right()[i] - static_cast<float>(hq[i] * frozen_gain)) < 1.0f);
    }

    // A muted route also renders valid silence after calibration.
    assert(frozen_storage.build(
        hq, frames, frozen, ym2612_authored_route(false, false)));
    for (std::size_t i = 0; i < frames; ++i) {
        assert(std::abs(frozen_storage.left()[i]) < 1.0e-6f);
        assert(std::abs(frozen_storage.right()[i]) < 1.0e-6f);
    }

    const float silent[frames]{};
    assert(!storage.build(silent, left, right, frames));

    float bad[frames]{};
    bad[3] = NAN;
    assert(!storage.build(bad, left, right, frames));

    // Invalid calibration evidence fails closed before a replacement becomes
    // ready; reset permits a clean reconstruction from a new gain domain.
    ym2612_hq_frozen_unit_gain invalid(1);
    assert(!invalid.observe(
        bad, centered_left, centered_right, frames, ym2612_authored_route(true, true)));
    assert(invalid.failed());
    assert(!invalid.ready());
    invalid.reset();
    assert(!invalid.failed());
    assert(invalid.observe(
        hq, centered_left, centered_right, frames, ym2612_authored_route(true, true)));
    assert(invalid.ready());

    return 0;
}
