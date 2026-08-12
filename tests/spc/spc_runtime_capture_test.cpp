#include "../../components/spc/spc_runtime_capture.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

using namespace gameaudio::spc;

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (false)

static spc_runtime_capture_record make_record(
    spc_voice_runtime_event_kind kind,
    std::int64_t tick,
    std::uint8_t voice,
    std::uint8_t source,
    std::uint64_t ram_serial) {
    spc_runtime_capture_record record;
    record.kind = kind;
    record.fields =
        spc_runtime_capture_field::voice |
        spc_runtime_capture_field::source_index |
        spc_runtime_capture_field::brr_address |
        spc_runtime_capture_field::directory_loop_address |
        spc_runtime_capture_field::key_on_delay |
        spc_runtime_capture_field::noise_enabled;
    record.tick = tick;
    record.tick_rate = 1024000;
    record.ram_write_serial = ram_serial;
    record.voice = voice;
    record.source_index = source;
    record.brr_address = static_cast<std::uint16_t>(0x3000u + source * 0x10u);
    record.directory_loop_address = static_cast<std::uint16_t>(record.brr_address + 9u);
    record.key_on_delay = 19;
    record.noise_enabled = false;
    return record;
}

int main() {
    static_assert(std::is_trivially_copyable<spc_runtime_capture_record>::value,
        "realtime capture record must remain a trivially copyable value type");

    spc_runtime_capture capture;
    CHECK(capture.count() == 0);
    CHECK(capture.next_trace_index() == 0);
    CHECK(!capture.overflowed());

    // Equal device timestamps remain strictly ordered by observation ordinal.
    capture.observe(make_record(
        spc_voice_runtime_event_kind::key_on_accepted,
        100,
        0,
        3,
        7));
    capture.observe(make_record(
        spc_voice_runtime_event_kind::source_latched,
        100,
        0,
        4,
        7));

    CHECK(capture.count() == 2);
    CHECK(capture.records()[0].tick == capture.records()[1].tick);
    CHECK(capture.records()[0].trace_index == 0);
    CHECK(capture.records()[1].trace_index == 1);
    CHECK(capture.records()[0].ram_write_serial == 7);
    CHECK(capture.records()[1].ram_write_serial == 7);
    CHECK(capture.records()[0].source_index == 3);
    CHECK(capture.records()[1].source_index == 4);

    // Realtime records convert to the existing off-thread semantic event
    // without making absent fields appear present.
    const auto semantic = make_spc_voice_runtime_event(capture.records()[0]);
    CHECK(semantic.kind == spc_voice_runtime_event_kind::key_on_accepted);
    CHECK(semantic.voice.has_value() && *semantic.voice == 0);
    CHECK(semantic.source_index.has_value() && *semantic.source_index == 3);
    CHECK(semantic.brr_address.has_value() && *semantic.brr_address == 0x3030);
    CHECK(semantic.key_on_delay.has_value() && *semantic.key_on_delay == 19);
    CHECK(semantic.noise_enabled.has_value() && !*semantic.noise_enabled);
    CHECK(!semantic.envelope_value.has_value());
    CHECK(!semantic.pitch_rate.has_value());

    // Draining a capture window must not reset execution identity. The next
    // record continues the same global trace ordinal across audio blocks.
    capture.begin_window();
    CHECK(capture.count() == 0);
    CHECK(capture.next_trace_index() == 2);
    capture.observe(make_record(
        spc_voice_runtime_event_kind::sample_phase_started,
        120,
        0,
        4,
        8));
    CHECK(capture.count() == 1);
    CHECK(capture.records()[0].trace_index == 2);
    CHECK(capture.next_trace_index() == 3);

    // Fill a fresh trace exactly to capacity and then force two dropped
    // observations. Dropped records still consume ordinals so a subsequent
    // capture window cannot pretend the trace stayed contiguous.
    capture.reset_trace();
    CHECK(capture.next_trace_index() == 0);
    for (std::size_t i = 0; i < spc_runtime_capture::capacity; ++i) {
        capture.observe(make_record(
            spc_voice_runtime_event_kind::source_latched,
            static_cast<std::int64_t>(i),
            static_cast<std::uint8_t>(i & 7u),
            static_cast<std::uint8_t>(i & 0xFFu),
            static_cast<std::uint64_t>(i)));
    }
    CHECK(capture.count() == spc_runtime_capture::capacity);
    CHECK(!capture.overflowed());
    CHECK(capture.records()[0].trace_index == 0);
    CHECK(capture.records()[spc_runtime_capture::capacity - 1].trace_index ==
        spc_runtime_capture::capacity - 1);

    capture.observe(make_record(
        spc_voice_runtime_event_kind::source_latched,
        20000,
        1,
        1,
        20000));
    capture.observe(make_record(
        spc_voice_runtime_event_kind::release_entered,
        20001,
        1,
        1,
        20001));
    CHECK(capture.overflowed());
    CHECK(capture.dropped() == 2);
    CHECK(capture.next_trace_index() == spc_runtime_capture::capacity + 2);

    const std::uint64_t next_after_gap = capture.next_trace_index();
    capture.begin_window();
    CHECK(!capture.overflowed());
    CHECK(capture.dropped() == 0);
    capture.observe(make_record(
        spc_voice_runtime_event_kind::source_latched,
        21000,
        2,
        9,
        21000));
    CHECK(capture.records()[0].trace_index == next_after_gap);

    // A true execution reset is the only operation that restarts source-order
    // identity at zero.
    capture.reset_trace();
    CHECK(capture.count() == 0);
    CHECK(capture.next_trace_index() == 0);
    capture.observe(make_record(
        spc_voice_runtime_event_kind::key_on_accepted,
        0,
        0,
        0,
        0));
    CHECK(capture.records()[0].trace_index == 0);

    return 0;
}
