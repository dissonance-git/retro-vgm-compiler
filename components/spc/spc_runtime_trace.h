#pragma once

#include "spc_runtime_capture.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace gameaudio::spc {

enum class spc_runtime_ram_write_origin : std::uint8_t {
    spc700_cpu = 0,
    dsp_echo,
    ipl_rom_overlay,
    controlled_fixture,
};

enum class spc_runtime_clock_lane : std::uint8_t {
    spc700 = 0,
    dsp,
    controlled,
};

constexpr std::size_t spc_runtime_clock_lane_count = 3;

constexpr std::size_t spc_runtime_clock_lane_index(
    spc_runtime_clock_lane lane) noexcept {
    return static_cast<std::size_t>(lane);
}

inline const char* spc_runtime_clock_lane_name(
    spc_runtime_clock_lane lane) noexcept {
    switch (lane) {
    case spc_runtime_clock_lane::spc700:
        return "spc700";
    case spc_runtime_clock_lane::dsp:
        return "dsp";
    case spc_runtime_clock_lane::controlled:
        return "controlled";
    }
    return "unknown";
}

constexpr spc_runtime_clock_lane spc_runtime_clock_lane_for_ram_write(
    spc_runtime_ram_write_origin origin) noexcept {
    switch (origin) {
    case spc_runtime_ram_write_origin::spc700_cpu:
    case spc_runtime_ram_write_origin::ipl_rom_overlay:
        return spc_runtime_clock_lane::spc700;
    case spc_runtime_ram_write_origin::dsp_echo:
        return spc_runtime_clock_lane::dsp;
    case spc_runtime_ram_write_origin::controlled_fixture:
        return spc_runtime_clock_lane::controlled;
    }
    return spc_runtime_clock_lane::controlled;
}

constexpr spc_runtime_clock_lane spc_runtime_clock_lane_for_event(
    spc_voice_runtime_event_kind kind) noexcept {
    switch (kind) {
    case spc_voice_runtime_event_kind::key_on_accepted:
    case spc_voice_runtime_event_kind::sample_phase_started:
    case spc_voice_runtime_event_kind::release_entered:
    case spc_voice_runtime_event_kind::became_inactive:
    case spc_voice_runtime_event_kind::source_latched:
        return spc_runtime_clock_lane::dsp;
    case spc_voice_runtime_event_kind::routing_state_changed:
    case spc_voice_runtime_event_kind::execution_reset:
        return spc_runtime_clock_lane::spc700;
    case spc_voice_runtime_event_kind::continuation_lost:
        return spc_runtime_clock_lane::controlled;
    }
    return spc_runtime_clock_lane::controlled;
}

inline const char* spc_runtime_ram_write_origin_name(
    spc_runtime_ram_write_origin origin) noexcept {
    switch (origin) {
    case spc_runtime_ram_write_origin::spc700_cpu:
        return "spc700_cpu";
    case spc_runtime_ram_write_origin::dsp_echo:
        return "dsp_echo";
    case spc_runtime_ram_write_origin::ipl_rom_overlay:
        return "ipl_rom_overlay";
    case spc_runtime_ram_write_origin::controlled_fixture:
        return "controlled_fixture";
    }
    return "unknown";
}

// Lossless offline facts observed after an exact SPC snapshot. The snapshot
// remains the sole initial-state authority. One RAM-write record corresponds to
// one emulated source mutation boundary and therefore one generation serial.
// Timing uses the producer's exact device-clock basis; the accurate snes_spc
// integration uses its 1,024,000-clock/second SPC timebase.
struct spc_runtime_trace_ram_write {
    std::uint64_t serial = 0;
    std::int64_t tick = 0;
    std::uint64_t tick_rate = 0;
    spc_runtime_ram_write_origin origin = spc_runtime_ram_write_origin::spc700_cpu;
    std::uint16_t address = 0;
    std::vector<std::uint8_t> bytes;
};

// A drained fixed-capacity DSP capture window. Overflow is preserved explicitly
// so missing observations become a semantic continuity break during replay.
struct spc_runtime_trace_window {
    std::vector<spc_runtime_capture_record> records;
    bool overflowed = false;
    std::uint64_t dropped = 0;
    std::optional<spc_runtime_capture_record> first_dropped{};
    std::uint64_t next_trace_index = 0;
};

struct spc_runtime_trace {
    std::vector<spc_runtime_trace_ram_write> ram_writes;
    std::vector<spc_runtime_trace_window> windows;
    std::uint64_t cross_lane_backstep_count = 0;
    std::uint64_t max_cross_lane_backstep_ticks = 0;
};

} // namespace gameaudio::spc
