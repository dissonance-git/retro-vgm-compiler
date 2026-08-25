#include "components/vgm/foo_input_vgm/src/studio_alignment_queue.h"
#include "components/vgm/foo_input_vgm/src/studio_frame_transport.h"
#include "components/vgm/foo_input_vgm/src/studio_hq_fm_observer.h"
#include "components/vgm/foo_input_vgm/src/studio_source_resampler.h"
#include "components/vgm/foo_input_vgm/src/studio_source_stream.h"
#include "components/vgm/foo_input_vgm/src/studio_source_timeline.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace { constexpr double pi = 3.141592653589793238462643383279502884; }
int main() {
    using namespace foobar_vgm::source_audio;
    studio_source_resampler_kernel kernel;
    assert(!kernel.configure(0.0, 48000.0));
    assert(kernel.configure(53267.0, 44100.0));
    assert(kernel.configured());
    assert(kernel.cutoff() > 0.0 && kernel.cutoff() < 1.0);
    constexpr std::size_t frames = 1024;
    std::array<studio_stereo_sample, frames> constant{};
    for (auto& sample : constant) sample = {0.25, -0.5};
    for (double position : {128.125, 256.5, 512.875}) {
        const auto reconstructed = kernel.reconstruct(constant.data(), constant.size(), position);
        assert(reconstructed.valid);
        assert(std::abs(reconstructed.sample.left - 0.25) < 1.0e-6);
        assert(std::abs(reconstructed.sample.right + 0.5) < 1.0e-6);
    }
    std::array<studio_stereo_sample, frames> passband{};
    constexpr double passband_frequency = 0.20;
    for (std::size_t i = 0; i < passband.size(); ++i) {
        const double value = std::sin(2.0 * pi * passband_frequency * static_cast<double>(i));
        passband[i] = {value, -value};
    }
    for (double position : {400.125, 400.25, 400.5, 400.75, 400.875}) {
        const double expected = std::sin(2.0 * pi * passband_frequency * position);
        const auto reconstructed = kernel.reconstruct(passband.data(), passband.size(), position);
        assert(reconstructed.valid);
        assert(std::abs(reconstructed.sample.left - expected) < 3.0e-5);
        assert(std::abs(reconstructed.sample.right + expected) < 3.0e-5);
    }
    std::array<studio_stereo_sample, frames> stopband{};
    constexpr double stopband_frequency = 0.45;
    for (std::size_t i = 0; i < stopband.size(); ++i) {
        const double value = std::sin(2.0 * pi * stopband_frequency * static_cast<double>(i));
        stopband[i] = {value, value};
    }
    const double source_step = 53267.0 / 44100.0;
    double energy = 0.0;
    std::size_t count = 0;
    for (double position = 300.0; position < 700.0; position += source_step) {
        const auto reconstructed = kernel.reconstruct(stopband.data(), stopband.size(), position);
        assert(reconstructed.valid);
        energy += reconstructed.sample.left * reconstructed.sample.left;
        ++count;
    }
    assert(count != 0);
    const double rms = std::sqrt(energy / static_cast<double>(count));
    assert(rms < 1.0e-3);

    studio_linear_timing_snapshot exact_up{};
    exact_up.source_rate_hz = 53267;
    exact_up.destination_rate_hz = 96000;
    exact_up.sample_p = 0;
    exact_up.sample_next = 1;
    const auto exact0 = studio_linear_source_position(exact_up, 1, 0);
    assert(exact0.valid);
    assert(exact0.phase_units == -static_cast<std::int64_t>(
        studio_source_resampler_kernel::phase_count));

    constexpr std::uint64_t split = 50;
    const std::uint64_t split_fp =
        ((split - 1u) * (1u << 11) * exact_up.source_rate_hz)
        / exact_up.destination_rate_hz;
    const std::uint32_t split_next = static_cast<std::uint32_t>(
        (split_fp + (1u << 11) - 1u) / (1u << 11));
    studio_linear_timing_snapshot exact_after = exact_up;
    exact_after.sample_p = static_cast<std::uint32_t>(split);
    exact_after.sample_next = split_next;
    const std::uint64_t pulled = split_next - exact_up.sample_next;
    const auto unsplit_position =
        studio_linear_source_position(exact_up, 1, split + 17);
    const auto split_position =
        studio_linear_source_position(exact_after, 1 + pulled, 17);
    assert(unsplit_position.valid && split_position.valid);
    assert(unsplit_position.phase_units == split_position.phase_units);

    studio_source_stream<2048> whole_stream;
    studio_source_stream<2048> split_stream;
    assert(whole_stream.append(0, passband.data(), passband.size()));
    assert(split_stream.append(0, passband.data(), 137));
    assert(split_stream.append(137, passband.data() + 137, 389));
    assert(split_stream.append(526, passband.data() + 526, passband.size() - 526));

    studio_source_phase_position stream_position{
        static_cast<std::int64_t>(
            400 * studio_source_resampler_kernel::phase_count + 1234),
        true
    };
    const auto whole_stream_result = whole_stream.reconstruct(kernel, stream_position);
    const auto split_stream_result = split_stream.reconstruct(kernel, stream_position);
    assert(whole_stream_result.valid && split_stream_result.valid);
    assert(std::abs(whole_stream_result.sample.left
        - split_stream_result.sample.left) < 1.0e-12);
    assert(std::abs(whole_stream_result.sample.right
        - split_stream_result.sample.right) < 1.0e-12);

    const auto stream_window = plan_studio_source_window(stream_position);
    assert(stream_window.valid);
    studio_source_stream<512> future_stream;
    const std::size_t before_final = static_cast<std::size_t>(stream_window.final);
    assert(future_stream.append(0, passband.data(), before_final));
    assert(!future_stream.reconstruct(kernel, stream_position).valid);
    assert(future_stream.append(
        static_cast<std::uint64_t>(before_final),
        passband.data() + before_final,
        1));
    assert(future_stream.reconstruct(kernel, stream_position).valid);

    struct protected_frame {
        std::int32_t reference = 0;
        std::int32_t dac = 0;
        std::int32_t psg = 0;
    };
    studio_alignment_queue<protected_frame, 4> aligned;
    const protected_frame protected0{101, 7, 11};
    const studio_source_phase_position aligned_position{
        static_cast<std::int64_t>(
            64 * studio_source_resampler_kernel::phase_count + 321),
        true
    };
    assert(aligned.push(900, aligned_position, protected0));
    studio_source_stream<128> aligned_stream;
    const auto aligned_window = plan_studio_source_window(aligned_position);
    assert(aligned_window.valid);
    const std::size_t aligned_before_final =
        static_cast<std::size_t>(aligned_window.final);
    assert(aligned_stream.append(0, passband.data(), aligned_before_final));
    assert(!aligned.front_ready(aligned_stream));
    assert(aligned_stream.append(
        static_cast<std::uint64_t>(aligned_before_final),
        passband.data() + aligned_before_final,
        1));
    assert(aligned.front_ready(aligned_stream));
    studio_alignment_queue<protected_frame, 4>::entry aligned_out{};
    assert(aligned.pop_ready(aligned_stream, aligned_out));
    assert(aligned_out.destination_ordinal == 900);
    assert(aligned_out.payload.reference == 101);
    assert(aligned_out.payload.dac == 7);
    assert(aligned_out.payload.psg == 11);

    struct native_integer_sample {
        std::int32_t left = 0;
        std::int32_t right = 0;
    };
    std::array<native_integer_sample, 160> live0{};
    std::array<native_integer_sample, 160> live1{};
    for (std::size_t i = 0; i < live0.size(); ++i) {
        live0[i] = {
            static_cast<std::int32_t>(i * 3u + 1u),
            -static_cast<std::int32_t>(i * 5u + 2u)
        };
        live1[i] = {
            static_cast<std::int32_t>(i * 7u + 3u),
            -static_cast<std::int32_t>(i * 11u + 4u)
        };
    }

    studio_hq_fm_observer<2, 512, 256> observer;
    assert(observer.configure(48000, 48000));
    studio_linear_timing_snapshot same_rate{};
    same_rate.source_rate_hz = 48000;
    same_rate.destination_rate_hz = 48000;

    std::array<const native_integer_sample*, 2> live_first{{
        live0.data(), live1.data()
    }};
    const auto observed_first = observer.observe_segment(
        same_rate, live_first, 80, 80, studio_hq_fm_gain{2, 3});
    assert(observed_first.valid);
    assert(observed_first.native_base == 0);
    assert(observed_first.destination_base == 0);
    assert(observed_first.startup_reference_frames == 0);
    assert(observed_first.newly_ready_studio_frames == 48);
    assert(observed_first.pending_future_frames
        == studio_source_resampler_kernel::post_roll);
    assert(observer.first_studio_destination_ordinal() == 0);
    assert(observer.ready_frames() == 48);

    studio_hq_fm_observer<2, 512, 256>::ready_frame ready{};
    for (std::uint64_t ordinal = 0; ordinal <= 47; ++ordinal) {
        assert(observer.pop_ready_frame(ready));
        assert(ready.valid);
        assert(ready.destination_ordinal == ordinal);
        assert(std::isfinite(ready.lane[0].left));
        assert(std::isfinite(ready.lane[0].right));
    }
    assert(observer.ready_frames() == 0);

    std::array<const native_integer_sample*, 2> live_second{{
        live0.data() + 80, live1.data() + 80
    }};
    const auto observed_second = observer.observe_segment(
        same_rate, live_second, 80, 80, studio_hq_fm_gain{2, 3});
    assert(observed_second.valid);
    assert(observed_second.native_base == 80);
    assert(observed_second.destination_base == 80);
    assert(observed_second.startup_reference_frames == 0);
    assert(observed_second.newly_ready_studio_frames == 80);
    assert(observed_second.pending_future_frames
        == studio_source_resampler_kernel::post_roll);
    assert(observer.ready_frames() == 80);
    for (std::uint64_t ordinal = 48; ordinal <= 127; ++ordinal) {
        assert(observer.pop_ready_frame(ready));
        assert(ready.destination_ordinal == ordinal);
    }
    assert(observer.ready_frames() == 0);
    assert(observer.next_native_ordinal() == 160);
    assert(observer.next_destination_ordinal() == 160);
    assert(observer.finish_reference_tail()
        == studio_source_resampler_kernel::post_roll);
    assert(observer.pending_frames() == 0);

    // Whole-frame transport can receive Studio ordinals out of immediate head
    // order, but cannot emit past an unresolved earlier source-time frame.
    studio_frame_transport<8> transport;
    transport.reset();
    assert(transport.push({0, 100, -100, 10, -10, false}));
    assert(transport.push({1, 200, -200, 20, -20, true}));
    assert(transport.push({2, 300, -300, 30, -30, true}));
    assert(transport.contiguous_final_frames() == 1);
    assert(transport.apply_studio_fm(2, 3000, -3000));
    assert(transport.contiguous_final_frames() == 1);

    studio_transport_output_frame transport_out{};
    assert(transport.pop_final(transport_out));
    assert(transport_out.destination_ordinal == 0);
    assert(!transport_out.used_studio_fm);
    assert(!transport.pop_final(transport_out));

    assert(transport.apply_studio_fm(1, 2000, -2000));
    assert(transport.contiguous_final_frames() == 2);
    assert(transport.pop_final(transport_out));
    assert(transport_out.destination_ordinal == 1);
    assert(transport_out.used_studio_fm);
    assert(transport_out.left == 2180);
    assert(transport_out.right == -2180);
    assert(transport.pop_final(transport_out));
    assert(transport_out.destination_ordinal == 2);
    assert(transport_out.left == 3270);
    assert(transport_out.right == -3270);
    assert(transport.empty());

    assert(transport.push({3, 400, -400, 40, -40, true}));
    assert(transport.push({4, 500, -500, 50, -50, true}));
    assert(!transport.finish_reference_tail(1));
    assert(transport.finish_reference_tail(2));
    assert(transport.contiguous_final_frames() == 2);
    assert(transport.pop_final(transport_out));
    assert(transport_out.destination_ordinal == 3);
    assert(!transport_out.used_studio_fm);
    assert(transport_out.left == 400);
    assert(transport.pop_final(transport_out));
    assert(transport_out.destination_ordinal == 4);
    assert(transport_out.left == 500);

    transport.reset();
    assert(transport.push({10, 700, -700, 70, -70, true}));
    transport.fail_closed_reference();
    assert(transport.pop_final(transport_out));
    assert(transport_out.destination_ordinal == 10);
    assert(!transport_out.used_studio_fm);
    assert(transport_out.left == 700);

    observer.reset();
    assert(observer.valid());
    assert(observer.next_native_ordinal() == 0);
    assert(observer.next_destination_ordinal() == 0);
    assert(observer.pending_frames() == 0);
    assert(observer.ready_frames() == 0);

    assert(!kernel.reconstruct(constant.data(), constant.size(), 12.0).valid);
    assert(!kernel.reconstruct(constant.data(), constant.size(), 1000.0).valid);
    return 0;
}
