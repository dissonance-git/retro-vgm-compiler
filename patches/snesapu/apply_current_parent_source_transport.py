#!/usr/bin/env python3
"""Upgrade the audited foo_snesapu parent to SPCP v3 source transport.

This is the active parent-side counterpart to apply_current_child_source_transport.py.
It carries optional verified pre-BRR and exact-upstream source packets only while
enhanced playback is active. Missing sidecars are normal and preserve the lower
fallback rung. Historical child-migration scripts are intentionally not invoked.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def decode_source(raw: bytes) -> tuple[str, str, bool]:
    bom = raw.startswith(b"\xef\xbb\xbf")
    try:
        return raw.decode("utf-8-sig"), "utf-8", bom
    except UnicodeDecodeError:
        return raw.decode("cp932"), "cp932", False


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text, encoding, bom = decode_source(raw)
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode(encoding)
    if bom:
        encoded = b"\xef\xbb\xbf" + encoded
    path.write_bytes(encoded)
    print(f"patched {label}: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("foo_snesapu_root", type=Path)
    root = parser.parse_args().foo_snesapu_root.resolve()
    parent = root / "foobar2000" / "foo_snesapu"
    header = parent / "spcplayer_controller.h"
    source = parent / "spcplayer_controller.cpp"
    input_cpp = parent / "input_snesapu.cpp"

    replace_once(
        header,
        """\tu32\t\tSetScript700(void *pSource, u32 len);
\tvoid    SetVoiceMute(u32 mute);
""",
        """\tu32\t\tSetScript700(void *pSource, u32 len);
\tvoid    SetPreBrrPacket(const void *pSource, u32 len);
\tvoid    SetStudioSourcePacket(const void *pSource, u32 len);
\tvoid    SetVoiceMute(u32 mute);
""",
        "parent source packet setters",
    )
    replace_once(
        header,
        """\tpfc::array_t<t_uint8> m_script700_data;
\tt_size\t\t\t\t  m_script700_size;


\tbool         initialized;
""",
        """\tpfc::array_t<t_uint8> m_script700_data;
\tt_size\t\t\t\t  m_script700_size;
\tpfc::array_t<t_uint8> m_prebrr_data;
\tt_size\t\t\t\t  m_prebrr_size;
\tpfc::array_t<t_uint8> m_studio_source_data;
\tt_size\t\t\t\t  m_studio_source_size;


\tbool         initialized;
""",
        "parent source packet storage",
    )
    replace_once(
        source,
        """\tm_spcp_size(0), m_spc_size(0), m_script700_size(0),
\tm_telem_enabled(false), m_source_enabled(false),
""",
        """\tm_spcp_size(0), m_spc_size(0), m_script700_size(0), m_prebrr_size(0), m_studio_source_size(0),
\tm_telem_enabled(false), m_source_enabled(false),
""",
        "parent source constructor state",
    )
    replace_once(
        source,
        """void spcplayer_controller::end_decode_initialization()
{
\tm_spcp_size = SPCP_HEADER_SIZE + m_spc_size + m_script700_size;
\tm_spcp_data.set_size(m_spcp_size);
""",
        """void spcplayer_controller::end_decode_initialization()
{
\tm_spcp_size = SPCP_HEADER_SIZE + m_spc_size + m_script700_size + m_prebrr_size + m_studio_source_size;
\tm_spcp_data.set_size(m_spcp_size);
""",
        "parent SPCP v3 packet size",
    )
    replace_once(
        source,
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
\tset_le32(m_spcp_data.get_ptr() + 24, m_studio_source_size);
\tset_le32(m_spcp_data.get_ptr() + 28, 0);
\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE, m_spc_data.get_ptr(), m_spc_size);
\tif (m_script700_size)
\t\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE + m_spc_size, m_script700_data.get_ptr(), m_script700_size);
\tif (m_prebrr_size)
\t\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE + m_spc_size + m_script700_size, m_prebrr_data.get_ptr(), m_prebrr_size);
\tif (m_studio_source_size)
\t\tmemcpy(m_spcp_data.get_ptr() + SPCP_HEADER_SIZE + m_spc_size + m_script700_size + m_prebrr_size, m_studio_source_data.get_ptr(), m_studio_source_size);

\tm_spc_data.force_reset();
\tm_script700_data.force_reset();
\tm_prebrr_data.force_reset();
\tm_studio_source_data.force_reset();
""",
        "parent SPCP v3 source payload",
    )
    replace_once(
        source,
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
        "parent source packet setters",
    )

    replace_once(
        input_cpp,
        """\tm_Apu.SetDSPAmp(m_CurAmp);

#ifdef _WIN64
\tm_Apu.SetVoiceMute(m_TagMask);
""",
        """\tm_Apu.SetDSPAmp(m_CurAmp);

#ifdef _WIN64
\tm_Apu.SetPreBrrPacket(nullptr, 0);
\tm_Apu.SetStudioSourcePacket(nullptr, 0);
\tif (cfg_enhanced_enabled && !filesystem::g_is_remote_or_unrecognized(m_SpcPath.get_ptr()))
\t{
\t\tpfc::string8 prebrr_sidecar = m_SpcPath;
\t\tprebrr_sidecar += ".prebrr";
\t\ttry
\t\t{
\t\t\tservice_ptr_t<file> restoration_file;
\t\t\tfilesystem::g_open_read(restoration_file, prebrr_sidecar, p_abort);
\t\t\tconst t_filesize size64 = restoration_file->get_size(p_abort);
\t\t\tif (size64 != filesize_invalid && size64 > 0 && size64 <= 64u * 1024u * 1024u)
\t\t\t{
\t\t\t\tpfc::array_t<t_uint8> packet;
\t\t\t\tconst t_size size = pfc::downcast_guarded<t_size>(size64);
\t\t\t\tpacket.set_size(size);
\t\t\t\trestoration_file->read_object(packet.get_ptr(), size, p_abort);
\t\t\t\tm_Apu.SetPreBrrPacket(packet.get_ptr(), static_cast<u32>(size));
\t\t\t}
\t\t}
\t\tcatch (const exception_io_not_found&) {}

\t\tpfc::string8 studio_sidecar = m_SpcPath;
\t\tstudio_sidecar += ".studiosrc";
\t\ttry
\t\t{
\t\t\tservice_ptr_t<file> studio_file;
\t\t\tfilesystem::g_open_read(studio_file, studio_sidecar, p_abort);
\t\t\tconst t_filesize size64 = studio_file->get_size(p_abort);
\t\t\tif (size64 != filesize_invalid && size64 > 0 && size64 <= 256u * 1024u * 1024u)
\t\t\t{
\t\t\t\tpfc::array_t<t_uint8> packet;
\t\t\t\tconst t_size size = pfc::downcast_guarded<t_size>(size64);
\t\t\t\tpacket.set_size(size);
\t\t\t\tstudio_file->read_object(packet.get_ptr(), size, p_abort);
\t\t\t\tm_Apu.SetStudioSourcePacket(packet.get_ptr(), static_cast<u32>(size));
\t\t\t}
\t\t}
\t\tcatch (const exception_io_not_found&) {}
\t}
\tm_Apu.SetVoiceMute(m_TagMask);
""",
        "SNES enhanced source sidecar discovery",
    )

    print("foo_snesapu parent SPCP v3 source transport applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
