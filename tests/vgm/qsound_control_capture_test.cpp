#include "../../components/vgm/enhancement/qsound_block_capture.h"

#include <array>
#include <cstdint>

using namespace gameaudio::vgm;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

int main() {
    qsound_control_state state;

    // Recovered DL-1425 reset state: all 19 source pans point at center, PCM
    // echo contributions are cleared.
    for (std::size_t i = 0; i < qsound_source_count; ++i)
        CHECK(state.pan(i) == 0x120u);
    for (std::size_t i = 0; i < qsound_pcm_source_count; ++i)
        CHECK(state.pcm_echo_contribution(i) == 0);

    qsound_source_control_write decoded;
    CHECK(qsound_decode_source_control_write(0x80, 0x110, decoded));
    CHECK(decoded.kind == qsound_source_control_kind::pan);
    CHECK(decoded.physical_slot == 0);
    CHECK(qsound_decode_source_control_write(0x8F, 0x130, decoded));
    CHECK(decoded.physical_slot == 15);
    CHECK(qsound_decode_source_control_write(0x90, 0x140, decoded));
    CHECK(decoded.physical_slot == 16);
    CHECK(qsound_decode_source_control_write(0x92, 0x160, decoded));
    CHECK(decoded.physical_slot == 18);

    // 0x93 is global delayed-reverb feedback. It is adjacent to the pan range
    // but is not a twentieth source pan.
    CHECK(!qsound_decode_source_control_write(0x93, 0x1234, decoded));
    CHECK(!qsound_decode_source_control_write(0xD9, 0x1234, decoded));
    CHECK(!qsound_decode_source_control_write(0xDE, 0x1234, decoded));
    CHECK(!qsound_decode_source_control_write(0xE4, 0x1234, decoded));

    CHECK(qsound_decode_source_control_write(0xBA, 0x7FFF, decoded));
    CHECK(decoded.kind == qsound_source_control_kind::pcm_echo_contribution);
    CHECK(decoded.physical_slot == 0);
    CHECK(qsound_decode_source_control_write(0xC9, 0x8001, decoded));
    CHECK(decoded.physical_slot == 15);
    CHECK(!qsound_decode_source_control_write(0xCA, 0x1234, decoded));

    CHECK(state.apply(0x80, 0x110));
    CHECK(state.pan(0) == 0x110u);
    CHECK(state.apply(0x92, 0x160));
    CHECK(state.pan(18) == 0x160u);
    CHECK(state.apply(0xBA, 0x8001));
    CHECK(state.pcm_echo_contribution(0) == static_cast<std::int16_t>(0x8001u));
    CHECK(!state.apply(0x93, 0x7777));

    const auto pcm = state.source(0, 0, 4);
    CHECK(pcm.route.raw_pan == 0x110u);
    CHECK(pcm.echo_contribution_known);
    CHECK(pcm.echo_contribution_raw == static_cast<std::int16_t>(0x8001u));
    CHECK(pcm.evidence.generation == 4u);
    CHECK(!pcm.evidence.authored_position_present);

    const auto adpcm = state.source(0, 18, 0);
    CHECK(adpcm.route.raw_pan == 0x160u);
    CHECK(!adpcm.echo_contribution_known);
    CHECK(!adpcm.evidence.authored_position_present);

    state.reset();
    CHECK(state.pan(0) == 0x120u);
    CHECK(state.pan(18) == 0x120u);
    CHECK(state.pcm_echo_contribution(0) == 0);

    qsound_block_capture capture;
    capture.begin_block(1000);

    // VGM C4 payload is [data MSB, data LSB, QSound address].
    const std::array<std::uint8_t, 3> pan_payload{{0x01, 0x10, 0x80}};
    command_event pan_event;
    pan_event.kind = command_event_kind::command;
    pan_event.command = 0xC4;
    pan_event.payload = pan_payload.data();
    pan_event.payload_size = static_cast<std::uint32_t>(pan_payload.size());
    capture.observe(pan_event, 1017);
    CHECK(capture.count() == 1u);
    CHECK(capture.environment_count() == 0u);
    CHECK(capture.controls()[0].sample_offset == 17u);
    CHECK(capture.controls()[0].write.kind == qsound_source_control_kind::pan);
    CHECK(capture.controls()[0].write.physical_slot == 0u);
    CHECK(capture.controls()[0].write.raw_value == 0x0110u);

    const std::array<std::uint8_t, 3> echo_payload{{0xFF, 0x00, 0xBA}};
    command_event echo_event = pan_event;
    echo_event.payload = echo_payload.data();
    capture.observe(echo_event, 1000);
    CHECK(capture.count() == 2u);
    CHECK(capture.environment_count() == 0u);
    CHECK(capture.controls()[1].sample_offset == 0u);
    CHECK(capture.controls()[1].write.kind == qsound_source_control_kind::pcm_echo_contribution);
    CHECK(capture.controls()[1].write.raw_value == 0xFF00u);

    // A source-facing write observed before the declared block start clamps to
    // frame zero, matching the existing VGM realtime collector semantics.
    capture.observe(pan_event, 999);
    CHECK(capture.count() == 3u);
    CHECK(capture.controls()[2].sample_offset == 0u);

    // Shared renderer state is captured on the same command timeline without
    // being admitted to the source-local trace.
    const std::array<std::uint8_t, 3> feedback_payload{{0x12, 0x34, 0x93}};
    command_event feedback_event = pan_event;
    feedback_event.payload = feedback_payload.data();
    capture.observe(feedback_event, 1020);
    CHECK(capture.count() == 3u);
    CHECK(capture.environment_count() == 1u);
    CHECK(capture.environment_controls()[0].sample_offset == 20u);
    CHECK(capture.environment_controls()[0].write.kind == qsound_environment_control_kind::echo_feedback);
    CHECK(capture.environment_controls()[0].write.channel == qsound_environment_global_channel);
    CHECK(capture.environment_controls()[0].write.raw_value == 0x1234u);

    const std::array<std::uint8_t, 3> wet_delay_payload{{0x00, 0x07, 0xDE}};
    command_event wet_delay_event = pan_event;
    wet_delay_event.payload = wet_delay_payload.data();
    capture.observe(wet_delay_event, 1021);
    CHECK(capture.count() == 3u);
    CHECK(capture.environment_count() == 2u);
    CHECK(capture.environment_controls()[1].sample_offset == 21u);
    CHECK(capture.environment_controls()[1].write.kind == qsound_environment_control_kind::wet_delay);
    CHECK(capture.environment_controls()[1].write.channel == 0u);
    CHECK(capture.environment_controls()[1].write.raw_value == 7u);

    command_event wrong_command = pan_event;
    wrong_command.command = 0xC5;
    capture.observe(wrong_command, 1020);
    CHECK(capture.count() == 3u);
    CHECK(capture.environment_count() == 2u);

    command_event malformed = pan_event;
    malformed.payload_size = 2;
    capture.observe(malformed, 1020);
    CHECK(capture.count() == 3u);
    CHECK(capture.environment_count() == 2u);

    // Source and environment capacities are independent. Exhausting one stream
    // does not silently discard valid controls from the other.
    capture.begin_block(0);
    for (std::size_t i = 0; i < qsound_block_capture::capacity; ++i)
        capture.observe(pan_event, static_cast<std::uint64_t>(i));
    CHECK(capture.count() == qsound_block_capture::capacity);
    CHECK(capture.environment_count() == 0u);
    CHECK(!capture.overflowed());
    CHECK(!capture.environment_overflowed());
    capture.observe(pan_event, qsound_block_capture::capacity);
    CHECK(capture.overflowed());
    CHECK(capture.dropped() == 1u);
    CHECK(capture.count() == qsound_block_capture::capacity);

    capture.begin_block(0);
    for (std::size_t i = 0; i < qsound_block_capture::capacity; ++i)
        capture.observe(feedback_event, static_cast<std::uint64_t>(i));
    CHECK(capture.count() == 0u);
    CHECK(capture.environment_count() == qsound_block_capture::capacity);
    CHECK(!capture.overflowed());
    CHECK(!capture.environment_overflowed());
    capture.observe(feedback_event, qsound_block_capture::capacity);
    CHECK(capture.environment_overflowed());
    CHECK(capture.environment_dropped() == 1u);
    CHECK(capture.environment_count() == qsound_block_capture::capacity);

    return 0;
}
