#include "../../model/realtime_musical_omniphony_pipeline.h"
#include "../../model/realtime_spatial_governor_trace_validation.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

struct fake_renderer_state {
    bool fail = false;
    float observed_foundation = -1.0f;
    vgmtooling::model::omniphony_source_mix_budget_v1_transport observed_budget{};
    std::size_t budget_calls = 0;
    std::size_t calls = 0;
    std::size_t resets = 0;
};

std::uint32_t fake_major() { return 0; }
std::uint32_t fake_minor() { return 4; }

std::int32_t fake_reset(void* processor)
{
    auto* state = static_cast<fake_renderer_state*>(processor);
    assert(state != nullptr);
    ++state->resets;
    state->observed_foundation = -1.0f;
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
    std::size_t frames,
    std::uint64_t,
    std::uint32_t,
    float* output)
{
    auto* state = static_cast<fake_renderer_state*>(processor);
    assert(state != nullptr);
    ++state->calls;
    assert(source_count == 1);
    state->observed_foundation = sources[0].foundation;
    if (state->fail)
        return -9;

    for (std::size_t frame = 0; frame < frames; ++frame) {
        output[frame * 2] = input[frame];
        output[frame * 2 + 1] = input[frame];
    }
    return 0;
}

} // namespace

int main()
{
    using namespace vgmtooling::model;

    constexpr double sample_rate = 48000.0;
    constexpr std::size_t frames = 4800;
    constexpr double pi = 3.141592653589793238462643383279502884;

    std::array<float, frames> source_pcm{};
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const double time = static_cast<double>(frame) / sample_rate;
        source_pcm[frame] = 0.5f * static_cast<float>(
            std::sin(2.0 * pi * 100.0 * time));
    }

    spatial_audio_lane_view lane{};
    lane.mono_pcm = source_pcm.data();
    lane.evidence.source_id = 42;
    lane.evidence.generation = 1;
    const spatial_source_block_view block{&lane, 1, frames};

    realtime_musical_omniphony_pipeline<4, 8, 8> pipeline{};
    fake_renderer_state renderer{};
    assert(pipeline.bind_renderer(
        static_cast<void*>(&renderer),
        fake_major,
        fake_minor,
        fake_reset,
        fake_set_mix_budget,
        fake_process));

    std::array<float, frames> source_scratch{};
    std::array<float, frames * 2> stereo{};

    // A renderer failure must not silently advance musical or scene memory. A
    // caller can retry or fall back with the same causal history still intact.
    renderer.fail = true;
    const auto failed = pipeline.process_block(
        block,
        sample_rate,
        source_scratch.data(),
        source_scratch.size(),
        stereo.data(),
        stereo.size(),
        0,
        96);
    assert(failed.prepared);
    assert(failed.transport_valid);
    assert(!failed.rendered);
    assert(!failed.learned);
    assert(failed.renderer_status == -9);
    assert(failed.governor_trace_index == 0);
    assert(pipeline.frontend().tracker().stream_seconds() == 0.0);
    assert(renderer.observed_foundation == 0.0f);
    assert(renderer.observed_budget.depth_scale == 1.0f);
    assert(renderer.observed_budget.height_scale == 1.0f);
    assert(renderer.observed_budget.externalization_scale == 1.0f);

    // First successful render still has no current-block lookahead. Only after
    // it reaches the renderer may raw PCM update future musical and mix memory.
    renderer.fail = false;
    const auto first = pipeline.process_block(
        block,
        sample_rate,
        source_scratch.data(),
        source_scratch.size(),
        stereo.data(),
        stereo.size(),
        0,
        96);
    assert(first.prepared);
    assert(first.budget_committed);
    assert(first.rendered);
    assert(first.learned);
    assert(first.renderer_status == 0);
    assert(first.governor_trace_index == 1);
    assert(renderer.observed_foundation == 0.0f);
    assert(renderer.observed_budget.depth_scale == 1.0f);
    assert(std::fabs(pipeline.frontend().tracker().stream_seconds() - 0.10) < 1.0e-9);

    const auto first_trace = pipeline.last_governor_trace();
    assert(first_trace.sequence_index == first.governor_trace_index);
    const auto first_trace_validation =
        validate_realtime_spatial_governor_trace(first_trace);
    assert(first_trace_validation.valid);
    assert(first_trace_validation.error == realtime_spatial_governor_trace_error::none);
    assert(first_trace_validation.observed_lane_count == 1);
    assert(first_trace_validation.active_dry_pair_count == 0);
    assert(first_trace_validation.reconstructed_coarse_spectral_overlap == 0.0f);

    // The next block can use the completed first block as past-only musical and
    // scene evidence. This is the causal adaptive DSP path, not a soundtrack
    // prepass or a genre preset.
    const auto learned_budget = pipeline.frontend().mix_budget();
    assert(learned_budget.depth_scale < 1.0f);
    assert(learned_budget.height_scale < 1.0f);
    const auto second = pipeline.process_block(
        block,
        sample_rate,
        source_scratch.data(),
        source_scratch.size(),
        stereo.data(),
        stereo.size(),
        frames,
        96);
    assert(second.rendered);
    assert(second.learned);
    assert(second.governor_trace_index == 2);
    assert(renderer.observed_foundation > 0.0f);
    assert(renderer.observed_budget.depth_scale == learned_budget.depth_scale);
    assert(renderer.observed_budget.height_scale == learned_budget.height_scale);
    assert(renderer.observed_budget.externalization_scale
        == learned_budget.added_externalization_scale);
    assert(std::fabs(pipeline.frontend().tracker().stream_seconds() - 0.20) < 1.0e-9);
    assert(renderer.calls == 3);
    assert(renderer.budget_calls == 3);

    const auto second_trace = pipeline.last_governor_trace();
    assert(second_trace.sequence_index == second.governor_trace_index);
    const auto continuity = validate_realtime_spatial_governor_trace_continuity(
        first_trace,
        second_trace);
    assert(continuity.valid);
    assert(continuity.error == realtime_spatial_governor_trace_error::none);

    auto bad_sequence_budget = second_trace;
    bad_sequence_budget.applied_budget.depth_scale = 1.0f;
    bad_sequence_budget.renderer_budget =
        make_omniphony_source_mix_budget(bad_sequence_budget.applied_budget);
    const auto bad_sequence_budget_continuity =
        validate_realtime_spatial_governor_trace_continuity(
            first_trace,
            bad_sequence_budget);
    assert(!bad_sequence_budget_continuity.valid);
    assert(bad_sequence_budget_continuity.error
        == realtime_spatial_governor_trace_error::sequence_budget_mismatch);

    // A seek/track reset clears both musical-memory and renderer budget state.
    // The first block after reset must again be history-free and neutral, and a
    // fresh trace timeline starts at transaction 1 rather than pretending to be
    // contiguous with the track that ended above.
    assert(pipeline.reset());
    assert(renderer.resets == 1);
    assert(pipeline.frontend().tracker().stream_seconds() == 0.0);
    assert(pipeline.frontend().mix_budget().depth_scale == 1.0f);
    const auto after_reset = pipeline.process_block(
        block,
        sample_rate,
        source_scratch.data(),
        source_scratch.size(),
        stereo.data(),
        stereo.size(),
        0,
        96);
    assert(after_reset.rendered);
    assert(after_reset.learned);
    assert(after_reset.governor_trace_index == 1);
    assert(pipeline.last_governor_trace().sequence_index == 1);
    assert(renderer.observed_foundation == 0.0f);
    assert(renderer.observed_budget.depth_scale == 1.0f);

    // Build a source-family-blind four-lane trace with three dry sources and one
    // shared wet field. Dry 0/1 occupy nearly the same broad spectrum while dry
    // 2 lives elsewhere. This is the geometry the real-corpus observatory must
    // be able to admit before any pair-aware renderer control is allowed.
    realtime_spatial_governor_trace<4> synthetic{};
    synthetic.valid = true;
    synthetic.sequence_index = 1;
    synthetic.lane_count = 4;
    synthetic.frame_count = frames;
    synthetic.sample_rate = sample_rate;
    synthetic.absolute_sample_position = 64000;
    synthetic.active_threshold = 0.15f;
    synthetic.applied_budget = {};
    synthetic.renderer_budget = make_omniphony_source_mix_budget(synthetic.applied_budget);
    synthetic.learned_budget = {};
    synthetic.scene.audio_observed = true;
    synthetic.scene.observed_lane_count = 4;
    synthetic.scene.active_lane_count = 4;
    synthetic.scene.energy_concentration = 0.26f;
    synthetic.scene.shared_effect_energy_share = 0.20f;

    synthetic.sources[0].audio_observed = true;
    synthetic.sources[0].source_id = 100;
    synthetic.sources[0].generation = 1;
    synthetic.sources[0].lane_kind = spatial_audio_lane_kind::dry_source;
    synthetic.sources[0].activity = 0.8f;
    synthetic.sources[0].relative_energy = 0.30f;
    synthetic.sources[0].coarse_band_energy_share = {0.80f, 0.15f, 0.05f};

    synthetic.sources[1] = synthetic.sources[0];
    synthetic.sources[1].source_id = 101;
    synthetic.sources[1].coarse_band_energy_share = {0.75f, 0.20f, 0.05f};

    synthetic.sources[2] = synthetic.sources[0];
    synthetic.sources[2].source_id = 102;
    synthetic.sources[2].relative_energy = 0.20f;
    synthetic.sources[2].coarse_band_energy_share = {0.05f, 0.10f, 0.85f};

    synthetic.sources[3] = synthetic.sources[0];
    synthetic.sources[3].source_id = 103;
    synthetic.sources[3].lane_kind = spatial_audio_lane_kind::shared_effect_return;
    synthetic.sources[3].relative_energy = 0.20f;
    synthetic.sources[3].coarse_band_energy_share = {0.80f, 0.15f, 0.05f};

    realtime_spatial_overlap_pair_observation close_pair{};
    realtime_spatial_overlap_pair_observation far_pair{};
    assert(synthetic.pair(0, 1, close_pair));
    assert(synthetic.pair(0, 2, far_pair));
    assert(close_pair.coarse_spectral_overlap > 0.90f);
    assert(far_pair.coarse_spectral_overlap < 0.30f);
    realtime_spatial_overlap_pair_observation wet_pair{};
    assert(!synthetic.pair(0, 3, wet_pair));
    synthetic.scene.coarse_spectral_overlap =
        synthetic.reconstructed_coarse_spectral_overlap();

    const auto synthetic_validation = validate_realtime_spatial_governor_trace(synthetic);
    assert(synthetic_validation.valid);
    assert(synthetic_validation.active_dry_pair_count == 3);
    assert(synthetic_validation.strongest_pair_overlap > 0.90f);
    assert(std::fabs(
        synthetic_validation.reconstructed_shared_effect_energy_share - 0.20f) < 1.0e-5f);
    assert(std::fabs(
        synthetic_validation.reconstructed_energy_concentration - 0.26f) < 1.0e-5f);

    // Corruptions are classified, not hand-waved. This makes the same validator
    // usable as an admission gate for future Sonic 3 and SPC corpus traces.
    auto bad_sequence = synthetic;
    bad_sequence.sequence_index = 0;
    const auto bad_sequence_validation = validate_realtime_spatial_governor_trace(bad_sequence);
    assert(!bad_sequence_validation.valid);
    assert(bad_sequence_validation.error
        == realtime_spatial_governor_trace_error::invalid_sequence);

    auto bad_renderer = synthetic;
    bad_renderer.renderer_budget.depth_scale = 0.5f;
    const auto bad_renderer_validation =
        validate_realtime_spatial_governor_trace(bad_renderer);
    assert(!bad_renderer_validation.valid);
    assert(bad_renderer_validation.error
        == realtime_spatial_governor_trace_error::renderer_budget_mismatch);

    auto bad_pair = synthetic;
    bad_pair.scene.coarse_spectral_overlap = 0.0f;
    const auto bad_pair_validation = validate_realtime_spatial_governor_trace(bad_pair);
    assert(!bad_pair_validation.valid);
    assert(bad_pair_validation.error
        == realtime_spatial_governor_trace_error::pair_overlap_mismatch);

    auto bad_count = synthetic;
    bad_count.scene.active_lane_count = 3;
    const auto bad_count_validation = validate_realtime_spatial_governor_trace(bad_count);
    assert(!bad_count_validation.valid);
    assert(bad_count_validation.error
        == realtime_spatial_governor_trace_error::scene_count_mismatch);

    auto bad_wet = synthetic;
    bad_wet.scene.shared_effect_energy_share = 0.90f;
    const auto bad_wet_validation = validate_realtime_spatial_governor_trace(bad_wet);
    assert(!bad_wet_validation.valid);
    assert(bad_wet_validation.error
        == realtime_spatial_governor_trace_error::scene_energy_mismatch);

    return 0;
}
