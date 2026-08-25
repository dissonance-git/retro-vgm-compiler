#!/usr/bin/env python3
"""Upgrade the canonical spcplayer child to the complete private source protocol.

Unlike the historical migration scripts, this patch targets the current
components/spc/spcplayer baseline: SRCE-v2 output is already native there, while
the private SPCP input envelope is still v1. It upgrades that envelope directly
to v3 and installs the two optional source-quality providers with the exact
32-bit stdcall ABI exported by patched SNESAPU.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    bom = raw.startswith(b"\xef\xbb\xbf")
    text = raw.decode("utf-8-sig")
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode("utf-8")
    if bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("spcplayer_root", type=Path)
    child = parser.parse_args().spcplayer_root.resolve()
    project_root = Path(__file__).resolve().parents[2]

    overlay = child / "retro_vgm"
    overlay.mkdir(parents=True, exist_ok=True)
    for name in (
        "snesapu_prebrr_provider.h",
        "snesapu_prebrr_packet.h",
        "spc_sample_restoration.h",
        "spc_snesapu_source_trajectory.h",
        "spc_studio_sample_reconstruction.h",
        "spc_upstream_playback_reconstruction.h",
        "snesapu_studio_source_provider.h",
        "spc_snapshot.h",
        "snesapu_studio_source_packet.h",
        "snesapu_studio_source_packet_runtime.h",
    ):
        shutil.copyfile(project_root / "components" / "spc" / name, overlay / name)

    header = child / "spcplayer.h"
    replace_once(header, "#define SPCP_HEADER_VERSION    1\n", "#define SPCP_HEADER_VERSION    3\n", "SPCP v3 version")
    replace_once(
        header,
        """#define SPCP_HEADER_SIZE       32
""",
        """#define SPCP_HEADER_SIZE       32
#define SPCP_HEADER_PREBRR_SIZE_OFFSET 20
#define SPCP_HEADER_STUDIO_SIZE_OFFSET 24
#define SPCP_HEADER_RESERVED_OFFSET    28
""",
        "SPCP v3 source payload offsets",
    )

    source = child / "main.cpp"
    replace_once(
        source,
        """#include <snesapu.h>
#include "spcplayer.h"
#include "source_capture_output.h"
""",
        """#include <snesapu.h>
#include "spcplayer.h"
#include "source_capture_output.h"
#include "retro_vgm/snesapu_prebrr_packet.h"
#include "retro_vgm/spc_studio_sample_reconstruction.h"
#include "retro_vgm/snesapu_studio_source_packet_runtime.h"
#include <Windows.h>
""",
        "canonical child source-provider includes",
    )

    wrappers = r'''
using RetroPreBrrRuntime = gameaudio::spc::snes_prebrr_packet_runtime<256>;
using RetroPreBrrCallback = u32 (__stdcall *)(void*, u32, u32, s16*);
using RetroSetPreBrrProvider = void (__stdcall *)(RetroPreBrrCallback, void*);

static u32 __stdcall retro_prebrr_callback(
    void* user, u32 source_number, u32 brr_block_address, s16* output16)
{
    return RetroPreBrrRuntime::callback(
        user, source_number, brr_block_address, output16) ? 1u : 0u;
}

static RetroSetPreBrrProvider resolve_prebrr_provider()
{
    HMODULE module = GetModuleHandleW(L"SNESAPU.dll");
    if (!module) module = GetModuleHandleW(L"snesapu.dll");
    return module ? reinterpret_cast<RetroSetPreBrrProvider>(
        GetProcAddress(module, "SetDSPPreBrrProvider")) : nullptr;
}

using RetroStudioRuntime = gameaudio::spc::snes_studio_source_packet_runtime<256>;
using RetroStudioBegin = u32 (__stdcall *)(void*, u32, u32, u32, u32, u32, u32);
using RetroStudioSample = u32 (__stdcall *)(void*, u32, u32, u32, u32, u32, u32, float*);
using RetroSetStudioSourceProvider = void (__stdcall *)(RetroStudioBegin, RetroStudioSample, void*);

static u32 __stdcall retro_studio_begin(
    void* user, u32 voice, u32 source_number, u32 first_brr,
    u32 loop_brr, u32 directory_page, u32 interpolation)
{
    if (!user) return 0u;
    auto* runtime = static_cast<RetroStudioRuntime*>(user);
    return runtime->provider().begin_voice(
        voice, source_number, first_brr, loop_brr, directory_page, interpolation) ? 1u : 0u;
}

static u32 __stdcall retro_studio_sample(
    void* user, u32 voice, u32 m_rate_q16_16, u32 effective_source_number,
    u32 live_loop_brr, u32 directory_page, u32 interpolation, float* output_sample)
{
    if (!user) return 0u;
    auto* runtime = static_cast<RetroStudioRuntime*>(user);
    return runtime->provider().render_voice(
        voice, m_rate_q16_16, effective_source_number, live_loop_brr,
        directory_page, interpolation, output_sample) ? 1u : 0u;
}

static RetroSetStudioSourceProvider resolve_studio_source_provider()
{
    HMODULE module = GetModuleHandleW(L"SNESAPU.dll");
    if (!module) module = GetModuleHandleW(L"snesapu.dll");
    return module ? reinterpret_cast<RetroSetStudioSourceProvider>(
        GetProcAddress(module, "SetDSPStudioSourceProvider")) : nullptr;
}
'''
    replace_once(
        source,
        """static uint32_t get_le32(const uint8_t *p_data)
{
    return (uint32_t)p_data[0] | (((uint32_t)p_data[1]) << 8) |
        (((uint32_t)p_data[2]) << 16) | (((uint32_t)p_data[3]) << 24);
}
""",
        """static uint32_t get_le32(const uint8_t *p_data)
{
    return (uint32_t)p_data[0] | (((uint32_t)p_data[1]) << 8) |
        (((uint32_t)p_data[2]) << 16) | (((uint32_t)p_data[3]) << 24);
}
""" + wrappers,
        "canonical child stdcall provider bridge",
    )

    old_parse = r'''        const uint32_t version = get_le32(spcp_data.data() + 4);
        uint32_t header_size = get_le32(spcp_data.data() + 4 * 2);
        uint32_t spc_size = get_le32(spcp_data.data() + 4 * 3);
        uint32_t script700_size = get_le32(spcp_data.data() + 4 * 4);
        (void)version;

        std::vector<uint8_t> spc_data;
        std::vector<uint8_t> script700_data;

        if (header_size + spc_size > spcp_data.size())
            spc_size = static_cast<uint32_t>(spcp_data.size() - header_size);
        spc_data.resize(spc_size);
        memcpy(spc_data.data(), spcp_data.data() + header_size, spc_size);

        if (script700_size)
        {
            if (header_size + spc_size + script700_size > spcp_data.size())
                script700_size = static_cast<uint32_t>(spcp_data.size() - spc_size - header_size);
            script700_data.resize(script700_size + 1);
            memcpy(script700_data.data(), spcp_data.data() + header_size + spc_size, script700_size);
            script700_data[script700_size] = '\0';
        }
'''
    new_parse = r'''        const uint32_t version = get_le32(spcp_data.data() + 4);
        const uint32_t header_size = get_le32(spcp_data.data() + 8);
        const uint32_t spc_size = get_le32(spcp_data.data() + 12);
        const uint32_t script700_size = get_le32(spcp_data.data() + 16);
        const uint32_t prebrr_size = get_le32(spcp_data.data() + SPCP_HEADER_PREBRR_SIZE_OFFSET);
        const uint32_t studio_source_size = get_le32(spcp_data.data() + SPCP_HEADER_STUDIO_SIZE_OFFSET);
        const uint32_t reserved = get_le32(spcp_data.data() + SPCP_HEADER_RESERVED_OFFSET);
        const uint64_t payload_size = static_cast<uint64_t>(spc_size)
            + static_cast<uint64_t>(script700_size)
            + static_cast<uint64_t>(prebrr_size)
            + static_cast<uint64_t>(studio_source_size);
        if (version != SPCP_HEADER_VERSION || header_size != SPCP_HEADER_SIZE
            || spc_size == 0 || reserved != 0 || header_size > spcp_data.size()
            || payload_size != static_cast<uint64_t>(spcp_data.size() - header_size))
            throw std::runtime_error("invalid SPCP v3 input header");

        std::vector<uint8_t> spc_data(spc_size);
        memcpy(spc_data.data(), spcp_data.data() + header_size, spc_size);
        std::vector<uint8_t> script700_data;
        if (script700_size)
        {
            script700_data.resize(script700_size + 1);
            memcpy(script700_data.data(), spcp_data.data() + header_size + spc_size, script700_size);
            script700_data[script700_size] = '\0';
        }
        const uint8_t* prebrr_data = spcp_data.data() + header_size + spc_size + script700_size;
        const uint8_t* studio_source_data = prebrr_data + prebrr_size;

        RetroPreBrrRuntime prebrr_runtime;
        if (prebrr_size && !prebrr_runtime.load(prebrr_data, prebrr_size))
            throw std::runtime_error("invalid verified pre-BRR packet");
        RetroStudioRuntime studio_runtime;
        if (studio_source_size && !studio_runtime.load(
                studio_source_data, studio_source_size, spc_data.data(), spc_data.size()))
            throw std::runtime_error("invalid verified studio-source packet or BRR witness mismatch");
        if (studio_runtime.loaded())
            gameaudio::spc::prepare_spc_studio_sample_reconstruction();
'''
    replace_once(source, old_parse, new_parse, "canonical child strict SPCP v3 parsing")

    replace_once(
        source,
        """        LoadSPCFile(spc_data.data());
        SetAPUOpt(3, numchn.value(), bits.value(), rate.value(), inter.value(), dspopts.value());
""",
        """        LoadSPCFile(spc_data.data());

        RetroSetPreBrrProvider set_prebrr_provider = resolve_prebrr_provider();
        if (prebrr_runtime.loaded())
        {
            if (!set_prebrr_provider)
                throw std::runtime_error("pre-BRR data supplied but patched SNESAPU provider export is unavailable");
            set_prebrr_provider(&retro_prebrr_callback, &prebrr_runtime);
        }
        else if (set_prebrr_provider)
            set_prebrr_provider(nullptr, nullptr);

        RetroSetStudioSourceProvider set_studio_provider = resolve_studio_source_provider();
        if (studio_runtime.loaded())
        {
            if (!set_studio_provider)
                throw std::runtime_error("studio-source data supplied but patched SNESAPU provider export is unavailable");
            set_studio_provider(&retro_studio_begin, &retro_studio_sample, &studio_runtime);
        }
        else if (set_studio_provider)
            set_studio_provider(nullptr, nullptr, nullptr);

        SetAPUOpt(3, numchn.value(), bits.value(), rate.value(), inter.value(), dspopts.value());
""",
        "canonical child provider installation",
    )

    replace_once(
        source,
        """        if (sources)
            source_api.set_enabled(false);
""",
        """        if (sources)
            source_api.set_enabled(false);
        if (set_studio_provider)
            set_studio_provider(nullptr, nullptr, nullptr);
        if (set_prebrr_provider)
            set_prebrr_provider(nullptr, nullptr);
""",
        "canonical child provider shutdown",
    )

    print("canonical spcplayer SPCP v3 source transport applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
