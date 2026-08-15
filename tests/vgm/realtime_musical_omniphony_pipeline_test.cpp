#include "../../model/realtime_musical_omniphony_pipeline.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

struct fake_renderer_state {
    bool fail = false;
    float observed_foundation = -1.0f;
    std::size_t calls = 0;
    std::size_t resets = 0;
};

std::uint32_t fake_major() { return 0; }
std::uint32_t fake_minor() { return 3; }

std::int32_t fake_reset(void* processor)
{
    auto* state = static_cast<fake_renderer_state*>(processor);
    assert(state != nullptr);
    ++state->resets;
    state->observed_foundation = -1.0f;
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
        fake_process));

    std::array<float, frames> source_scratch{};
    std::array<float, frames * 2> stereo{};

    // A renderer failure must not silently advance musical memory. A caller can
    // retry or fall back with the same causal history still intact.
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
    assert(pipeline.frontend().tracker().stream_seconds() == 0.0);
    assert(renderer.observed_foundation == 0.0f);

    // First successful render still has no current-block lookahead. Only after
    // it reaches the renderer may the raw PCM update future musical memory.
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
    assert(first.rendered);
    assert(first.learned);
    assert(first.renderer_status == 0);
    assert(renderer.observed_foundation == 0.0f);
    assert(std::fabs(pipeline.frontend().tracker().stream_seconds() - 0.10) < 1.0e-9);

    // The next block can use the completed first block as past-only musical
    // evidence. This is the causal DSP memory path, not a soundtrack prepass.
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
    assert(renderer.observed_foundation > 0.0f);
    assert(std::fabs(pipeline.frontend().tracker().stream_seconds() - 0.20) < 1.0e-9);
    assert(renderer.calls == 3);

    // A seek/track reset clears both the musical-memory side and the renderer
    // side. The first block after reset must again be history-free.
    assert(pipeline.reset());
    assert(renderer.resets == 1);
    assert(pipeline.frontend().tracker().stream_seconds() == 0.0);
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
    assert(renderer.observed_foundation == 0.0f);

    return 0;
}
