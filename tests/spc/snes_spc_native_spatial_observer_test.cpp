#include "SPC_DSP.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {

struct observer_state {
    std::size_t calls = 0;
    int last_dry_count = 0;
    std::array<short, SPC_DSP::voice_count> last_dry{};
    short last_echo_left = 0;
    short last_echo_right = 0;
};

void observe_native_spatial(
    void* user,
    short const* dry_voice,
    int dry_voice_count,
    short echo_left,
    short echo_right)
{
    auto* state = static_cast<observer_state*>(user);
    assert(state != nullptr);
    assert(dry_voice != nullptr);
    assert(dry_voice_count == SPC_DSP::voice_count);

    ++state->calls;
    state->last_dry_count = dry_voice_count;
    for (int voice = 0; voice < dry_voice_count; ++voice)
        state->last_dry[static_cast<std::size_t>(voice)] = dry_voice[voice];
    state->last_echo_left = echo_left;
    state->last_echo_right = echo_right;
}

} // namespace

int main()
{
    constexpr int hardware_frames = 2;
    constexpr int clocks = hardware_frames * 32;
    constexpr int stereo_samples = hardware_frames * 2;

    std::array<unsigned char, 65536> observed_ram{};
    std::array<unsigned char, 65536> control_ram{};
    std::array<short, stereo_samples> observed_output{};
    std::array<short, stereo_samples> control_output{};

    SPC_DSP observed{};
    SPC_DSP control{};
    observed.init(observed_ram.data());
    control.init(control_ram.data());
    observed.reset();
    control.reset();

    observer_state state{};
    observed.set_native_spatial_observer(observe_native_spatial, &state);
    observed.set_output(observed_output.data(), static_cast<int>(observed_output.size()));
    control.set_output(control_output.data(), static_cast<int>(control_output.size()));

    observed.run(clocks);
    control.run(clocks);

    // One callback per 32-clock S-DSP hardware frame, carrying exactly the
    // eight physical dry voices plus the two shared post-EVOL echo values.
    assert(state.calls == hardware_frames);
    assert(state.last_dry_count == SPC_DSP::voice_count);

    // Observation is not allowed to perturb the pinned DSP. Starting from
    // byte-identical RAM/register state, the protected stereo result must remain
    // sample-identical with and without the callback installed.
    assert(observed.sample_count() == stereo_samples);
    assert(control.sample_count() == stereo_samples);
    for (std::size_t sample = 0; sample < observed_output.size(); ++sample)
        assert(observed_output[sample] == control_output[sample]);

    // Removing the observer must stop callbacks without resetting synthesis.
    observed.set_native_spatial_observer(nullptr, nullptr);
    observed.set_output(observed_output.data(), static_cast<int>(observed_output.size()));
    observed.run(32);
    assert(state.calls == hardware_frames);

    return 0;
}
