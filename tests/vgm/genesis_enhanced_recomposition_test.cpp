#include "components/vgm/enhancement/genesis_enhanced_recomposition.h"
#include "components/vgm/enhancement/genesis_realtime_musical_omniphony_pipeline.h"
#include "components/vgm/enhancement/genesis_spatial_source.h"
#include "components/vgm/enhancement/genesis_spatial_source_bus.h"
#include "model/spatial_playback_options.h"

#include <array>
#include <cassert>
#include <cmath>

int main() {
    using namespace gameaudio::vgm;
    using namespace vgmtooling::model;

    constexpr std::size_t frames = 4;
    const float reference_l[frames] = {10.0f, 20.0f, 30.0f, 40.0f};
    const float reference_r[frames] = {-10.0f, -20.0f, -30.0f, -40.0f};

    float old_fm_l[frames] = {1.0f, 2.0f, 3.0f, 4.0f};
    float old_fm_r[frames] = {0.5f, 1.0f, 1.5f, 2.0f};
    float new_fm_l[frames] = {2.0f, 4.0f, 6.0f, 8.0f};
    float new_fm_r[frames] = {1.0f, 2.0f, 3.0f, 4.0f};

    float old_dac_l[frames] = {0.75f, 1.00f, 1.25f, 1.50f};
    float old_dac_r[frames] = {0.50f, 0.75f, 1.00f, 1.25f};
    float new_dac_l[frames] = {1.00f, 1.50f, 2.00f, 2.50f};
    float new_dac_r[frames] = {0.75f, 1.25f, 1.75f, 2.25f};

    float old_psg_l[frames] = {0.25f, 0.50f, 0.75f, 1.00f};
    float old_psg_r[frames] = {0.25f, 0.50f, 0.75f, 1.00f};
    float new_psg_l[frames] = {0.50f, 1.00f, 1.50f, 2.00f};
    float new_psg_r[frames] = {0.50f, 1.00f, 1.50f, 2.00f};

    std::array<genesis_source_replacement_view, genesis_recomposition_source_count> sources{};
    auto& fm = sources[static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1)];
    fm.reference = {old_fm_l, old_fm_r, true};
    fm.enhanced = {new_fm_l, new_fm_r, true};
    fm.replace = true;

    auto& dac = sources[static_cast<std::size_t>(genesis_recomposition_source::ym2612_dac)];
    dac.reference = {old_dac_l, old_dac_r, true};
    dac.enhanced = {new_dac_l, new_dac_r, true};
    dac.replace = true;

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
            + (new_dac_l[frame] - old_dac_l[frame])
            + (new_psg_l[frame] - old_psg_l[frame]);
        const float expected_r = reference_r[frame]
            + (new_fm_r[frame] - old_fm_r[frame])
            + (new_dac_r[frame] - old_dac_r[frame])
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

    // Higher-quality == exact reference must be exact parity. This is the
    // strongest algebraic control for proving the recompositor is transparent.
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

    // The spatial bus consumes the already-selected isolated source lanes. It
    // cannot choose source quality itself, so surround changes presentation only.
    std::array<genesis_stereo_source_view, genesis_recomposition_source_count> reference_sources{};
    std::array<genesis_stereo_source_view, genesis_recomposition_source_count> higher_quality_sources{};
    std::array<spatial_source_evidence, genesis_recomposition_source_count> source_evidence{};

    constexpr std::size_t fm1_index =
        static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1);
    constexpr std::size_t psg0_index =
        static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0);
    reference_sources[fm1_index] = {old_fm_l, old_fm_r, true};
    reference_sources[psg0_index] = {old_psg_l, old_psg_r, true};
    higher_quality_sources[fm1_index] = {new_fm_l, new_fm_r, true};
    higher_quality_sources[psg0_index] = {new_psg_l, new_psg_r, true};
    source_evidence[fm1_index] = make_genesis_spatial_source(
        genesis_spatial_device::ym2612_fm, 0, 0, 1, ym2612_authored_route(true, true));
    source_evidence[psg0_index] = make_genesis_spatial_source(
        genesis_spatial_device::sn76489_tone, 0, 0, 1, sn76489_authored_route(0xFF, 0));

    const auto check_spatial_combination = [&](bool quality, bool surround) {
        spatial_playback_options options;
        options.enhanced = quality;
        options.surround = surround;

        const auto& selected = uses_enhanced_renderer(options)
            ? higher_quality_sources
            : reference_sources;
        genesis_spatial_source_bus_storage<frames> bus;
        assert(bus.build(selected, source_evidence, frames));
        assert(bus.valid());
        assert(bus.lane_count() == 2);
        assert(bus.canonical_source_index(0) == fm1_index);
        assert(bus.canonical_source_index(1) == psg0_index);

        const auto& block = bus.block();
        assert(block.lane_count == 2);
        assert(block.frame_count == frames);
        assert(block.lanes[0].evidence.stereo_route.gain_preapplied);
        assert(block.lanes[1].evidence.stereo_route.gain_preapplied);

        const float expected_fm0 = quality
            ? static_cast<float>(std::sqrt((4.0 + 1.0) * 0.5))
            : static_cast<float>(std::sqrt((1.0 + 0.25) * 0.5));
        const float expected_psg0 = quality ? 0.50f : 0.25f;
        assert(std::abs(block.lanes[0].mono_pcm[0] - expected_fm0) < 1.0e-6f);
        assert(std::abs(block.lanes[1].mono_pcm[0] - expected_psg0) < 1.0e-6f);

        if (surround)
            assert(resolve_spatial_playback(options) == spatial_playback_path::source_full_sphere);
        else
            assert(resolve_spatial_playback(options) == spatial_playback_path::reference_stereo);
    };

    check_spatial_combination(false, false);
    check_spatial_combination(true, false);
    check_spatial_combination(false, true);
    check_spatial_combination(true, true);

    // Compile and exercise the outer Genesis -> Omniphony seam without a
    // renderer. It accepts the selected source set and validates it, but cannot
    // render until the independent Spatial host binds Omniphony.
    genesis_realtime_musical_omniphony_pipeline<frames> omniphony;
    std::array<float, genesis_recomposition_source_count * frames> source_scratch{};
    std::array<float, frames * 2> spatial_stereo{};
    const auto unbound_spatial = omniphony.process_selected_sources(
        reference_sources,
        source_evidence,
        frames,
        48000.0,
        source_scratch.data(),
        source_scratch.size(),
        spatial_stereo.data(),
        spatial_stereo.size(),
        1000,
        96);
    assert(unbound_spatial.source_block_valid);
    assert(!unbound_spatial.omniphony.prepared);
    assert(!unbound_spatial.omniphony.rendered);

    // Never subtract a merely similar source. If exact reference contribution
    // evidence is missing, the strict quality path fails closed to reference.
    auto inexact = sources;
    inexact[static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1)].reference.exact = false;
    assert(!render.build(reference_l, reference_r, frames, inexact));
    assert(render.last_error() ==
        genesis_enhanced_recomposition_error::missing_exact_reference_source);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        assert(render.left()[frame] == reference_l[frame]);
        assert(render.right()[frame] == reference_r[frame]);
    }

    // The strict control remains all-or-nothing.
    auto transactional = sources;
    transactional[static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0)].enhanced.left = nullptr;
    assert(!render.build(reference_l, reference_r, frames, transactional));
    assert(render.last_error() ==
        genesis_enhanced_recomposition_error::missing_enhanced_source);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        assert(render.left()[frame] == reference_l[frame]);
        assert(render.right()[frame] == reference_r[frame]);
    }

    // Product playback is family-local: a broken PSG candidate keeps PSG on
    // protected reference while independently proven FM and DAC remain applied.
    auto psg_failed = sources;
    psg_failed[static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0)]
        .enhanced.left = nullptr;
    assert(render.build_independent_families(reference_l, reference_r, frames, psg_failed));
    assert(render.valid());
    assert(render.used_replacement());
    assert(render.had_family_failure());
    assert(render.last_error() == genesis_enhanced_recomposition_error::none);

    const auto fm_status = render.family_status(genesis_recomposition_family::ym2612_fm);
    const auto dac_status = render.family_status(genesis_recomposition_family::ym2612_dac);
    const auto psg_status = render.family_status(genesis_recomposition_family::sn76489_psg);
    assert(fm_status.requested && fm_status.applied);
    assert(dac_status.requested && dac_status.applied);
    assert(psg_status.requested && !psg_status.applied);
    assert(psg_status.error == genesis_enhanced_recomposition_error::missing_enhanced_source);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const float expected_l = reference_l[frame]
            + (new_fm_l[frame] - old_fm_l[frame])
            + (new_dac_l[frame] - old_dac_l[frame]);
        const float expected_r = reference_r[frame]
            + (new_fm_r[frame] - old_fm_r[frame])
            + (new_dac_r[frame] - old_dac_r[frame]);
        assert(std::abs(render.left()[frame] - expected_l) < 1.0e-6f);
        assert(std::abs(render.right()[frame] - expected_r) < 1.0e-6f);
    }

    // DAC failure is likewise isolated. FM and PSG still compose on the same
    // protected block, proving all three Genesis source families are independent.
    auto dac_failed = sources;
    dac_failed[static_cast<std::size_t>(genesis_recomposition_source::ym2612_dac)]
        .reference.exact = false;
    assert(render.build_independent_families(reference_l, reference_r, frames, dac_failed));
    assert(render.had_family_failure());
    assert(render.family_status(genesis_recomposition_family::ym2612_fm).applied);
    assert(!render.family_status(genesis_recomposition_family::ym2612_dac).applied);
    assert(render.family_status(genesis_recomposition_family::sn76489_psg).applied);
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
