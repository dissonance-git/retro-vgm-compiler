#include "components/vgm/enhancement/ym2151_enhanced_recomposition.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>

int main() {
    using namespace gameaudio::vgm;

    constexpr std::size_t frames = 4;
    const std::array<float, frames> reference_left{20.0f, 30.0f, 40.0f, 50.0f};
    const std::array<float, frames> reference_right{-20.0f, -30.0f, -40.0f, -50.0f};

    std::array<std::array<float, frames>, ym2151_recomposition_source_count> old_left{};
    std::array<std::array<float, frames>, ym2151_recomposition_source_count> old_right{};
    std::array<std::array<float, frames>, ym2151_recomposition_source_count> new_left{};
    std::array<std::array<float, frames>, ym2151_recomposition_source_count> new_right{};

    for (std::size_t source = 0; source < ym2151_recomposition_source_count; ++source) {
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const float base = static_cast<float>((source + 1) * (frame + 1));
            old_left[source][frame] = base * 0.1f;
            old_right[source][frame] = -base * 0.05f;
            new_left[source][frame] = old_left[source][frame] * 1.5f;
            new_right[source][frame] = old_right[source][frame] * 1.5f;
        }
    }

    std::array<ym2151_source_replacement_view, ym2151_recomposition_source_count> sources{};
    for (std::size_t source = 0; source < ym2151_recomposition_source_count; ++source) {
        sources[source].reference = {
            old_left[source].data(), old_right[source].data(), true};
        sources[source].enhanced = {
            new_left[source].data(), new_right[source].data(), true};
        sources[source].replace = true;
    }

    ym2151_enhanced_recomposition_storage<frames> render;
    assert(render.build_independent_families(
        reference_left.data(), reference_right.data(), frames, sources));
    assert(render.valid());
    assert(render.used_replacement());
    assert(!render.had_family_failure());
    const auto fm_status = render.family_status(ym2151_recomposition_family::fm);
    assert(fm_status.requested && fm_status.applied);

    for (std::size_t frame = 0; frame < frames; ++frame) {
        float expected_left = reference_left[frame];
        float expected_right = reference_right[frame];
        for (std::size_t source = 0; source < ym2151_recomposition_source_count; ++source) {
            expected_left += new_left[source][frame] - old_left[source][frame];
            expected_right += new_right[source][frame] - old_right[source][frame];
        }
        assert(std::fabs(render.left()[frame] - expected_left) < 1.0e-5f);
        assert(std::fabs(render.right()[frame] - expected_right) < 1.0e-5f);
    }

    // Exact enhanced lanes equal to exact reference lanes are algebraic parity.
    auto parity = sources;
    for (std::size_t source = 0; source < ym2151_recomposition_source_count; ++source)
        parity[source].enhanced = parity[source].reference;
    assert(render.build_independent_families(
        reference_left.data(), reference_right.data(), frames, parity));
    for (std::size_t frame = 0; frame < frames; ++frame) {
        assert(render.left()[frame] == reference_left[frame]);
        assert(render.right()[frame] == reference_right[frame]);
    }

    // One invalid OPM channel invalidates the enhanced FM family but not the
    // protected block. This is product-facing fail-close, not partial guessing.
    auto failed_family = sources;
    failed_family[static_cast<std::size_t>(ym2151_recomposition_source::fm5)]
        .enhanced.exact = false;
    assert(render.build_independent_families(
        reference_left.data(), reference_right.data(), frames, failed_family));
    assert(render.valid());
    assert(!render.used_replacement());
    assert(render.had_family_failure());
    const auto failed_status = render.family_status(ym2151_recomposition_family::fm);
    assert(failed_status.requested && !failed_status.applied);
    assert(failed_status.error ==
        ym2151_enhanced_recomposition_error::missing_enhanced_source);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        assert(render.left()[frame] == reference_left[frame]);
        assert(render.right()[frame] == reference_right[frame]);
    }

    // Strict scientific/control mode rejects the same block globally.
    assert(!render.build(
        reference_left.data(), reference_right.data(), frames, failed_family));
    assert(!render.valid());
    assert(render.last_error() ==
        ym2151_enhanced_recomposition_error::missing_enhanced_source);

    // Reference corruption is globally invalid and cannot be hidden by fallback.
    auto corrupt_reference = reference_left;
    corrupt_reference[2] = std::numeric_limits<float>::quiet_NaN();
    assert(!render.build_independent_families(
        corrupt_reference.data(), reference_right.data(), frames, sources));
    assert(render.last_error() == ym2151_enhanced_recomposition_error::nonfinite_sample);

    // Unrequested enhancement is exact protected-reference playback.
    std::array<ym2151_source_replacement_view, ym2151_recomposition_source_count> none{};
    assert(render.build_independent_families(
        reference_left.data(), reference_right.data(), frames, none));
    assert(render.valid());
    assert(!render.used_replacement());
    assert(!render.family_status(ym2151_recomposition_family::fm).requested);

    return 0;
}
