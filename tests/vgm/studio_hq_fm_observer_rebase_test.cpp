#include "components/vgm/foo_input_vgm/src/studio_hq_fm_observer.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

int main() {
    using namespace foobar_vgm::source_audio;

    using observer_type = studio_hq_fm_observer<2, 512, 256>;
    observer_type observer;
    assert(observer.configure(44100, 44100));

    std::array<studio_stereo_sample, 80> lane0{};
    std::array<studio_stereo_sample, 80> lane1{};
    for (std::size_t i = 0; i < lane0.size(); ++i) {
        lane0[i] = {static_cast<double>(i), -static_cast<double>(i)};
        lane1[i] = {static_cast<double>(i * 2), -static_cast<double>(i * 2)};
    }
    const std::array<const studio_stereo_sample*, 2> lanes{{
        lane0.data(), lane1.data()
    }};

    studio_linear_timing_snapshot same_rate{};
    same_rate.source_rate_hz = 44100;
    same_rate.destination_rate_hz = 44100;

    // Initial device attachment has a proven silent negative-time FM state.
    // The symmetric FIR can therefore own destination frame zero as soon as its
    // real future support has arrived, with no reference-to-FIR startup switch.
    const auto initial = observer.observe_segment(
        same_rate,
        lanes,
        lane0.size(),
        lane0.size(),
        studio_hq_fm_gain{1, 1});
    assert(initial.valid);
    assert(initial.native_base == 0);
    assert(initial.destination_base == 0);
    assert(initial.startup_reference_frames == 0);
    assert(observer.studio_domain_started());
    assert(observer.first_studio_destination_ordinal() == 0);
    assert(observer.next_destination_ordinal() == lane0.size());

    observer_type::ready_frame ready{};
    assert(observer.pop_ready_frame(ready));
    assert(ready.valid);
    assert(ready.destination_ordinal == 0);

    // A seek discards native FIR history but does not renumber PlayerA output.
    // Pre-seek music is not silence, so reset revokes the initial zero-prefix
    // evidence and the observer falls back until 31 true history samples exist.
    constexpr std::uint64_t destination_base = 1000;
    observer.reset(destination_base);
    assert(observer.valid());
    assert(observer.next_native_ordinal() == 0);
    assert(observer.next_destination_ordinal() == destination_base);
    assert(observer.next_release_ordinal() == destination_base);
    assert(!observer.studio_domain_started());

    const auto observed = observer.observe_segment(
        same_rate,
        lanes,
        lane0.size(),
        lane0.size(),
        studio_hq_fm_gain{1, 1});
    assert(observed.valid);
    assert(observed.native_base == 0);
    assert(observed.destination_base == destination_base);
    assert(observed.startup_reference_frames
        == studio_source_resampler_kernel::pre_roll);
    assert(observer.studio_domain_started());
    assert(observer.first_studio_destination_ordinal()
        == destination_base + studio_source_resampler_kernel::pre_roll);
    assert(observer.next_destination_ordinal() == destination_base + lane0.size());

    assert(observer.pop_ready_frame(ready));
    assert(ready.valid);
    assert(ready.destination_ordinal
        == destination_base + studio_source_resampler_kernel::pre_roll);

    // Another discontinuity restarts only source history. The observer remains
    // configured and can also admit an upsampler pregeneration sample at a
    // nonzero absolute destination origin, but that is real captured history,
    // not permission to recreate the fresh-start silent prefix.
    constexpr std::uint64_t second_base = 5000;
    observer.reset(second_base);
    assert(observer.valid());
    assert(observer.next_native_ordinal() == 0);
    assert(observer.next_destination_ordinal() == second_base);
    assert(observer.next_release_ordinal() == second_base);

    const std::array<studio_stereo_sample, 1> pregenerated{{{1.0, -1.0}}};
    const std::array<const studio_stereo_sample*, 2> pregeneration_lanes{{
        pregenerated.data(), pregenerated.data()
    }};
    assert(observer.append_initial_pregeneration(pregeneration_lanes, 1));
    assert(observer.next_native_ordinal() == 1);
    assert(observer.next_destination_ordinal() == second_base);

    return 0;
}
