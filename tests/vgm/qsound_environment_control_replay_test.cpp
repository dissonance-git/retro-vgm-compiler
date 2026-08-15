#include "../../components/vgm/enhancement/qsound_environment_control_replay.h"

#include <array>
#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

namespace {

command_event make_c4(const std::array<std::uint8_t, 3>& payload) {
    command_event event;
    event.kind = command_event_kind::command;
    event.command = 0xC4;
    event.payload = payload.data();
    event.payload_size = static_cast<std::uint32_t>(payload.size());
    return event;
}

} // namespace

int main() {
    const std::array<std::uint8_t, 3> feedback_payload{{0x12, 0x34, 0x93}};
    const std::array<std::uint8_t, 3> wet_delay_payload{{0x00, 0x07, 0xDE}};
    const command_event feedback = make_c4(feedback_payload);
    const command_event wet_delay = make_c4(wet_delay_payload);

    qsound_block_capture capture;
    qsound_environment_control_state state;

    capture.begin_block(1000);
    capture.observe(feedback, 1010);
    capture.observe(wet_delay, 1020);
    const auto replayed = qsound_replay_environment_controls(capture, 64, state);
    CHECK(replayed.status == qsound_environment_replay_status::replayed);
    CHECK(replayed.applied == 2u);
    CHECK(state.echo_feedback().known);
    CHECK(state.echo_feedback().raw_value == 0x1234u);
    CHECK(state.wet_delay(0).known);
    CHECK(state.wet_delay(0).raw_value == 7u);

    // Destination-block clipping preserves event order when several controls
    // occur beyond the rendered tail.
    state.reset();
    capture.begin_block(1000);
    capture.observe(feedback, 1100);
    capture.observe(wet_delay, 1200);
    const auto clipped = qsound_replay_environment_controls(capture, 16, state);
    CHECK(clipped.status == qsound_environment_replay_status::replayed);
    CHECK(clipped.applied == 2u);
    CHECK(state.echo_feedback().known);
    CHECK(state.wet_delay(0).known);

    // Timeline failure is atomic: a previously valid state remains unchanged
    // rather than receiving the valid prefix of a malformed capture.
    state.reset();
    CHECK(state.apply(0x93, 0x5555));
    capture.begin_block(1000);
    capture.observe(feedback, 1020);
    capture.observe(wet_delay, 1010);
    const auto reversed = qsound_replay_environment_controls(capture, 64, state);
    CHECK(reversed.status == qsound_environment_replay_status::non_monotonic_timeline);
    CHECK(reversed.applied == 0u);
    CHECK(state.echo_feedback().known);
    CHECK(state.echo_feedback().raw_value == 0x5555u);
    CHECK(!state.wet_delay(0).known);

    // Overflow also fails before state mutation.
    state.reset();
    CHECK(state.apply(0x93, 0x6666));
    capture.begin_block(0);
    for (std::size_t i = 0; i <= qsound_block_capture::capacity; ++i)
        capture.observe(feedback, static_cast<std::uint64_t>(i));
    CHECK(capture.environment_overflowed());
    const auto overflowed = qsound_replay_environment_controls(capture, qsound_block_capture::capacity + 1, state);
    CHECK(overflowed.status == qsound_environment_replay_status::capture_overflow);
    CHECK(overflowed.applied == 0u);
    CHECK(state.echo_feedback().raw_value == 0x6666u);

    // Source-local C4 writes do not enter the environment replay stream.
    const std::array<std::uint8_t, 3> pan_payload{{0x01, 0x20, 0x80}};
    const command_event pan = make_c4(pan_payload);
    state.reset();
    capture.begin_block(0);
    capture.observe(pan, 1);
    const auto source_only = qsound_replay_environment_controls(capture, 8, state);
    CHECK(source_only.status == qsound_environment_replay_status::replayed);
    CHECK(source_only.applied == 0u);
    CHECK(!state.echo_feedback().known);

    return 0;
}
