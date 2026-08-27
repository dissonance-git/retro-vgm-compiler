#include "components/vgm/enhancement/genesis_enhanced_recomposition.h"
#include "components/vgm/enhancement/genesis_spatial_source.h"
#include "components/vgm/enhancement/vgm_realtime_musical_omniphony_pipeline.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

struct fake_renderer_state {
    vgmtooling::model::omniphony_source_mix_budget_v1_transport observed_budget{};
    std::size_t source_count = 0;
    std::size_t budget_calls = 0;
    std::size_t process_calls = 0;
};

std::uint32_t fake_abi_major() { return 0; }
std::uint32_t fake_abi_minor() { return 4; }

std::int32_t fake_reset(void*)
{
    return 0;
}

std::int32_t fake_set_mix_budget(
    void* processor,
    const vgmtooling::model::omniphony_source_mix_budget_v1_transport* budget)
{
    auto* state = static_cast<fake_renderer_state*>(processor);
    assert(state != nullptr);
    assert(budget != nullptr);
    state->observed_budget = *budget;
    ++state->budget_calls;
    return 0;
}

std::int32_t fake_process(
    void* processor,
    const float* input,
    const vgmtooling::model::omniphony_source_evidence_v1_transport* sources,
    std::size_t source_count,
    const vgmtooling::model::omniphony_source_evidence_event_v1_transport*,
    std::size_t,
    std::size_t frame_count,
    std::uint64_t,
    std::uint32_t,
    float* output)
{
    auto* state = static_cast<fake_renderer_state*>(processor);
    assert(state != nullptr);
    assert(input != nullptr);
    assert(sources != nullptr);
    assert(output != nullptr);
    assert(source_count == 2);
    assert(sources[0].lane_kind == vgmtooling::model::omniphony_source_lane_dry);
    assert(sources[1].lane_kind == vgmtooling::model::omniphony_source_lane_dry);

    state->source_count = source_count;
    ++state->process_calls;
    for (std::size_t frame = 0; frame < frame_count; ++frame) {
        const float sample = input[frame * source_count];
        output[frame * 2] = sample;
        output[frame * 2 + 1] = sample;
    }
    return 0;
}

bool near(float left, float right, float tolerance = 1.0e-6f)
{
    return std::fabs(left - right) <= tolerance;
}

} // namespace

int main()
{
    using namespace gameaudio::vgm;
    using namespace vgmtooling::model;

    constexpr double sample_rate = 48000.0;
    constexpr std::size_t frames = 4800;
    constexpr double pi = 3.141592653589793238462643383279502884;

    std::array<float, frames> shared_wave{};
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const double time = static_cast<double>(frame) / sample_rate;
        shared_wave[frame] = 0.40f * static_cast<float>(
            std::sin(2.0 * pi * 1000.0 * time));
    }

    using pipeline_type = vgm_realtime_musical_omniphony_pipeline<genesis_recomposition_source_count, frames, 16, 16>;
    pipeline_type::source_array sources{};
    pipeline_type::evidence_array evidence{};

    constexpr std::size_t fm1_index =
        static_cast<std::size_t>(genesis_recomposition_source::ym2612_fm1);
    constexpr std::size_t psg0_index =
        static_cast<std::size_t>(genesis_recomposition_source::sn76489_tone0);

    sources[fm1_index] = {shared_wave.data(), shared_wave.data(), true};
    sources[psg0_index] = {shared_wave.data(), shared_wave.data(), true};
    evidence[fm1_index] = make_genesis_spatial_source(
        genesis_spatial_device::ym2612_fm,
        0,
        0,
        1,
        ym2612_authored_route(true, true));
    evidence[psg0_index] = make_genesis_spatial_source(
        genesis_spatial_device::sn76489_tone,
        0,
        0,
        1,
        sn76489_authored_route(0xFF, 0));
    evidence[fm1_index].presentation.width = 1.0f;
    evidence[fm1_index].presentation.diffuse = 1.0f;
    evidence[fm1_index].presentation.confidence = 1.0f;
    evidence[psg0_index].presentation = evidence[fm1_index].presentation;

    pipeline_type pipeline{};
    fake_renderer_state renderer{};
    assert(pipeline.bind_renderer(
        static_cast<void*>(&renderer),
        fake_abi_major,
        fake_abi_minor,
        fake_reset,
        fake_set_mix_budget,
        fake_process));

    std::array<float, frames * genesis_recomposition_source_count> source_scratch{};
    std::array<float, frames * 2> stereo{};

    const auto first = pipeline.process_selected_sources(
        sources,
        evidence,
        frames,
        sample_rate,
        source_scratch.data(),
        source_scratch.size(),
        stereo.data(),
        stereo.size(),
        32000,
        96);
    assert(first.source_block_valid);
    assert(first.omniphony.prepared);
    assert(first.omniphony.budget_committed);
    assert(first.omniphony.rendered);
    assert(first.omniphony.learned);
    assert(renderer.source_count == 2);
    assert(renderer.budget_calls == 1);
    assert(renderer.process_calls == 1);

    const auto& first_trace = pipeline.pipeline().last_governor_trace();
    assert(first_trace.valid);
    assert(first_trace.lane_count == 2);
    assert(first_trace.frame_count == frames);
    assert(first_trace.sample_rate == sample_rate);
    assert(first_trace.absolute_sample_position == 32000);
    assert(first_trace.scene.observed_lane_count == 2);
    assert(first_trace.scene.active_lane_count == 2);
    assert(first_trace.scene.shared_effect_energy_share == 0.0f);
    assert(first_trace.scene.coarse_spectral_overlap > 0.99f);
    assert(first_trace.sources[0].lane_kind == spatial_audio_lane_kind::dry_source);
    assert(first_trace.sources[1].lane_kind == spatial_audio_lane_kind::dry_source);
    for (std::size_t band = 0; band < 3; ++band) {
        assert(near(
            first_trace.sources[0].coarse_band_energy_share[band],
            first_trace.sources[1].coarse_band_energy_share[band],
            1.0e-5f));
    }

    // With exactly two active dry Genesis lanes, the pairwise field has one and
    // only one admissible pair. Its overlap must therefore reproduce the scene
    // scalar while retaining the underlying FM/PSG source identities.
    realtime_spatial_overlap_pair_observation pair{};
    assert(first_trace.pair(0, 1, pair));
    assert(pair.valid);
    assert(pair.left_lane_index == 0);
    assert(pair.right_lane_index == 1);
    assert(pair.left_source_id == first_trace.sources[0].source_id);
    assert(pair.left_generation == first_trace.sources[0].generation);
    assert(pair.right_source_id == first_trace.sources[1].source_id);
    assert(pair.right_generation == first_trace.sources[1].generation);
    assert(pair.coarse_spectral_overlap > 0.99f);
    assert(pair.pair_energy_weight > 0.0f);
    assert(near(
        first_trace.reconstructed_coarse_spectral_overlap(),
        first_trace.scene.coarse_spectral_overlap,
        1.0e-5f));

    // Pair order is presentation-independent. Asking for the reverse lane order
    // returns the same canonical pair instead of inventing a directed relation.
    realtime_spatial_overlap_pair_observation reverse_pair{};
    assert(first_trace.pair(1, 0, reverse_pair));
    assert(reverse_pair.left_lane_index == pair.left_lane_index);
    assert(reverse_pair.right_lane_index == pair.right_lane_index);
    assert(near(reverse_pair.coarse_spectral_overlap, pair.coarse_spectral_overlap));
    assert(near(reverse_pair.pair_energy_weight, pair.pair_energy_weight));

    assert(first_trace.applied_budget.dry_width_scale == 1.0f);
    assert(first_trace.applied_budget.dry_diffuse_scale == 1.0f);
    assert(first_trace.applied_budget.depth_scale == 1.0f);
    assert(first_trace.applied_budget.height_scale == 1.0f);
    assert(first_trace.renderer_budget.depth_scale == 1.0f);
    assert(first_trace.renderer_budget.height_scale == 1.0f);
    assert(first_trace.renderer_budget.shared_wet_strength_scale == 1.0f);
    assert(first_trace.renderer_budget.shared_wet_extent_scale == 1.0f);
    assert(first_trace.renderer_budget.externalization_scale == 1.0f);

    const auto learned_budget = first_trace.learned_budget;
    assert(learned_budget.dry_width_scale < 1.0f);
    assert(learned_budget.dry_diffuse_scale < 1.0f);
    assert(learned_budget.added_externalization_scale < 1.0f);

    const auto second = pipeline.process_selected_sources(
        sources,
        evidence,
        frames,
        sample_rate,
        source_scratch.data(),
        source_scratch.size(),
        stereo.data(),
        stereo.size(),
        32000 + frames,
        96);
    assert(second.source_block_valid);
    assert(second.omniphony.budget_committed);
    assert(second.omniphony.rendered);
    assert(second.omniphony.learned);
    assert(renderer.budget_calls == 2);
    assert(renderer.process_calls == 2);

    const auto& second_trace = pipeline.pipeline().last_governor_trace();
    assert(second_trace.valid);
    assert(second_trace.absolute_sample_position == 32000 + frames);
    assert(near(second_trace.applied_budget.dry_width_scale, learned_budget.dry_width_scale));
    assert(near(second_trace.applied_budget.dry_diffuse_scale, learned_budget.dry_diffuse_scale));
    assert(near(second_trace.renderer_budget.depth_scale, learned_budget.depth_scale));
    assert(near(second_trace.renderer_budget.height_scale, learned_budget.height_scale));
    assert(near(
        second_trace.renderer_budget.shared_wet_strength_scale,
        learned_budget.shared_wet_strength));
    assert(near(
        second_trace.renderer_budget.shared_wet_extent_scale,
        learned_budget.shared_wet_extent));
    assert(near(
        second_trace.renderer_budget.externalization_scale,
        learned_budget.added_externalization_scale));
    assert(near(renderer.observed_budget.depth_scale, second_trace.renderer_budget.depth_scale));
    assert(near(renderer.observed_budget.height_scale, second_trace.renderer_budget.height_scale));
    assert(near(
        renderer.observed_budget.externalization_scale,
        second_trace.renderer_budget.externalization_scale));

    return 0;
}
