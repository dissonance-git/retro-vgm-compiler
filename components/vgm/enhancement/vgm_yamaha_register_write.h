#pragma once

#include "vgm_command_trace_capture.h"

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

// VGM-level Yamaha register-write targets. These names describe the command
// family encoded by the VGM stream, not necessarily the exact silicon variant.
// For example, header context is still required to distinguish YM2612/YM3438,
// YM2151/YM2164, or YM2610/YM2610B where the VGM format permits a variant flag.
enum class yamaha_register_target : std::uint8_t {
    ym2413,
    ym2612,
    ym2151,
    ym2203,
    ym2608,
    ym2610,
    ym3812,
    ym3526,
    y8950,
    ymz280b,
    ymf262,
};

// Exact transport-level meaning recoverable from one complete VGM register
// write command. This does not imply shared synthesis semantics among chips.
struct yamaha_register_write {
    yamaha_register_target target = yamaha_register_target::ym2413;
    std::uint8_t instance = 0;
    std::uint8_t port = 0;
    std::uint8_t address = 0;
    std::uint8_t data = 0;
    std::uint8_t source_command = 0;
};

namespace detail {

struct yamaha_command_semantics {
    yamaha_register_target target;
    std::uint8_t port;
};

constexpr std::optional<yamaha_command_semantics> decode_primary_yamaha_command(
    const std::uint8_t command) noexcept {
    switch (command) {
    case 0x51:
        return yamaha_command_semantics{yamaha_register_target::ym2413, 0};
    case 0x52:
        return yamaha_command_semantics{yamaha_register_target::ym2612, 0};
    case 0x53:
        return yamaha_command_semantics{yamaha_register_target::ym2612, 1};
    case 0x54:
        return yamaha_command_semantics{yamaha_register_target::ym2151, 0};
    case 0x55:
        return yamaha_command_semantics{yamaha_register_target::ym2203, 0};
    case 0x56:
        return yamaha_command_semantics{yamaha_register_target::ym2608, 0};
    case 0x57:
        return yamaha_command_semantics{yamaha_register_target::ym2608, 1};
    case 0x58:
        return yamaha_command_semantics{yamaha_register_target::ym2610, 0};
    case 0x59:
        return yamaha_command_semantics{yamaha_register_target::ym2610, 1};
    case 0x5A:
        return yamaha_command_semantics{yamaha_register_target::ym3812, 0};
    case 0x5B:
        return yamaha_command_semantics{yamaha_register_target::ym3526, 0};
    case 0x5C:
        return yamaha_command_semantics{yamaha_register_target::y8950, 0};
    case 0x5D:
        return yamaha_command_semantics{yamaha_register_target::ymz280b, 0};
    case 0x5E:
        return yamaha_command_semantics{yamaha_register_target::ymf262, 0};
    case 0x5F:
        return yamaha_command_semantics{yamaha_register_target::ymf262, 1};
    default:
        return std::nullopt;
    }
}

} // namespace detail

// Decode only the exact register-write family defined by VGM commands 0x51-0x5F
// and the dual-chip mirrors 0xA1-0xAF. Other VGM commands remain opaque to this
// layer. In particular, 0xA0 is the AY8910 command and must not be mistaken for
// a second Yamaha command.
inline std::optional<yamaha_register_write> decode_yamaha_register_write(
    const command_trace_record& record) noexcept {
    if (record.kind != command_event_kind::command ||
        !has_complete_payload(record) ||
        record.payload_size != 2) {
        return std::nullopt;
    }

    std::uint8_t primary_command = record.command;
    std::uint8_t instance = 0;
    if (record.command >= 0xA1 && record.command <= 0xAF) {
        primary_command = static_cast<std::uint8_t>(record.command - 0x50);
        instance = 1;
    }

    const auto semantics = detail::decode_primary_yamaha_command(primary_command);
    if (!semantics.has_value())
        return std::nullopt;

    return yamaha_register_write{
        semantics->target,
        instance,
        semantics->port,
        record.payload_prefix[0],
        record.payload_prefix[1],
        record.command,
    };
}

} // namespace gameaudio::vgm
