#include "../../components/vgm/enhancement/qsound_echo_block_verifier.h"

#include <array>
#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

void add_source(
    qsound_native_source_capture& capture,
    std::uint64_t sample,
    std::int16_t pcm0 = 0) {
    std::array<std::int16_t, qsound_native_source_count> lanes{};
    lanes[0] = pcm0;
    capture.observe(0, 24038, sample, lanes.data(), lanes.size());
}

void add_mix(
    qsound_native_mix_capture& capture,
    std::uint64_t sample,
    bool valid,
    std::int16_t echo_send0,
    std::int32_t echo_input,
    std::int16_t echo_output) {
    qsound_native_mix_frame frame;
    frame.native_sample = sample;
    frame.accounting_valid = valid;
    frame.pcm_echo_contribution[0] = valid ? echo_send0 : 0;
    frame.echo_feedback = 0;
    frame.echo_length = valid ? 6 : 0;
    frame.echo_input = valid ? echo_input : 0;
    frame.echo_output = valid ? echo_output : 0;
    capture.observe(0, 24038, &frame);
}

} // namespace

int main() {
    qsound_native_source_capture source;
    qsound_native_mix_capture mix;
    qsound_echo_seed seed;
    seed.delay_line[0] = 2;
    seed.delay_position = 0;
    seed.last_sample = 0;

    source.begin_block();
    mix.begin_block();
    add_source(source, 100, 0);
    add_mix(mix, 100, true, 0, 0, 1);
    add_source(source, 101, 0);
    add_mix(mix, 101, false, 0, 0, 0);
    add_source(source, 102, 16384);
    add_mix(mix, 102, true, 2, 131072, 1);

    const auto exact = qsound_verify_echo_block(source, mix, seed);
    CHECK(exact.status == qsound_echo_block_status::exact);
    CHECK(exact.checked_frames == 2u);
    CHECK(exact.unavailable_frames == 1u);
    CHECK(exact.failure_index == 3u);
    CHECK(exact.final_seed.delay_position == 2u);
    CHECK(exact.final_seed.last_sample == 0);
    CHECK(exact.final_seed.delay_line[0] == 0);
    CHECK(exact.final_seed.delay_line[1] == 2);

    // A wrong source/send identity fails before the echo state consumes it.
    source.begin_block();
    mix.begin_block();
    add_source(source, 200, 100);
    add_mix(mix, 200, true, 2, 799, 0); // exact recomposition would be 800
    const auto bad_input = qsound_verify_echo_block(source, mix, {});
    CHECK(bad_input.status == qsound_echo_block_status::echo_input_mismatch);
    CHECK(bad_input.checked_frames == 0u);
    CHECK(bad_input.final_seed.delay_position == 0u);

    // A wrong echo output advances only through the independently valid input
    // transition and then exposes the output mismatch at that exact frame.
    source.begin_block();
    mix.begin_block();
    add_source(source, 300, 0);
    add_mix(mix, 300, true, 0, 0, 7);
    const auto bad_output = qsound_verify_echo_block(source, mix, {});
    CHECK(bad_output.status == qsound_echo_block_status::echo_output_mismatch);
    CHECK(bad_output.checked_frames == 0u);
    CHECK(bad_output.failure_index == 0u);
    CHECK(bad_output.final_seed.delay_position == 1u);

    qsound_echo_seed invalid_seed;
    invalid_seed.delay_position = qsound_echo_state::delay_capacity;
    const auto seed_failure = qsound_verify_echo_block(source, mix, invalid_seed);
    CHECK(seed_failure.status == qsound_echo_block_status::seed_invalid);

    // Capture shape mismatches fail before replay.
    source.begin_block();
    mix.begin_block();
    add_source(source, 400, 0);
    const auto shape_failure = qsound_verify_echo_block(source, mix, {});
    CHECK(shape_failure.status == qsound_echo_block_status::capture_shape_mismatch);

    return 0;
}
