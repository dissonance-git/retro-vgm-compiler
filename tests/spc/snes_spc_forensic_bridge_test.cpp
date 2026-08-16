#include "components/spc/spc_runtime_trace_recorder.h"
#include "SPC_DSP.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>

using namespace gameaudio::spc;

namespace {

constexpr std::uint64_t spc_clock_rate = 1024000;
constexpr std::uint16_t directory_base = 0x1000;
constexpr std::uint16_t sample_start = 0x2000;
constexpr std::uint16_t loop_start = 0x2000;
constexpr std::uint16_t echo_start = 0x3000;

void set_le16(
    std::array<std::uint8_t, 0x10000>& ram,
    std::uint16_t address,
    std::uint16_t value) {
    ram[address] = static_cast<std::uint8_t>(value & 0xFFu);
    ram[static_cast<std::uint16_t>(address + 1u)] =
        static_cast<std::uint8_t>((value >> 8u) & 0xFFu);
}

const spc_runtime_capture_record* find_record(
    const spc_runtime_trace& trace,
    spc_voice_runtime_event_kind kind) {
    for (const auto& window : trace.windows) {
        for (const auto& record : window.records) {
            if (record.kind == kind)
                return &record;
        }
    }
    return nullptr;
}

std::size_t count_origin(
    const spc_runtime_trace& trace,
    spc_runtime_ram_write_origin origin) {
    std::size_t count = 0;
    for (const auto& write : trace.ram_writes) {
        if (write.origin == origin)
            ++count;
    }
    return count;
}

} // namespace

int main() {
    std::array<std::uint8_t, 0x10000> ram{};

    // SRCN 0 directory entry: start + loop. The one-block BRR sample terminates
    // immediately and loops to itself; payload is deliberately non-zero so the
    // DSP has deterministic signal to feed its echo path.
    set_le16(ram, directory_base + 0, sample_start);
    set_le16(ram, directory_base + 2, loop_start);
    ram[sample_start] = 0x03; // END + LOOP, filter 0, range 0.
    for (std::size_t i = 1; i < 9; ++i)
        ram[sample_start + i] = static_cast<std::uint8_t>(0x11u * i);

    SPC_DSP dsp;
    dsp.init(ram.data());

    spc_runtime_trace_recorder recorder;
    dsp.set_runtime_instrumentation_sink(&recorder, spc_clock_rate);

    // Make every relevant register explicit. This test is about phase-accurate
    // observation, not about inheriting the emulator's power-on register image.
    dsp.write(SPC_DSP::r_flg, 0x00);
    dsp.write(SPC_DSP::r_dir, directory_base >> 8);
    dsp.write(SPC_DSP::r_non, 0x00);
    dsp.write(SPC_DSP::r_pmon, 0x00);
    dsp.write(SPC_DSP::r_eon, 0x01);
    dsp.write(SPC_DSP::r_esa, echo_start >> 8);
    dsp.write(SPC_DSP::r_edl, 0x01);
    dsp.write(SPC_DSP::r_efb, 0x20);
    dsp.write(SPC_DSP::r_mvoll, 0x7F);
    dsp.write(SPC_DSP::r_mvolr, 0x7F);
    dsp.write(SPC_DSP::r_evoll, 0x40);
    dsp.write(SPC_DSP::r_evolr, 0x40);
    dsp.write(SPC_DSP::r_koff, 0x00);

    constexpr int voice0 = 0x00;
    dsp.write(voice0 + SPC_DSP::v_voll, 0x7F);
    dsp.write(voice0 + SPC_DSP::v_volr, 0x7F);
    dsp.write(voice0 + SPC_DSP::v_pitchl, 0x00);
    dsp.write(voice0 + SPC_DSP::v_pitchh, 0x10); // 0x1000 = nominal sample rate.
    dsp.write(voice0 + SPC_DSP::v_srcn, 0x00);
    dsp.write(voice0 + SPC_DSP::v_adsr0, 0x8F);
    dsp.write(voice0 + SPC_DSP::v_adsr1, 0xE0);
    dsp.write(voice0 + SPC_DSP::v_gain, 0x00);

    // A register write is not an accepted key-on. The observer must stay empty
    // until the accurate DSP reaches the phase that consumes pending KON.
    dsp.write(SPC_DSP::r_kon, 0x01);
    assert(recorder.next_trace_index() == 0);
    assert(recorder.ram_write_serial() == 0);

    // Two sample periods are enough for the DSP's every-other-sample KON latch
    // and voice-0 V3c phase to accept the request. Run incrementally so this
    // assertion would catch a hook placed directly on register write.
    dsp.run(31);
    assert(recorder.next_trace_index() == 0);
    dsp.run(33);
    assert(recorder.next_trace_index() >= 1);

    // Continue through KON setup, BRR latch/decoding, envelope activity, and at
    // least several echo writes. This remains tiny compared with one second.
    dsp.run(32 * 24);

    const auto trace = recorder.finish();
    assert(!trace.windows.empty());

    const auto* accepted = find_record(
        trace,
        spc_voice_runtime_event_kind::key_on_accepted);
    const auto* sample_started = find_record(
        trace,
        spc_voice_runtime_event_kind::sample_phase_started);
    assert(accepted != nullptr);
    assert(sample_started != nullptr);

    assert(accepted->tick_rate == spc_clock_rate);
    assert(sample_started->tick_rate == spc_clock_rate);
    assert(sample_started->tick > accepted->tick);
    assert(accepted->voice == 0);
    assert(sample_started->voice == 0);

    assert(has_field(sample_started->fields, spc_runtime_capture_field::source_index));
    assert(has_field(sample_started->fields, spc_runtime_capture_field::brr_address));
    assert(has_field(
        sample_started->fields,
        spc_runtime_capture_field::directory_loop_address));
    assert(sample_started->source_index == 0);
    assert(sample_started->brr_address == sample_start);
    assert(sample_started->directory_loop_address == loop_start);
    assert(sample_started->pitch_rate == 0x1000);

    // Echo writes are genuine APURAM mutations in the same causal clock. We do
    // not assert the synthesized value here, only that the real accurate DSP
    // changed the configured echo region and the recorder classified it as DSP
    // ownership rather than a fictitious SPC700 CPU write.
    assert(count_origin(trace, spc_runtime_ram_write_origin::dsp_echo) > 0);
    bool echo_address_seen = false;
    std::optional<std::int64_t> previous_write_tick{};
    for (const auto& write : trace.ram_writes) {
        assert(write.tick_rate == spc_clock_rate);
        if (previous_write_tick.has_value())
            assert(write.tick >= *previous_write_tick);
        previous_write_tick = write.tick;

        if (write.origin != spc_runtime_ram_write_origin::dsp_echo)
            continue;
        assert(write.bytes.size() == 2);
        if (write.address >= echo_start && write.address < echo_start + 0x800)
            echo_address_seen = true;
    }
    assert(echo_address_seen);

    return 0;
}
