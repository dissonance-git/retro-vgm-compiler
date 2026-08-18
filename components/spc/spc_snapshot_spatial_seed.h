#pragma once

#include "spc_runtime_capture.h"
#include "spc_snapshot.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace gameaudio::spc {

constexpr std::int8_t spc_snapshot_signed_dsp_byte(std::uint8_t value) noexcept
{
    return value < 0x80u
        ? static_cast<std::int8_t>(value)
        : static_cast<std::int8_t>(static_cast<int>(value) - 0x100);
}

// Exact playback-start spatial state recovered from the 128 saved S-DSP
// registers in an SPC snapshot. These are not synthetic musical events: they are
// a bounded bridge that lets the runtime spatial adapter start from the device
// state that already exists at frame zero instead of pretending VOLL/VOLR/EON
// are unknown until the next accepted KON or route observation.
//
// The records deliberately carry no source/sample/persistent-part claim. Voice
// episode generation remains zero until the live DSP accepts a key-on.
inline std::array<spc_runtime_capture_record, 8> make_spc_snapshot_spatial_seed(
    const spc_snapshot& snapshot,
    std::uint64_t tick_rate) noexcept
{
    std::array<spc_runtime_capture_record, 8> records{};
    constexpr std::size_t eon_register = 0x4D;
    const std::uint8_t eon = snapshot.dsp[eon_register];

    for (std::size_t voice = 0; voice < records.size(); ++voice) {
        const std::size_t base = voice * 0x10u;
        auto& record = records[voice];
        record.kind = spc_voice_runtime_event_kind::source_latched;
        record.fields = spc_runtime_capture_field::voice
            | spc_runtime_capture_field::route_gain_left
            | spc_runtime_capture_field::route_gain_right
            | spc_runtime_capture_field::echo_send_enabled;
        record.tick = 0;
        record.tick_rate = tick_rate;
        record.voice = static_cast<std::uint8_t>(voice);
        record.route_gain_left = spc_snapshot_signed_dsp_byte(snapshot.dsp[base]);
        record.route_gain_right = spc_snapshot_signed_dsp_byte(snapshot.dsp[base + 1u]);
        record.echo_send_enabled = (eon & static_cast<std::uint8_t>(1u << voice)) != 0;
    }
    return records;
}

} // namespace gameaudio::spc
