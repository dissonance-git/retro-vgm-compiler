#!/usr/bin/env python3
"""Carry verified upstream SPC sources into the child-local pInter seam.

Run after:
  apply_prebrr_provider.py
  apply_studio_source_provider.py
  apply_prebrr_transport.py

This upgrades private SPCP v2 to v3. The parent appends an optional
`<track>.studiosrc` packet after the lower-rung pre-BRR payload. The child parses
that packet once, byte-compares every serialized BRR witness against the actual
SPC RAM image, prepares the 64-tap coefficient table outside the audio loop, and
installs only child-local stdcall callbacks.

No file path, corpus lookup, hashing, allocation or IPC occurs in MixSample.
If this top rung is absent or a live callback later fails, the assembly seam
falls through to historical pInter, which can still consume the pre-BRR provider
below it.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil


def decode_source(raw: bytes) -> tuple[str, str, bool]:
    has_utf8_bom = raw.startswith(b"\xef\xbb\xbf")
    try:
        return raw.decode("utf-8-sig"), "utf-8", has_utf8_bom
    except UnicodeDecodeError:
        return raw.decode("cp932"), "cp932", False


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text, encoding, has_utf8_bom = decode_source(raw)
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode(encoding)
    if has_utf8_bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("foo_snesapu_root", type=Path)
    args = parser.parse_args()
    foo_root = args.foo_snesapu_root.resolve()
    parent = foo_root / "foobar2000" / "foo_snesapu"
    child = foo_root / "spcplayer"
    project_root = Path(__file__).resolve().parents[2]

    overlay = child / "retro_vgm"
    overlay.mkdir(parents=True, exist_ok=True)
    for name in (
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
        print(f"staged {name}: {overlay / name}")

    spcplayer_h = child / "spcplayer.h"
    replace_once(
        spcplayer_h,
        "#define SPCP_HEADER_VERSION 2\n",
        "#define SPCP_HEADER_VERSION 3\n",
        "SPCP v3 version",
    )
    replace_once(
        spcplayer_h,
        "#define SPCP_HEADER_PREBRR_SIZE_OFFSET 20\n",
        """#define SPCP_HEADER_PREBRR_SIZE_OFFSET 20
// 24: Verified upstream/studio-source payload size in bytes (uint32 little endian)
#define SPCP_HEADER_STUDIO_SIZE_OFFSET 24
// 28 remains reserved and must be zero.
""",
        "SPCP v3 studio-source field",
    )

    controller_h = parent / "spcplayer_controller.h"
    replace_once(
        controller_h,
        """\tvoid    SetPreBrrPacket(const void *pSource, u32 len);
\tvoid    SetVoiceMute(u32 mute);
""",
        """\tvoid    SetPreBrrPacket(const void *pSource, u32 len);
\tvoid    SetStudioSourcePacket(const void *pSource, u32 len);
\tvoid    SetVoiceMute(u32 mute);
""",
        "parent studio-source setter declaration",
    )
    replace_once(
        controller_h,
        """\tpfc::array_t<t_uint8> m_prebrr_data;
\tt_size\t\t\t\t  m_prebrr_size;


\tbool         initialized;
""",
        """\tpfc::array_t<t_uint8> m_prebrr_data;
\tt_size\t\t\t\t  m_prebrr_size;
\tpfc::array_t<t_uint8> m_studio_source_data;
\tt_size\t\t\t\t  m_studio_source_size;


\tbool         initialized;
""",
        "parent studio-source storage",
    )

    controller_cpp = parent / "spcplayer_controller.cpp"
    replace_once(
        controller_cpp,
        """\tm_spcp_size(0), m_spc_size(0), m_script700_size(0), m_prebrr_size(0),
\tm_telem_enabled(false), m_source_enabled(false),
""",
        """\tm_spcp_size(0), m_spc_size(0), m_script700_size(0), m_prebrr_size(0), m_studio_source_size(0),
\tm_telem_enabled(false), m_source_enabled(false),
""",
        "parent studio-source constructor state",
    )
    replace_once(
        controller_cpp,
        """\tm_spcp_size = SPCP_HEADER_SIZE + m_spc_size + m_script700_size + m_prebrr_size;
""",
        """\tm_spcp_size = SPCP_HEADER_SIZE + m_spc_size + m_script700_size + m_prebrr_size + m_studio_source_size;
""",
        "parent SPCP v3 packet size",
    )
    replace_once(
        controller_cpp,
        """\tset_le32(m_spcp_data.get_ptr() + 20, m_prebrr_size);
\tset_le32(m_spcp_data.get_ptr() + 24, 0);
\tset_le32(m_spcp_data.get_ptr() + 28, 0);
""",
        """\tset_le32(m_spcp_data.get_ptr() + 20, m_prebrr_size);
\tset_le32(m_spcp_data.get_ptr() + 24, m_studio_source_size);
\tset_le32(m_spcp_data.get_ptr() + 28, 0);
""",
        "parent SPCP v3 studio-source header",
    )
    replace_once(
        controller_cpp,
        """\tif (m_prebrr_size)
\t\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE + m_spc_size + m_script700_size, m_prebrr_data.get_ptr(), m_prebrr_size);

\tm_spc_data.force_reset();
\tm_script700_data.force_reset();
\tm_prebrr_data.force_reset();
""",
        """\tif (m_prebrr_size)
\t\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE + m_spc_size + m_script700_size, m_prebrr_data.get_ptr(), m_prebrr_size);
\tif (m_studio_source_size)
\t\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE + m_spc_size + m_script700_size + m_prebrr_size, m_studio_source_data.get_ptr(), m_studio_source_size);

\tm_spc_data.force_reset();
\tm_script700_data.force_reset();
\tm_prebrr_data.force_reset();
\tm_studio_source_data.force_reset();
""",
        "parent SPCP v3 studio-source payload",
    )
    replace_once(
        controller_cpp,
        """void spcplayer_controller::SetPreBrrPacket(const void* pSource, u32 len)
{
\tm_prebrr_data.force_reset();
\tm_prebrr_size = 0;
\tif (pSource == nullptr || len == 0) return;
\tm_prebrr_data.set_size(len);
\tmemcpy(m_prebrr_data.get_ptr(), pSource, len);
\tm_prebrr_size = len;
}

void spcplayer_controller::SetVoiceMute(u32 mute)
""",
        """void spcplayer_controller::SetPreBrrPacket(const void* pSource, u32 len)
{
\tm_prebrr_data.force_reset();
\tm_prebrr_size = 0;
\tif (pSource == nullptr || len == 0) return;
\tm_prebrr_data.set_size(len);
\tmemcpy(m_prebrr_data.get_ptr(), pSource, len);
\tm_prebrr_size = len;
}

void spcplayer_controller::SetStudioSourcePacket(const void* pSource, u32 len)
{
\tm_studio_source_data.force_reset();
\tm_studio_source_size = 0;
\tif (pSource == nullptr || len == 0) return;
\tm_studio_source_data.set_size(len);
\tmemcpy(m_studio_source_data.get_ptr(), pSource, len);
\tm_studio_source_size = len;
}

void spcplayer_controller::SetVoiceMute(u32 mute)
""",
        "parent studio-source packet setter",
    )

    child_cpp = child / "main.cpp"
    replace_once(
        child_cpp,
        """#include "retro_vgm/snesapu_prebrr_packet.h"
#include <windows.h>
""",
        """#include "retro_vgm/snesapu_prebrr_packet.h"
#include "retro_vgm/snesapu_studio_source_packet_runtime.h"
#include <windows.h>
""",
        "child studio-source runtime include",
    )
    replace_once(
        child_cpp,
        """static RetroSetPreBrrProvider resolve_prebrr_provider()
{
    HMODULE module = GetModuleHandleW(L"snesapu.dll");
    if (module == NULL) return nullptr;
    return reinterpret_cast<RetroSetPreBrrProvider>(
        GetProcAddress(module, "SetDSPPreBrrProvider"));
}
""",
        """static RetroSetPreBrrProvider resolve_prebrr_provider()
{
    HMODULE module = GetModuleHandleW(L"snesapu.dll");
    if (module == NULL) return nullptr;
    return reinterpret_cast<RetroSetPreBrrProvider>(
        GetProcAddress(module, "SetDSPPreBrrProvider"));
}

using RetroStudioRuntime = gameaudio::spc::snes_studio_source_packet_runtime<256>;
using RetroStudioBeginCallback = u32 (__stdcall *)(void*, u32, u32, u32, u32, u32, u32);
using RetroStudioSampleCallback = u32 (__stdcall *)(void*, u32, u32, u32, u32, float*);
using RetroSetStudioSourceProvider = void (__stdcall *)(
    RetroStudioBeginCallback, RetroStudioSampleCallback, void*);

static u32 __stdcall retro_studio_begin(
    void* user, u32 voice, u32 srcn, u32 first_brr, u32 loop_brr,
    u32 directory_page, u32 interpolation)
{
    if (user == nullptr) return 0u;
    auto* runtime = static_cast<RetroStudioRuntime*>(user);
    return runtime->provider().begin_voice(
        voice, srcn, first_brr, loop_brr, directory_page, interpolation) ? 1u : 0u;
}

static u32 __stdcall retro_studio_sample(
    void* user, u32 voice, u32 m_rate_q16_16, u32 directory_page,
    u32 interpolation, float* out_sample)
{
    if (user == nullptr) return 0u;
    auto* runtime = static_cast<RetroStudioRuntime*>(user);
    return runtime->provider().render_voice(
        voice, m_rate_q16_16, directory_page, interpolation, out_sample) ? 1u : 0u;
}

static RetroSetStudioSourceProvider resolve_studio_source_provider()
{
    HMODULE module = GetModuleHandleW(L"snesapu.dll");
    if (module == NULL) return nullptr;
    return reinterpret_cast<RetroSetStudioSourceProvider>(
        GetProcAddress(module, "SetDSPStudioSourceProvider"));
}
""",
        "child studio-source callbacks/runtime resolver",
    )

    replace_once(
        child_cpp,
        """    const uint32_t prebrr_size = get_le32(&spcbuf[20]);
    const uint32_t reserved2 = get_le32(&spcbuf[24]);
    const uint32_t reserved3 = get_le32(&spcbuf[28]);
""",
        """    const uint32_t prebrr_size = get_le32(&spcbuf[20]);
    const uint32_t studio_source_size = get_le32(&spcbuf[24]);
    const uint32_t reserved3 = get_le32(&spcbuf[28]);
""",
        "child SPCP v3 studio-source field",
    )
    replace_once(
        child_cpp,
        """        + static_cast<uint64_t>(script700_size)
        + static_cast<uint64_t>(prebrr_size);
""",
        """        + static_cast<uint64_t>(script700_size)
        + static_cast<uint64_t>(prebrr_size)
        + static_cast<uint64_t>(studio_source_size);
""",
        "child SPCP v3 payload size",
    )
    replace_once(
        child_cpp,
        """        || reserved2 != 0 || reserved3 != 0
""",
        """        || reserved3 != 0
""",
        "child SPCP v3 reserved field",
    )
    replace_once(
        child_cpp,
        """        std::cerr << "Invalid SPCP v2 input header.\\n";
""",
        """        std::cerr << "Invalid SPCP v3 input header.\\n";
""",
        "child SPCP v3 error text",
    )
    replace_once(
        child_cpp,
        """    const uint8_t* prebrr_data = script700_data + script700_size;

    RetroPreBrrRuntime prebrr_runtime;
""",
        """    const uint8_t* prebrr_data = script700_data + script700_size;
    const uint8_t* studio_source_data = prebrr_data + prebrr_size;

    RetroPreBrrRuntime prebrr_runtime;
""",
        "child studio-source payload pointer",
    )
    replace_once(
        child_cpp,
        """    if (prebrr_size != 0 && !prebrr_runtime.load(prebrr_data, prebrr_size))
    {
        std::cerr << "Invalid verified pre-BRR packet.\\n";
        return -1;
    }

    InitAPU();
""",
        """    if (prebrr_size != 0 && !prebrr_runtime.load(prebrr_data, prebrr_size))
    {
        std::cerr << "Invalid verified pre-BRR packet.\\n";
        return -1;
    }

    RetroStudioRuntime studio_runtime;
    if (studio_source_size != 0
        && !studio_runtime.load(studio_source_data, studio_source_size, spc_data, spc_size))
    {
        std::cerr << "Invalid verified studio-source packet or BRR witness mismatch.\\n";
        return -1;
    }
    if (studio_runtime.loaded())
        gameaudio::spc::prepare_spc_studio_sample_reconstruction();

    InitAPU();
""",
        "child studio-source packet admission",
    )
    replace_once(
        child_cpp,
        """    else if (set_prebrr_provider != nullptr)
    {
        set_prebrr_provider(nullptr, nullptr);
    }
""",
        """    else if (set_prebrr_provider != nullptr)
    {
        set_prebrr_provider(nullptr, nullptr);
    }

    RetroSetStudioSourceProvider set_studio_source_provider =
        resolve_studio_source_provider();
    if (studio_runtime.loaded())
    {
        if (set_studio_source_provider == nullptr)
        {
            std::cerr << "Studio-source data supplied but patched SNESAPU provider export is unavailable.\\n";
            return -1;
        }
        set_studio_source_provider(
            &retro_studio_begin, &retro_studio_sample, &studio_runtime);
    }
    else if (set_studio_source_provider != nullptr)
    {
        set_studio_source_provider(nullptr, nullptr, nullptr);
    }
""",
        "child studio-source provider installation",
    )

    input_cpp = parent / "input_snesapu.cpp"
    replace_once(
        input_cpp,
        """\t\tcatch (const exception_io_not_found&)
\t\t{
\t\t\t// Missing verified source is normal: use high-rate BRR Enhanced fallback.
\t\t}
\t}
\tm_Apu.SetVoiceMute(m_TagMask);
""",
        """\t\tcatch (const exception_io_not_found&)
\t\t{
\t\t\t// Missing verified source is normal: use high-rate BRR Enhanced fallback.
\t\t}
\t}

\tm_Apu.SetStudioSourcePacket(nullptr, 0);
\tif (cfg_enhanced_enabled && !filesystem::g_is_remote_or_unrecognized(m_SpcPath.get_ptr()))
\t{
\t\tpfc::string8 sidecar = m_SpcPath;
\t\tsidecar += ".studiosrc";
\t\ttry
\t\t{
\t\t\tservice_ptr_t<file> studio_file;
\t\t\tfilesystem::g_open_read(studio_file, sidecar, p_abort);
\t\t\tconst t_filesize size64 = studio_file->get_size(p_abort);
\t\t\tif (size64 != filesize_invalid && size64 > 0 && size64 <= 256u * 1024u * 1024u)
\t\t\t{
\t\t\t\tpfc::array_t<t_uint8> studio_packet;
\t\t\t\tconst t_size size = pfc::downcast_guarded<t_size>(size64);
\t\t\t\tstudio_packet.set_size(size);
\t\t\t\tstudio_file->read_object(studio_packet.get_ptr(), size, p_abort);
\t\t\t\tm_Apu.SetStudioSourcePacket(studio_packet.get_ptr(), static_cast<u32>(size));
\t\t\t}
\t\t}
\t\tcatch (const exception_io_not_found&)
\t\t{
\t\t\t// Missing top-rung source is normal: the pre-BRR/BRR ladder remains live.
\t\t}
\t}
\tm_Apu.SetVoiceMute(m_TagMask);
""",
        "SNES Enhanced studio-source sidecar discovery",
    )

    print("foo_snesapu SPCP v3 studio-source transport applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
