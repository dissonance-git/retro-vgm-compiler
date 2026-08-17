#!/usr/bin/env python3
"""Carry verified pre-BRR game-grid PCM from foobar into the spcplayer child.

Target: historical foo_snesapu tree containing both:
  foo_snesapu/foobar2000/foo_snesapu
  foo_snesapu/spcplayer

The parent discovers an optional `<track>.prebrr` sidecar only while Enhanced is
active and appends it to private SPCP protocol v2. The child validates and owns
that PCM once at startup, then resolves the patched SNESAPU provider export at
runtime. No new import-library dependency is required in the child build.

No corpus/library path crosses the realtime boundary. The sidecar contains only
already-verified, historically prepared 16-bit game-grid samples for this SPC.
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
    for name in ("snesapu_prebrr_provider.h", "snesapu_prebrr_packet.h"):
        shutil.copyfile(project_root / "components" / "spc" / name, overlay / name)
        print(f"staged {name}: {overlay / name}")

    spcplayer_h = child / "spcplayer.h"
    replace_once(
        spcplayer_h,
        """#define SPCP_HEADER_VERSION 1
""",
        """#define SPCP_HEADER_VERSION 2
""",
        "SPCP v2 version",
    )
    replace_once(
        spcplayer_h,
        """// 16: Script700 payload size in bytes (uint32 little endian)
// 20: Reserved1 (uint32 little endian, must be zero)
""",
        """// 16: Script700 payload size in bytes (uint32 little endian)
// 20: Verified pre-BRR payload size in bytes (uint32 little endian)
#define SPCP_HEADER_PREBRR_SIZE_OFFSET 20
""",
        "SPCP v2 pre-BRR field",
    )

    controller_h = parent / "spcplayer_controller.h"
    replace_once(
        controller_h,
        """\tu32\t\tSetScript700(void *pSource, u32 len);
\tvoid    SetVoiceMute(u32 mute);
""",
        """\tu32\t\tSetScript700(void *pSource, u32 len);
\tvoid    SetPreBrrPacket(const void *pSource, u32 len);
\tvoid    SetVoiceMute(u32 mute);
""",
        "parent pre-BRR packet setter declaration",
    )
    replace_once(
        controller_h,
        """\tpfc::array_t<t_uint8> m_script700_data;
\tt_size\t\t\t\t  m_script700_size;


\tbool         initialized;
""",
        """\tpfc::array_t<t_uint8> m_script700_data;
\tt_size\t\t\t\t  m_script700_size;
\tpfc::array_t<t_uint8> m_prebrr_data;
\tt_size\t\t\t\t  m_prebrr_size;


\tbool         initialized;
""",
        "parent pre-BRR packet storage",
    )

    controller_cpp = parent / "spcplayer_controller.cpp"
    replace_once(
        controller_cpp,
        """\tm_spcp_size(0), m_spc_size(0), m_script700_size(0),
\tm_telem_enabled(false), m_source_enabled(false),
""",
        """\tm_spcp_size(0), m_spc_size(0), m_script700_size(0), m_prebrr_size(0),
\tm_telem_enabled(false), m_source_enabled(false),
""",
        "parent pre-BRR constructor state",
    )
    replace_once(
        controller_cpp,
        """void spcplayer_controller::end_decode_initialization()
{
\tm_spcp_size = SPCP_HEADER_SIZE + m_spc_size + m_script700_size;
\tm_spcp_data.set_size(m_spcp_size);
""",
        """void spcplayer_controller::end_decode_initialization()
{
\tm_spcp_size = SPCP_HEADER_SIZE + m_spc_size + m_script700_size + m_prebrr_size;
\tm_spcp_data.set_size(m_spcp_size);
""",
        "parent SPCP v2 packet size",
    )
    replace_once(
        controller_cpp,
        """\tset_le32(m_spcp_data.get_ptr() + 12, m_spc_size);
\tset_le32(m_spcp_data.get_ptr() + 16, m_script700_size);
\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE, m_spc_data.get_ptr(), m_spc_size);
\tif (m_script700_size)
\t\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE + m_spc_size, m_script700_data.get_ptr(), m_script700_size);

\tm_spc_data.force_reset();
\tm_script700_data.force_reset();
""",
        """\tset_le32(m_spcp_data.get_ptr() + 12, m_spc_size);
\tset_le32(m_spcp_data.get_ptr() + 16, m_script700_size);
\tset_le32(m_spcp_data.get_ptr() + 20, m_prebrr_size);
\tset_le32(m_spcp_data.get_ptr() + 24, 0);
\tset_le32(m_spcp_data.get_ptr() + 28, 0);
\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE, m_spc_data.get_ptr(), m_spc_size);
\tif (m_script700_size)
\t\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE + m_spc_size, m_script700_data.get_ptr(), m_script700_size);
\tif (m_prebrr_size)
\t\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE + m_spc_size + m_script700_size, m_prebrr_data.get_ptr(), m_prebrr_size);

\tm_spc_data.force_reset();
\tm_script700_data.force_reset();
\tm_prebrr_data.force_reset();
""",
        "parent SPCP v2 pre-BRR payload",
    )
    replace_once(
        controller_cpp,
        """u32 spcplayer_controller::SetScript700(void* pSource, u32 len)
{
\tm_script700_data.set_size(len);
\tmemcpy(m_script700_data.get_ptr(), pSource, len);
\tm_script700_size = len;
\treturn 1;
}

void spcplayer_controller::SetVoiceMute(u32 mute)
""",
        """u32 spcplayer_controller::SetScript700(void* pSource, u32 len)
{
\tm_script700_data.set_size(len);
\tmemcpy(m_script700_data.get_ptr(), pSource, len);
\tm_script700_size = len;
\treturn 1;
}

void spcplayer_controller::SetPreBrrPacket(const void* pSource, u32 len)
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
        "parent pre-BRR packet setter",
    )

    child_cpp = child / "main.cpp"
    replace_once(
        child_cpp,
        """#include <snesapu.h>
#include "source_capture_output.h"
""",
        """#include <snesapu.h>
#include "source_capture_output.h"
#include "retro_vgm/snesapu_prebrr_packet.h"
#include <windows.h>
""",
        "child pre-BRR runtime include",
    )
    replace_once(
        child_cpp,
        """static uint32_t m_sample_rate = 32000;
""",
        """static uint32_t m_sample_rate = 32000;

using RetroPreBrrRuntime = gameaudio::spc::snes_prebrr_packet_runtime<256>;
using RetroPreBrrCallback = u32 (__stdcall *)(void*, u32, u32, s16*);
using RetroSetPreBrrProvider = void (__stdcall *)(RetroPreBrrCallback, void*);

static u32 __stdcall retro_prebrr_callback(void* user, u32 srcn, u32 brr_addr, s16* out16)
{
    return RetroPreBrrRuntime::callback(user, srcn, brr_addr, out16) ? 1u : 0u;
}

static RetroSetPreBrrProvider resolve_prebrr_provider()
{
    HMODULE module = GetModuleHandleW(L"snesapu.dll");
    if (module == NULL) return nullptr;
    return reinterpret_cast<RetroSetPreBrrProvider>(
        GetProcAddress(module, "SetDSPPreBrrProvider"));
}
""",
        "child pre-BRR callback/runtime resolver",
    )

    replace_once(
        child_cpp,
        """    const uint32_t magic = get_le32(&spcbuf[0]);
    const uint32_t version = get_le32(&spcbuf[4]);
    const uint32_t header_size = get_le32(&spcbuf[8]);
    const uint32_t spc_size = get_le32(&spcbuf[12]);
    const uint32_t script700_size = get_le32(&spcbuf[16]);
    (void)version;
    if (magic != SPCP_HEADER_MAGIC || header_size < SPCP_HEADER_SIZE || spc_size == 0 ||
        header_size > spcbuf.size() || spc_size > spcbuf.size() - header_size ||
        script700_size > spcbuf.size() - header_size - spc_size)
    {
        std::cerr << "Invalid input header.\n";
        return -1;
    }

    const uint8_t* spc_data = &spcbuf[header_size];
    const uint8_t* script700_data = spc_data + spc_size;

    InitAPU();
    LoadSPCFile((void*)spc_data);
""",
        """    const uint32_t magic = get_le32(&spcbuf[0]);
    const uint32_t version = get_le32(&spcbuf[4]);
    const uint32_t header_size = get_le32(&spcbuf[8]);
    const uint32_t spc_size = get_le32(&spcbuf[12]);
    const uint32_t script700_size = get_le32(&spcbuf[16]);
    const uint32_t prebrr_size = get_le32(&spcbuf[20]);
    const uint32_t reserved2 = get_le32(&spcbuf[24]);
    const uint32_t reserved3 = get_le32(&spcbuf[28]);
    const uint64_t payload_size = static_cast<uint64_t>(spc_size)
        + static_cast<uint64_t>(script700_size)
        + static_cast<uint64_t>(prebrr_size);
    if (magic != SPCP_HEADER_MAGIC || version != SPCP_HEADER_VERSION
        || header_size != SPCP_HEADER_SIZE || spc_size == 0
        || reserved2 != 0 || reserved3 != 0
        || header_size > spcbuf.size()
        || payload_size != static_cast<uint64_t>(spcbuf.size() - header_size))
    {
        std::cerr << "Invalid SPCP v2 input header.\n";
        return -1;
    }

    const uint8_t* spc_data = &spcbuf[header_size];
    const uint8_t* script700_data = spc_data + spc_size;
    const uint8_t* prebrr_data = script700_data + script700_size;

    RetroPreBrrRuntime prebrr_runtime;
    if (prebrr_size != 0 && !prebrr_runtime.load(prebrr_data, prebrr_size))
    {
        std::cerr << "Invalid verified pre-BRR packet.\n";
        return -1;
    }

    InitAPU();
    LoadSPCFile((void*)spc_data);
    RetroSetPreBrrProvider set_prebrr_provider = resolve_prebrr_provider();
    if (prebrr_runtime.loaded())
    {
        if (set_prebrr_provider == nullptr)
        {
            std::cerr << "Pre-BRR data supplied but patched SNESAPU provider export is unavailable.\n";
            return -1;
        }
        set_prebrr_provider(&retro_prebrr_callback, &prebrr_runtime);
    }
    else if (set_prebrr_provider != nullptr)
    {
        set_prebrr_provider(nullptr, nullptr);
    }
""",
        "child strict SPCP v2/pre-BRR parsing",
    )

    input_cpp = parent / "input_snesapu.cpp"
    replace_once(
        input_cpp,
        """\tm_Apu.SetDSPAmp(m_CurAmp);

#ifdef _WIN64
\tm_Apu.SetVoiceMute(m_TagMask);
""",
        """\tm_Apu.SetDSPAmp(m_CurAmp);

#ifdef _WIN64
\tm_Apu.SetPreBrrPacket(nullptr, 0);
\tif (cfg_enhanced_enabled && !filesystem::g_is_remote_or_unrecognized(m_SpcPath.get_ptr()))
\t{
\t\tpfc::string8 sidecar = m_SpcPath;
\t\tsidecar += ".prebrr";
\t\ttry
\t\t{
\t\t\tservice_ptr_t<file> restoration_file;
\t\t\tfilesystem::g_open_read(restoration_file, sidecar, p_abort);
\t\t\tconst t_filesize size64 = restoration_file->get_size(p_abort);
\t\t\tif (size64 != filesize_invalid && size64 > 0 && size64 <= 64u * 1024u * 1024u)
\t\t\t{
\t\t\t\tpfc::array_t<t_uint8> restoration_packet;
\t\t\t\tconst t_size size = pfc::downcast_guarded<t_size>(size64);
\t\t\t\trestoration_packet.set_size(size);
\t\t\t\trestoration_file->read_object(restoration_packet.get_ptr(), size, p_abort);
\t\t\t\tm_Apu.SetPreBrrPacket(restoration_packet.get_ptr(), static_cast<u32>(size));
\t\t\t}
\t\t}
\t\tcatch (const exception_io_not_found&)
\t\t{
\t\t\t// Missing verified source is normal: use high-rate BRR Enhanced fallback.
\t\t}
\t}
\tm_Apu.SetVoiceMute(m_TagMask);
""",
        "SNES Enhanced pre-BRR sidecar discovery",
    )

    print("foo_snesapu SPCP v2 pre-BRR transport applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
