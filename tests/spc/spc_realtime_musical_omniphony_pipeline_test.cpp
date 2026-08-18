#include "../../components/spc/spc_realtime_musical_omniphony_pipeline.h"
#include "../../components/spc/spc_source_bus.h"
#include "../../components/spc/spc_spatial_source.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

struct fake_renderer_state {
    vgmtooling::model::omniphony_source_mix_budget_v1_transport observed_budget{};
    float observed_first_dry_width = 0.0f;
    float observed_first_dry_diffuse = 0.0f;
    std::size_t observed_source_count = 0;
    std::size_t budget_calls = 0;
    std::size_t process_calls = 0;
    std::size_t reset_calls = 0;
};

std::uint32_t fake_abi_major() { return 0; }
std::uint32_t fake_abi_minor() { return 4; }

std::int32_t fake_reset(void* processor)
{
    auto* state = static_cast<fake_renderer_state*>(processor);
    assert(state != nullptr);
    ++state->reset_calls;
    state->observed_budget = {};
    return 0;
}

std::int32_t fake_set_mix_budget(
    void* processor,
    const vgmtooling::model::omniphony_source_mix_budget_v1_transport* budget)
{
    auto* state = static_cast<fake_renderer_state*>(processor);
    assert(state != nullptr);
    assert(budget != nullptr);
    ++state->budget_calls;
    state->observed_budget = *budget;
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
    assert(source_count == 4);
    assert(sources[0].lane_kind == vgmtooling::model::omniphony_source_lane_dry);
    assert(sources[1].lane_kind == vgmtooling::model::omniphony_source_lane_dry);
    assert(sources[2].lane_kind == vgmtooling::model::omniphony_source_lane_shared_wet);
    assert(sources[3].lane_kind == vgmtooling::model::omniphony_source_lane_shared_wet);

    ++state->process_calls;
    state->observed_source_count = source_count;
    state->observed_first_dry_width = sources[0].width;
    state->observed_first_dry_diffuse = sources[0].diffuse;

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
    using namespace gameaudio::spc;
    using namespace vgmtooling::model;

    constexpr double sample_rate = 48000.0;
    constexpr std::size_t frames = 4800;
    constexpr std::size_t source_count = 4;
    constexpr double pi = 3.141592653589793238462643383279502884;

    std::array<float, frames> dry_voice_0{};
    std::array<float, frames> dry_voice_1{};
    std::array<float, frames> echo_left{};
    std::array<float, frames> echo_right{};
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const double time = static_cast<double>(frame) / sample_rate;
        const float dry = 0.40f * static_cast<float>(std::sin(2.0 * pi * 1000.0 * time));
        dry_voice_0[frame] = dry;
        dry_voice_1[frame] = dry;
        echo_left[frame] = 0.25f * static_cast<float>(std::sin(2.0 * pi * 7000.0 * time));
        echo_right[frame] = 0.25f * static_cast<float>(std::sin(2.0 * pi * 100.0 * time));
    }

    spc_runtime_capture_record voice_0{};
    voice_0.fields = spc_runtime_capture_field::voice
        | spc_runtime_capture_field::route_gain_left
        | spc_runtime_capture_field::route_gain_right
        | spc_runtime_capture_field::echo_send_enabled;
    voice_0.voice = 0;
    voice_0.route_gain_left = 127;
    voice_0.route_gain_right = 127;
    voice_0.echo_send_enabled = true;

    auto voice_1 = voice_0;
    voice_1.voice = 1;

    auto voice_0_evidence = make_spc_spatial_source(voice_0, 1);
    auto voice_1_evidence = make_spc_spatial_source(voice_1, 1);
    // Pin explicit source-attached presentation so the test can distinguish the
    // neutral first block from the crowding-scaled next block without musical
    // role inference becoming another variable.
    voice_0_evidence.presentation.width = 1.0f;
    voice_0_evidence.presentation.diffuse = 1.0f;
    voice_0_evidence.presentation.confidence = 1.0f;
    voice_1_evidence.presentation = voice_0_evidence.presentation;

    const auto echo_left_evidence = spc_source_bus::make_post_evol_echo_source(
        spc_source_bus::echo_side::left,
        1);
    const auto echo_right_evidence = spc_source_bus::make_post_evol_echo_source(
        spc_source_bus::echo_side::right,
        1);

    std::array<spatial_audio_lane_view, source_count> lanes{};
    lanes[0].kind = spatial_audio_lane_kind::dry_source;
    lanes[0].mono_pcm = dry_voice_0.data();
    lanes[0].evidence = voice_0_evidence;
    lanes[1].kind = spatial_audio_lane_kind::dry_source;
    lanes[1].mono_pcm = dry_voice_1.data();
    lanes[1].evidence = voice_1_evidence;
    lanes[2].kind = spatial_audio_lane_kind::shared_effect_return;
    lanes[2].mono_pcm = echo_left.data();
    lanes[2].evidence = echo_left_evidence;
    lanes[3].kind = spatial_audio_lane_kind::shared_effect_return;
    lanes[3].mono_pcm = echo_right.data();
    lanes[3].evidence = echo_right_evidence;

    spatial_source_host_chunk chunk{};
    chunk.sources.lanes = lanes.data();
    chunk.sources.lane_count = lanes.size();
    chunk.sources.frame_count = frames;
    chunk.session_epoch = 1;
    chunk.reference_frame_start = 12000;

    spc_realtime_musical_omniphony_pipeline<10, 16, 16> pipeline{};
    fake_renderer_state renderer{};
    assert(pipeline.bind_renderer(
        static_cast<void*>(&renderer),
        fake_abi_major,
        fake_abi_minor,
        fake_reset,
        fake_set_mix_budget,
        fake_process));
    assert(pipeline.renderer_bound());

    std::array<float, frames * source_count> source_scratch{};
    std::array<float, frames * 2> stereo{};

    // No current-block lookahead: the first SPC block reaches Omniphony with a
    // neutral scene budget and the original source-attached extent evidence.
    const auto first = pipeline.process_chunk(
        chunk,
        sample_rate,
        source_scratch.data(),
        source_scratch.size(),
        stereo.data(),
        stereo.size(),
        96);
    assert(first.source_chunk_valid);
    assert(first.omniphony.prepared);
    assert(first.omniphony.budget_committed);
    assert(first.omniphony.rendered);
    assert(first.omniphony.learned);
    assert(renderer.observed_source_count == source_count);
    assert(renderer.budget_calls == 1);
    assert(renderer.process_calls == 1);
    assert(renderer.observed_budget.depth_scale == 1.0f);
    assert(renderer.observed_budget.height_scale == 1.0f);
    assert(renderer.observed_budget.shared_wet_strength_scale == 1.0f);
    assert(renderer.observed_budget.shared_wet_extent_scale == 1.0f);
    assert(renderer.observed_budget.externalization_scale == 1.0f);
    assert(near(renderer.observed_first_dry_width, 1.0f));
    assert(near(renderer.observed_first_dry_diffuse, 1.0f));

    // The two dry S-DSP voices are spectrally identical. The two linked shared
    // echo lanes deliberately occupy very different bands. They must contribute
    // to wet occupancy but must NOT enter the dry-source masking pair itself.
    const auto& scene = pipeline.pipeline().frontend().observer().scene();
    assert(scene.observed_lane_count == source_count);
    assert(scene.active_lane_count == source_count);
    assert(scene.shared_effect_energy_share > 0.10f);
    assert(scene.coarse_spectral_overlap > 0.99f);

    const auto learned_budget = pipeline.pipeline().frontend().mix_budget();
    assert(learned_budget.dry_width_scale < 1.0f);
    assert(learned_budget.dry_diffuse_scale < 1.0f);
    assert(learned_budget.shared_wet_strength < 1.0f);
    assert(learned_budget.shared_wet_extent < 1.0f);
    assert(learned_budget.added_externalization_scale < 1.0f);

    // The diagnostic snapshot is passive but complete enough for corpus-scale
    // validation: it records source spectra, scene geometry, the neutral budget
    // used for this block, the exact ABI record that reached Omniphony, and the
    // budget learned for the future. Shared echo remains visible as wet evidence.
    const auto& first_trace = pipeline.pipeline().last_governor_trace();
    assert(first_trace.valid);
    assert(first_trace.lane_count == source_count);
    assert(first_trace.frame_count == frames);
    assert(first_trace.sample_rate == sample_rate);
    assert(first_trace.absolute_sample_position == 12000);
    assert(first_trace.scene.observed_lane_count == source_count);
    assert(first_trace.scene.shared_effect_energy_share == scene.shared_effect_energy_share);
    assert(first_trace.scene.coarse_spectral_overlap == scene.coarse_spectral_overlap);
    assert(first_trace.sources[0].lane_kind == spatial_audio_lane_kind::dry_source);
    assert(first_trace.sources[1].lane_kind == spatial_audio_lane_kind::dry_source);
    assert(first_trace.sources[2].lane_kind == spatial_audio_lane_kind::shared_effect_return);
    assert(first_trace.sources[3].lane_kind == spatial_audio_lane_kind::shared_effect_return);
    for (std::size_t band = 0; band < 3; ++band) {
        assert(near(
            first_trace.sources[0].coarse_band_energy_share[band],
            first_trace.sources[1].coarse_band_energy_share[band],
            1.0e-5f));
    }
    assert(first_trace.applied_budget.depth_scale == 1.0f);
    assert(first_trace.applied_budget.height_scale == 1.0f);
    assert(first_trace.applied_budget.shared_wet_strength == 1.0f);
    assert(first_trace.applied_budget.shared_wet_extent == 1.0f);
    assert(first_trace.applied_budget.added_externalization_scale == 1.0f);
    assert(first_trace.renderer_budget.depth_scale == renderer.observed_budget.depth_scale);
    assert(first_trace.renderer_budget.height_scale == renderer.observed_budget.height_scale);
    assert(first_trace.renderer_budget.shared_wet_strength_scale
        == renderer.observed_budget.shared_wet_strength_scale);
    assert(first_trace.renderer_budget.shared_wet_extent_scale
        == renderer.observed_budget.shared_wet_extent_scale);
    assert(first_trace.renderer_budget.externalization_scale
        == renderer.observed_budget.externalization_scale);
    assert(near(first_trace.learned_budget.dry_width_scale, learned_budget.dry_width_scale));
    assert(near(first_trace.learned_budget.dry_diffuse_scale, learned_budget.dry_diffuse_scale));
    assert(near(first_trace.learned_budget.shared_wet_strength, learned_budget.shared_wet_strength));
    assert(near(first_trace.learned_budget.shared_wet_extent, learned_budget.shared_wet_extent));
    assert(near(
        first_trace.learned_budget.added_externalization_scale,
        learned_budget.added_externalization_scale));

    // Only the NEXT chunk may use those completed-scene observations. This also
    // closes both presentation planes: dry width/diffuse travel in source
    // evidence, while wet/externalization travel through ABI 0.4 scene control.
    chunk.reference_frame_start += frames;
    const auto second = pipeline.process_chunk(
        chunk,
        sample_rate,
        source_scratch.data(),
        source_scratch.size(),
        stereo.data(),
        stereo.size(),
        96);
    assert(second.omniphony.budget_committed);
    assert(second.omniphony.rendered);
    assert(second.omniphony.learned);
    assert(renderer.budget_calls == 2);
    assert(renderer.process_calls == 2);
    assert(near(renderer.observed_first_dry_width, learned_budget.dry_width_scale));
    assert(near(renderer.observed_first_dry_diffuse, learned_budget.dry_diffuse_scale));
    assert(near(
        renderer.observed_budget.depth_scale,
        learned_budget.depth_scale));
    assert(near(
        renderer.observed_budget.height_scale,
        learned_budget.height_scale));
    assert(near(
        renderer.observed_budget.shared_wet_strength_scale,
        learned_budget.shared_wet_strength));
    assert(near(
        renderer.observed_budget.shared_wet_extent_scale,
        learned_budget.shared_wet_extent));
    assert(near(
        renderer.observed_budget.externalization_scale,
        learned_budget.added_externalization_scale));

    const auto& second_trace = pipeline.pipeline().last_governor_trace();
    assert(second_trace.valid);
    assert(second_trace.absolute_sample_position == 12000 + frames);
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

    return 0;
}
