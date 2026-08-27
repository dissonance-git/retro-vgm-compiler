#include "components/spc/spc_runtime_trace_recorder.h"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <utility>

using namespace gameaudio::spc;

namespace {

constexpr std::uint64_t spc_clock_rate = 1024000;

spc_runtime_capture_record event(
    std::int64_t tick,
    std::uint8_t voice = 0) {
    spc_runtime_capture_record record;
    record.kind = spc_voice_runtime_event_kind::key_on_accepted;
    record.fields =
        spc_runtime_capture_field::voice |
        spc_runtime_capture_field::source_index |
        spc_runtime_capture_field::pitch_rate;
    record.tick = tick;
    record.tick_rate = spc_clock_rate;
    record.voice = voice;
    record.source_index = 3;
    record.pitch_rate = 0x1000;
    record.trace_index = 999999;
    record.ram_write_serial = 999999;
    return record;
}

} // namespace

int main() {
    {
        spc_runtime_trace_recorder recorder;
        recorder.observe_voice_event(event(10));
        assert(recorder.ram_write_serial() == 0);
        assert(recorder.next_trace_index() == 1);

        assert(recorder.observe_apuram_write_byte(
            spc_runtime_ram_write_origin::spc700_cpu,
            15,
            spc_clock_rate,
            0x2000,
            0x11) == 1);
        recorder.observe_voice_event(event(20));

        assert(recorder.observe_apuram_write_le16(
            spc_runtime_ram_write_origin::dsp_echo,
            25,
            spc_clock_rate,
            0xFFFF,
            0x4433) == 2);
        recorder.observe_voice_event(event(30));
        assert(recorder.flush_window());

        const auto& trace = recorder.trace();
        assert(trace.ram_writes.size() == 2);
        assert(trace.ram_writes[0].serial == 1);
        assert(trace.ram_writes[0].tick == 15);
        assert(trace.ram_writes[0].tick_rate == spc_clock_rate);
        assert(trace.ram_writes[0].origin == spc_runtime_ram_write_origin::spc700_cpu);
        assert(trace.ram_writes[0].address == 0x2000);
        assert(trace.ram_writes[0].bytes.size() == 1);
        assert(trace.ram_writes[0].bytes[0] == 0x11);
        assert(trace.ram_writes[1].serial == 2);
        assert(trace.ram_writes[1].tick == 25);
        assert(trace.ram_writes[1].origin == spc_runtime_ram_write_origin::dsp_echo);
        assert(trace.ram_writes[1].address == 0xFFFF);
        assert(trace.ram_writes[1].bytes.size() == 2);
        assert(trace.ram_writes[1].bytes[0] == 0x33);
        assert(trace.ram_writes[1].bytes[1] == 0x44);

        assert(trace.windows.size() == 1);
        assert(trace.windows[0].records.size() == 3);
        assert(trace.windows[0].records[0].trace_index == 0);
        assert(trace.windows[0].records[1].trace_index == 1);
        assert(trace.windows[0].records[2].trace_index == 2);
        assert(trace.windows[0].records[0].ram_write_serial == 0);
        assert(trace.windows[0].records[1].ram_write_serial == 1);
        assert(trace.windows[0].records[2].ram_write_serial == 2);
        assert(trace.windows[0].next_trace_index == 3);
        assert(!trace.windows[0].overflowed);
        assert(trace.windows[0].dropped == 0);

        assert(!recorder.flush_window());
        assert(recorder.trace().windows.size() == 1);

        auto completed = recorder.finish();
        assert(completed.ram_writes.size() == 2);
        assert(completed.windows.size() == 1);
        assert(recorder.ram_write_serial() == 0);
        assert(recorder.next_trace_index() == 0);
        assert(recorder.trace().ram_writes.empty());
        assert(recorder.trace().windows.empty());
    }

    {
        spc_runtime_trace_recorder recorder;
        for (std::size_t index = 0;
             index < spc_runtime_capture::capacity + 2;
             ++index) {
            recorder.observe_voice_event(event(static_cast<std::int64_t>(index)));
        }
        assert(recorder.flush_window());

        const auto& window = recorder.trace().windows.front();
        assert(window.records.size() == spc_runtime_capture::capacity);
        assert(window.overflowed);
        assert(window.dropped == 2);
        assert(window.first_dropped.has_value());
        assert(window.first_dropped->trace_index == spc_runtime_capture::capacity);
        assert(window.next_trace_index == spc_runtime_capture::capacity + 2);

        recorder.observe_voice_event(event(20000));
        assert(recorder.flush_window());
        assert(recorder.trace().windows.size() == 2);
        assert(recorder.trace().windows[1].records.size() == 1);
        assert(recorder.trace().windows[1].records[0].trace_index ==
               spc_runtime_capture::capacity + 2);
    }

    {
        // SPC700 callbacks and DSP callbacks share a nominal device-clock rate
        // but are not phase-aligned at frame boundaries. Preserve the raw
        // backstep and causal callback order rather than clamping either clock.
        spc_runtime_trace_recorder recorder;
        recorder.observe_voice_event(event(20));
        assert(recorder.observe_apuram_write_byte(
            spc_runtime_ram_write_origin::spc700_cpu,
            15,
            spc_clock_rate,
            0x2100,
            0x22) == 1);
        recorder.observe_voice_event(event(25));
        assert(recorder.observe_apuram_write_byte(
            spc_runtime_ram_write_origin::spc700_cpu,
            18,
            spc_clock_rate,
            0x2101,
            0x23) == 2);
        recorder.flush_window();

        const auto& trace = recorder.trace();
        assert(trace.cross_lane_backstep_count == 2);
        assert(trace.max_cross_lane_backstep_ticks == 7);
        assert(trace.windows.front().records[0].tick == 20);
        assert(trace.ram_writes[0].tick == 15);
    }

    {
        // A backstep inside one producer lane is still invalid.
        spc_runtime_trace_recorder recorder;
        recorder.observe_voice_event(event(20));
        bool threw = false;
        try {
            recorder.observe_voice_event(event(19));
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);

        const std::uint8_t value = 0x44;
        spc_runtime_trace_recorder cpu_recorder;
        (void)cpu_recorder.observe_apuram_write(
            spc_runtime_ram_write_origin::spc700_cpu,
            20,
            spc_clock_rate,
            0x2200,
            &value,
            1);
        threw = false;
        try {
            (void)cpu_recorder.observe_apuram_write(
                spc_runtime_ram_write_origin::spc700_cpu,
                19,
                spc_clock_rate,
                0x2201,
                &value,
                1);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    {
        spc_runtime_trace_recorder recorder;
        auto invalid = event(0);
        invalid.tick_rate = 0;
        bool threw = false;
        try {
            recorder.observe_voice_event(invalid);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);

        const std::uint8_t value = 0x00;
        threw = false;
        try {
            (void)recorder.observe_apuram_write(
                spc_runtime_ram_write_origin::controlled_fixture,
                0,
                0,
                0x0000,
                &value,
                1);
        } catch (const std::invalid_argument&) {
            threw = true;
        }
        assert(threw);
    }

    return 0;
}
