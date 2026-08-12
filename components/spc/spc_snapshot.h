#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace gameaudio::spc {

// Exact file-layout constants for the conventional SNES-SPC700 snapshot.
// These match the layout consumed by mature SPC emulators such as Game_Music_Emu.
constexpr std::size_t spc_signature_size = 35;
constexpr std::size_t spc_header_size = 0x100;
constexpr std::size_t spc_ram_offset = 0x100;
constexpr std::size_t spc_ram_size = 0x10000;
constexpr std::size_t spc_dsp_offset = 0x10100;
constexpr std::size_t spc_dsp_size = 0x80;
constexpr std::size_t spc_min_file_size = 0x10180;
constexpr std::size_t spc_full_file_size = 0x10200;
constexpr std::size_t spc_trailing_offset = 0x10180;
constexpr std::size_t spc_unused_size = 0x40;
constexpr std::size_t spc_ipl_offset = 0x101C0;
constexpr std::size_t spc_ipl_size = 0x40;

struct spc_cpu_registers {
    std::uint16_t pc = 0;
    std::uint8_t a = 0;
    std::uint8_t x = 0;
    std::uint8_t y = 0;
    std::uint8_t psw = 0;
    std::uint8_t sp = 0;
};

struct spc_snapshot {
    std::array<std::uint8_t, spc_signature_size> signature{};
    std::uint8_t has_id666 = 0;
    std::uint8_t version = 0;
    spc_cpu_registers cpu{};

    // Raw bytes from the remainder of the 0x100-byte file header. Keep these
    // uninterpreted in the common parser so ID666/text variants cannot be
    // mistaken for musical or execution truth.
    std::array<std::uint8_t, 0xD4> header_payload{};

    std::array<std::uint8_t, spc_ram_size> ram{};
    std::array<std::uint8_t, spc_dsp_size> dsp{};

    // Full-size SPC files commonly carry another 0x80 bytes after the minimum
    // RAM+DSP image. Preserve them byte-for-byte when present without requiring
    // them for a valid minimum snapshot.
    bool has_trailing_state = false;
    std::array<std::uint8_t, spc_unused_size> trailing_unused{};
    std::array<std::uint8_t, spc_ipl_size> ipl_rom{};

    std::size_t source_size = 0;
};

inline bool has_spc_signature(const std::uint8_t* data, std::size_t size) noexcept {
    // Game_Music_Emu intentionally checks the stable 27-byte signature prefix,
    // not every later version/control byte in the 35-byte signature area.
    static constexpr char prefix[] = "SNES-SPC700 Sound File Data";
    constexpr std::size_t prefix_size = sizeof(prefix) - 1;
    return data != nullptr && size >= prefix_size &&
        std::memcmp(data, prefix, prefix_size) == 0;
}

inline spc_snapshot parse_spc_snapshot(const std::uint8_t* data, std::size_t size) {
    if (!has_spc_signature(data, size))
        throw std::invalid_argument("not an SNES-SPC700 snapshot");
    if (size < spc_min_file_size)
        throw std::invalid_argument("truncated SNES-SPC700 snapshot");

    spc_snapshot snapshot;
    snapshot.source_size = size;
    std::memcpy(snapshot.signature.data(), data, snapshot.signature.size());

    snapshot.has_id666 = data[0x23];
    snapshot.version = data[0x24];
    snapshot.cpu.pc = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(data[0x25]) |
        (static_cast<std::uint16_t>(data[0x26]) << 8u));
    snapshot.cpu.a = data[0x27];
    snapshot.cpu.x = data[0x28];
    snapshot.cpu.y = data[0x29];
    snapshot.cpu.psw = data[0x2A];
    snapshot.cpu.sp = data[0x2B];

    std::memcpy(snapshot.header_payload.data(), data + 0x2C, snapshot.header_payload.size());
    std::memcpy(snapshot.ram.data(), data + spc_ram_offset, snapshot.ram.size());
    std::memcpy(snapshot.dsp.data(), data + spc_dsp_offset, snapshot.dsp.size());

    if (size >= spc_full_file_size) {
        snapshot.has_trailing_state = true;
        std::memcpy(
            snapshot.trailing_unused.data(),
            data + spc_trailing_offset,
            snapshot.trailing_unused.size());
        std::memcpy(snapshot.ipl_rom.data(), data + spc_ipl_offset, snapshot.ipl_rom.size());
    }

    return snapshot;
}

inline spc_snapshot parse_spc_snapshot(const std::array<std::uint8_t, spc_full_file_size>& data) {
    return parse_spc_snapshot(data.data(), data.size());
}

} // namespace gameaudio::spc
