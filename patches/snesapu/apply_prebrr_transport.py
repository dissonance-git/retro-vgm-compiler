#!/usr/bin/env python3
"""Carry verified pre-BRR game-grid PCM from foobar into the spcplayer child.

Target: historical foo_snesapu tree containing both:
  foo_snesapu/foobar2000/foo_snesapu
  foo_snesapu/spcplayer

The parent discovers an optional `<track>.prebrr` sidecar only while Enhanced is
active, validates its bounded size, then appends it to SPCP protocol v2. The
child parses it once, owns the PCM for the process lifetime, and installs the
SNESAPU block provider added by apply_prebrr_provider.py.

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

    # Copy project-owned header-only runtime into the child build. The source of
    # truth remains this repository; the dependency checkout receives a build
    # overlay, not a forked independent implementation.
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
        """#define SPCP_HEADER_SCRIPT700_SIZE_OFFSET 16
#define SPCP_HEADER_RESERVED1_OFFSET 20
""",
        """#define SPCP_HEADER_SCRIPT700_SIZE_OFFSET 16
#define SPCP_HEADER_PREBRR_SIZE_OFFSET 20
""",
        "SPCP pre-BRR size field",
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
""",
        """\tpfc::array_t<t_uint8> m_script700_data;
\tt_size\t\t\t\t  m_script700_size;
\tpfc::array_t<t_uint8> m_prebrr_data;
\tt_size\t\t\t\t  m_prebrr_size;
""",
        "parent pre-BRR packet storage",
    )

    controller_cpp = parent / "spcplayer_controller.cpp"
    replace_once(
        controller_cpp,
        """\tm_spcp_size(0), m_spc_size(0), m_script700_size(0),
""",
        """\tm_spcp_size(0), m_spc_size(0), m_script700_size(0), m_prebrr_size(0),
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
        """#include "source_capture_output.h"
""",
        """#include "source_capture_output.h"
#include "retro_vgm/snesapu_prebrr_packet.h"
""",
        "child pre-BRR runtime include",
    )
    replace_once(
        child_cpp,
        """static uint32_t m_sample_rate = 32000;
""",
        """static uint32_t m_sample_rate = 32000;

using RetroPreBrrRuntime = gameaudio::spc::snes_prebrr_packet_runtime<256>;
static int __stdcall retro_prebrr_callback(void* user, u32 srcn, u32 brr_addr, s16* out16)
{
    return RetroPreBrrRuntime::callback(user, srcn, brr_addr, out16);
}
""",
        "child pre-BRR callback wrapper",
    )
    replace_once(
        child_cpp,
        """    uint32_t spc_size = spcp_read_le32(header + SPCP_HEADER_SPC_SIZE_OFFSET);
    uint32_t script700_size = spcp_read_le32(header + SPCP_HEADER_SCRIPT700_SIZE_OFFSET);
    std::vector<uint8_t> spcbuf(static_cast<size_t>(spc_size) + script700_size);
""",
        """    uint32_t spc_size = spcp_read_le32(header + SPCP_HEADER_SPC_SIZE_OFFSET);
    uint32_t script700_size = spcp_read_le32(header + SPCP_HEADER_SCRIPT700_SIZE_OFFSET);
    uint32_t prebrr_size = spcp_read_le32(header + SPCP_HEADER_PREBRR_SIZE_OFFSET);
    if (spcp_read_le32(header + 24) != 0 || spcp_read_le32(header + 28) != 0)
    {
        std::cerr << "spcplayer: unsupported nonzero SPCP v2 reserved fields\n";
        return -1;
    }
    const uint64_t payload_size64 = static_cast<uint64_t>(spc_size)
        + static_cast<uint64_t>(script700_size)
        + static_cast<uint64_t>(prebrr_size);
    if (payload_size64 > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
    {
        std::cerr << "spcplayer: SPCP payload too large\n";
        return -1;
    }
    std::vector<uint8_t> spcbuf(static_cast<size_t>(payload_size64));
""",
        "child SPCP v2 payload size",
    )
    replace_once(
        child_cpp,
        """    InitAPU();
    LoadSPCFile(spcbuf.data());
    if (script700_size > 0)
""",
        """    const uint8_t* prebrr_data = spcbuf.data()
        + static_cast<size_t>(spc_size)
        + static_cast<size_t>(script700_size);
    RetroPreBrrRuntime prebrr_runtime;
    if (prebrr_size != 0
        && !prebrr_runtime.load(prebrr_data, static_cast<size_t>(prebrr_size)))
    {
        std::cerr << "spcplayer: invalid pre-BRR restoration packet\n";
        return -1;
    }

    InitAPU();
    LoadSPCFile(spcbuf.data());
    if (prebrr_runtime.loaded())
        SetDSPPreBrrProvider(&retro_prebrr_callback, &prebrr_runtime);
    else
        SetDSPPreBrrProvider(NULL, NULL);
    if (script700_size > 0)
""",
        "child pre-BRR provider installation",
    )

    # The packet-size guard uses numeric_limits.
    replace_once(
        child_cpp,
        """#include <vector>
#include <cstring>
""",
        """#include <vector>
#include <cstring>
#include <limits>
""",
        "child payload overflow include",
    )

    input_cpp = parent / "input_snesapu.cpp"
    # Load only a per-track verified sidecar and only when Enhanced is active.
    # Missing sidecar is normal and leaves the high-rate BRR/sinc path active.
    replace_once(
        input_cpp,
        """\tm_Apu.SetDSPAmp(m_CurAmp);

#ifdef _WIN64
\tm_Apu.SetVoiceMute(m_TagMask);
""",
        """\tm_Apu.SetDSPAmp(m_CurAmp);

#ifdef _WIN64
\tm_Apu.SetPreBrrPacket(nullptr, 0);
\tif (cfg_enhanced_enabled && !filesystem::g_is_remote_or_unrecognized(m_SpcPath))
\t{
\t\tpfc::string8 sidecar = m_SpcPath;
\t\tsidecar += ".prebrr";
\t\ttry
\t\t{
\t\t\tfile::ptr restoration_file;
\t\t\tfilesystem::g_open_read(restoration_file, sidecar, p_abort);
\t\t\tconst t_filesize size64 = restoration_file->get_size(p_abort);
\t\t\t// Per-track prepared PCM is bounded to 64 MiB. Larger files are
\t\t\t// almost certainly malformed or the wrong archival object.
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
\t\t\t// No verified upstream source for this track: normal BRR Enhanced fallback.
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
