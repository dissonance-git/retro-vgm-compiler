#pragma once

#include "vgm_command_event.h"
#include "vgm_format_version.h"

#include <cstdint>
#include <optional>

namespace gameaudio::vgm {

// Exact wire-level semantics of VGM 1.60+ DAC Stream Control commands 0x90-0x95.
// The stream transport is chip-neutral. Device-specific meaning of destination
// port/register fields belongs to the target chip adapter.
enum class dac_stream_command_kind : std::uint8_t {
    setup,
    set_data,
    set_frequency,
    start,
    stop,
    fast_start,
};

struct dac_stream_command {
    dac_stream_command_kind kind = dac_stream_command_kind::setup;
    std::uint8_t stream_id = 0;
    bool reserved_stream_id = false;

    // 0x90 setup.
    std::uint8_t chip_type = 0;
    std::uint8_t chip_instance = 0;
    std::uint8_t destination_port = 0;
    std::uint8_t destination_command = 0;

    // 0x91 data source.
    std::uint8_t data_bank_id = 0;
    std::uint8_t step_size = 0;
    std::uint8_t step_base = 0;

    // 0x92 frequency.
    std::uint32_t frequency_hz = 0;

    // 0x93 start.
    std::uint32_t start_offset = 0;
    std::uint8_t play_mode = 0;
    std::uint32_t length = 0;
    bool loop = false;
    bool reverse = false;

    // 0x94 stop.
    bool stop_all = false;

    // 0x95 fast start.
    std::uint16_t block_id = 0;
    std::uint8_t fast_flags = 0;
};

namespace detail {

constexpr std::uint16_t read_le16(const std::uint8_t* data) noexcept {
    return static_cast<std::uint16_t>(data[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) << 8);
}

constexpr std::uint32_t read_le32(const std::uint8_t* data) noexcept {
    return static_cast<std::uint32_t>(data[0]) |
           (static_cast<std::uint32_t>(data[1]) << 8) |
           (static_cast<std::uint32_t>(data[2]) << 16) |
           (static_cast<std::uint32_t>(data[3]) << 24);
}

constexpr std::uint32_t expected_dac_stream_payload_size(const std::uint8_t command) noexcept {
    switch (command) {
    case 0x90: return 4;
    case 0x91: return 4;
    case 0x92: return 5;
    case 0x93: return 10;
    case 0x94: return 1;
    case 0x95: return 4;
    default: return 0;
    }
}

} // namespace detail

inline std::optional<dac_stream_command> decode_dac_stream_command(
    const std::uint32_t version,
    const command_event& event) noexcept {
    if (version < vgm_version_1_60 || event.kind != command_event_kind::command)
        return std::nullopt;

    const std::uint32_t expected = detail::expected_dac_stream_payload_size(event.command);
    if (expected == 0 || event.payload == nullptr || event.payload_size != expected)
        return std::nullopt;

    dac_stream_command result;
    result.stream_id = event.payload[0];
    result.reserved_stream_id = result.stream_id == 0xFFu && event.command != 0x94u;

    switch (event.command) {
    case 0x90: {
        result.kind = dac_stream_command_kind::setup;
        const std::uint8_t encoded_chip = event.payload[1];
        result.chip_type = static_cast<std::uint8_t>(encoded_chip & 0x7Fu);
        result.chip_instance = static_cast<std::uint8_t>((encoded_chip >> 7) & 0x01u);
        result.destination_port = event.payload[2];
        result.destination_command = event.payload[3];
        return result;
    }
    case 0x91:
        result.kind = dac_stream_command_kind::set_data;
        result.data_bank_id = event.payload[1];
        result.step_size = event.payload[2];
        result.step_base = event.payload[3];
        return result;
    case 0x92:
        result.kind = dac_stream_command_kind::set_frequency;
        result.frequency_hz = detail::read_le32(event.payload + 1);
        return result;
    case 0x93:
        result.kind = dac_stream_command_kind::start;
        result.start_offset = detail::read_le32(event.payload + 1);
        result.play_mode = event.payload[5];
        result.length = detail::read_le32(event.payload + 6);
        result.reverse = (result.play_mode & 0x10u) != 0;
        result.loop = (result.play_mode & 0x80u) != 0;
        return result;
    case 0x94:
        result.kind = dac_stream_command_kind::stop;
        result.stop_all = result.stream_id == 0xFFu;
        result.reserved_stream_id = false;
        return result;
    case 0x95:
        result.kind = dac_stream_command_kind::fast_start;
        result.block_id = detail::read_le16(event.payload + 1);
        result.fast_flags = event.payload[3];
        result.loop = (result.fast_flags & 0x01u) != 0;
        result.reverse = (result.fast_flags & 0x10u) != 0;
        return result;
    default:
        return std::nullopt;
    }
}

} // namespace gameaudio::vgm
