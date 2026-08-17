#include "components/vgm/enhancement/genesis_enhanced_recomposition.h"

#include <array>
#include <cassert>
#include <cmath>

int main() {
    using namespace gameaudio::vgm;

    constexpr std::size_t frames = 4;
    const float reference_l[frames] = {10.0f, 20.0f, 30.0f, 40.0f};
    const float reference_r[frames] = {-10.0f, -20.0f, -30.0f, -40.0f};

    float old_fm_l[frames] = {1.0f, 2.0f, 3.0f, 4.0f};
    float old_fm_r[frames] = {0.5f, 1.0f, 1.5f, 2.0f};
    float new_fm_l[frames] = {2.0f, 4.0f, 6.0f, 8.0f};
    float new_fm_r[frames] = {1.0f, 2.0f, 3.0f, 4.0f};

    float old_psg_l[frames] = {0.25f, 0.50f, 0.75f, 1.00f};
    float old_psg_r[frames] = {0.25f, 0.50f, 0.75f, 1.00f};
    float new_psg_l[frames] = {0.50f, 1.00f, 1.50f, 2.00f};
    float new_psg_r[frames] = {0.50f, 1.00f, 1.50f, 2.00f};

    std::array<genesis_source_replacement_view, genesis_recomposition_source_count> sources{};
    auto& fm = sources[static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1)];
    fm.reference = {old_fm_l, old_fm_r, true};
    fm.enhanced = {new_fm_l, new_fm_r, true};
    fm.replace = true;

    auto& psg = sources[static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0)];
    psg.reference = {old_psg_l, old_psg_r, true};
    psg.enhanced = {new_psg_l, new_psg_r, true};
    psg.replace = true;

    genesis_enhanced_recomposition_storage<frames> render;
    assert(render.build(reference_l, reference_r, frames, sources));
    assert(render.valid());
    assert(render.used_replacement());
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const float expected_l = reference_l[frame]
            + (new_fm_l[frame] - old_fm_l[frame])
            + (new_psg_l[frame] - old_psg_l[frame]);
        const float expected_r = reference_r[frame]
            + (new_fm_r[frame] - old_fm_r[frame])
            + (new_psg_r[frame] - old_psg_r[frame]);
        assert(std::abs(render.left()[frame] - expected_l) < 1.0e-6f);
        assert(std::abs(render.right()[frame] - expected_r) < 1.0e-6f);
    }

    // An empty replacement set is an exact protected-reference pass-through.
    std::array<genesis_source_replacement_view, genesis_recomposition_source_count> none{};
    assert(render.build(reference_l, reference_r, frames, none));
    assert(render.valid());
    assert(!render.used_replacement());
    for (std::size_t frame = 0; frame < frames; ++frame) {
        assert(render.left()[frame] == reference_l[frame]);
        assert(render.right()[frame] == reference_r[frame]);
    }

    // Enhanced == exact reference must be exact parity. This is the strongest
    // algebraic control for proving the recompositor itself is transparent.
    auto parity = none;
    auto& parity_fm = parity[static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm2)];
    parity_fm.reference = {old_fm_l, old_fm_r, true};
    parity_fm.enhanced = {old_fm_l, old_fm_r, true};
    parity_fm.replace = true;
    assert(render.build(reference_l, reference_r, frames, parity));
    for (std::size_t frame = 0; frame < frames; ++frame) {
        assert(render.left()[frame] == reference_l[frame]);
        assert(render.right()[frame] == reference_r[frame]);
    }

    // Never subtract a merely similar source. If exact reference contribution
    // evidence is missing, Enhanced fails closed back to protected reference.
    auto inexact = sources;
    inexact[static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1)].reference.exact = false;
    assert(!render.build(reference_l, reference_r, frames, inexact));
    assert(render.last_error() ==
        genesis_enhanced_recomposition_error::missing_exact_reference_source);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        assert(render.left()[frame] == reference_l[frame]);
        assert(render.right()[frame] == reference_r[frame]);
    }

    // A broken later replacement must undo any earlier successful delta rather
    // than leak a half-enhanced block into playback.
    auto transactional = sources;
    transactional[static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0)].enhanced.left = nullptr;
    assert(!render.build(reference_l, reference_r, frames, transactional));
    assert(render.last_error() ==
        genesis_enhanced_recomposition_error::missing_enhanced_source);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        assert(render.left()[frame] == reference_l[frame]);
        assert(render.right()[frame] == reference_r[frame]);
    }

    // Non-finite replacement data is evidence corruption, not a value to clip.
    auto nonfinite = sources;
    new_psg_l[2] = NAN;
    assert(!render.build(reference_l, reference_r, frames, nonfinite));
    assert(render.last_error() == genesis_enhanced_recomposition_error::nonfinite_sample);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        assert(render.left()[frame] == reference_l[frame]);
        assert(render.right()[frame] == reference_r[frame]);
    }

    return 0;
}
