#!/usr/bin/env python3
"""Restore the historical SNESAPU Surround checkbox as the spatial switch.

Runs immediately after apply_enhanced_ui.py. The temporary semantic-7.1/spatial
preference is removed; IDC_DSP_SURROUND and DSP_SURND return to the ordinary DSP
option table, so the existing guid_options/cfg_dsp_option state owns persistence.
enhanced remains a separate x64 preference.
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
    parser.add_argument(
        "source_dir",
        type=Path,
        help="foo_snesapu/foobar2000/foo_snesapu source directory",
    )
    root = parser.parse_args().source_dir.resolve()
    prefs = root / "preferences_snesapu.cpp"
    resource_h = root / "resource.h"
    resource_rc = root / "resource.rc"
    input_cpp = root / "input_snesapu.cpp"

    replace_once(
        resource_h,
        """#define IDC_SEM71_ENABLED               1029
#define IDC_ENHANCED_ENABLED            1030
""",
        """#define IDC_ENHANCED_ENABLED            1030
""",
        "remove duplicate SNES spatial control id",
    )

    replace_once(
        resource_rc,
        """    CONTROL         "Spatial",IDC_SEM71_ENABLED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,170,65,60,15
    CONTROL         "enhanced",IDC_ENHANCED_ENABLED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,240,65,70,15
""",
        """    CONTROL         "Surround",IDC_DSP_SURROUND,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,170,65,145,15
    CONTROL         "enhanced",IDC_ENHANCED_ENABLED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,170,143,70,15
""",
        "restore SNES Surround UI slot",
    )

    replace_once(
        prefs,
        """#ifdef _WIN64
static const GUID guid_sem71_enabled =
{ 0xb7c3a912, 0x6f11, 0x4d8e, { 0x8a, 0x15, 0xc2, 0x3d, 0x67, 0xf9, 0x01, 0xae } };
static const GUID guid_enhanced_enabled =
{ 0x47f27b61, 0x8ac2, 0x4f30, { 0xb9, 0x11, 0x72, 0xd8, 0x91, 0x51, 0x5a, 0x03 } };
#endif
""",
        """#ifdef _WIN64
static const GUID guid_enhanced_enabled =
{ 0x47f27b61, 0x8ac2, 0x4f30, { 0xb9, 0x11, 0x72, 0xd8, 0x91, 0x51, 0x5a, 0x03 } };
#endif
""",
        "remove SNES duplicate spatial preference GUID",
    )

    replace_once(
        prefs,
        """#ifdef _WIN64
cfg_int cfg_sem71_enabled             (guid_sem71_enabled,             1);  // preserve existing Spatial preference
cfg_int cfg_enhanced_enabled          (guid_enhanced_enabled,          0);  // protected reference remains default
#endif
""",
        """#ifdef _WIN64
cfg_int cfg_enhanced_enabled          (guid_enhanced_enabled,          0);  // protected reference remains default
#endif
""",
        "remove SNES duplicate spatial cfg var",
    )

    replace_once(
        prefs,
        """#ifdef _WIN64
\t\tCOMMAND_ID_HANDLER_EX(IDC_SEM71_ENABLED,    OnEditChange)
\t\tCOMMAND_ID_HANDLER_EX(IDC_ENHANCED_ENABLED, OnEditChange)
#endif
""",
        """\t\tCOMMAND_ID_HANDLER_EX(IDC_DSP_SURROUND,     OnEditChange)
#ifdef _WIN64
\t\tCOMMAND_ID_HANDLER_EX(IDC_ENHANCED_ENABLED, OnEditChange)
#endif
""",
        "restore SNES Surround message handler",
    )

    replace_once(
        prefs,
        """#define MAX_DSP_OPT 12
// populate dsp options (IDC_DSP_SURROUND slot is now Semantic 7.1 output)
static const int g_DSP_OPT[][2] = {
\t{IDC_DSP_OLDADPCM,\tDSP_OLDSMP},
\t{IDC_DSP_REVERSE,\tDSP_REVERSE},
""",
        """#define MAX_DSP_OPT 13
// populate DSP options; Surround is persisted in the historical option bitfield
static const int g_DSP_OPT[][2] = {
\t{IDC_DSP_OLDADPCM,\tDSP_OLDSMP},
\t{IDC_DSP_SURROUND,\tDSP_SURND},
\t{IDC_DSP_REVERSE,\tDSP_REVERSE},
""",
        "restore DSP_SURND to SNES option table",
    )

    replace_once(
        prefs,
        """#ifdef _WIN64
\tSendDlgItemMessage(IDC_SEM71_ENABLED, BM_SETCHECK, cfg_sem71_enabled, 0);
\tSendDlgItemMessage(IDC_ENHANCED_ENABLED, BM_SETCHECK, cfg_enhanced_enabled, 0);
#else
\t// Source-aware Spatial/enhanced paths are x64-only in this shell.
\t::EnableWindow(GetDlgItem(IDC_SEM71_ENABLED), FALSE);
\t::EnableWindow(GetDlgItem(IDC_ENHANCED_ENABLED), FALSE);
#endif
""",
        """#ifdef _WIN64
\tSendDlgItemMessage(IDC_ENHANCED_ENABLED, BM_SETCHECK, cfg_enhanced_enabled, 0);
#else
\t// Omniphony source presentation and enhanced playback are x64-only here.
\t::EnableWindow(GetDlgItem(IDC_DSP_SURROUND), FALSE);
\t::EnableWindow(GetDlgItem(IDC_ENHANCED_ENABLED), FALSE);
#endif
""",
        "initialize SNES Surround/enhanced controls",
    )

    replace_once(
        prefs,
        """#ifdef _WIN64
\t((CButton)GetDlgItem(IDC_SEM71_ENABLED)).SetCheck(1);  // preserve historical Spatial reset
\t((CButton)GetDlgItem(IDC_ENHANCED_ENABLED)).SetCheck(0); // reference synthesis default
#endif
""",
        """#ifdef _WIN64
\t((CButton)GetDlgItem(IDC_ENHANCED_ENABLED)).SetCheck(0); // reference synthesis default
#endif
""",
        "remove duplicate SNES spatial reset",
    )

    replace_once(
        prefs,
        """#ifdef _WIN64
\tcfg_sem71_enabled = SendDlgItemMessage(IDC_SEM71_ENABLED, BM_GETCHECK, 0, 0);
\tcfg_enhanced_enabled = SendDlgItemMessage(IDC_ENHANCED_ENABLED, BM_GETCHECK, 0, 0);
#endif
""",
        """#ifdef _WIN64
\tcfg_enhanced_enabled = SendDlgItemMessage(IDC_ENHANCED_ENABLED, BM_GETCHECK, 0, 0);
#endif
""",
        "remove duplicate SNES spatial apply",
    )

    replace_once(
        prefs,
        """#ifdef _WIN64
\tif (m_IDLastChanged == -1 || m_IDLastChanged == IDC_SEM71_ENABLED) {
\t\tif (SendDlgItemMessage(IDC_SEM71_ENABLED, BM_GETCHECK) != cfg_sem71_enabled) return true;
\t}
\tif (m_IDLastChanged == -1 || m_IDLastChanged == IDC_ENHANCED_ENABLED) {
\t\tif (SendDlgItemMessage(IDC_ENHANCED_ENABLED, BM_GETCHECK) != cfg_enhanced_enabled) return true;
\t}
#endif
""",
        """#ifdef _WIN64
\tif (m_IDLastChanged == -1 || m_IDLastChanged == IDC_ENHANCED_ENABLED) {
\t\tif (SendDlgItemMessage(IDC_ENHANCED_ENABLED, BM_GETCHECK) != cfg_enhanced_enabled) return true;
\t}
#endif
""",
        "remove duplicate SNES spatial dirty-state check",
    )

    replace_once(
        input_cpp,
        """extern cfg_int cfg_sem71_enabled;
extern cfg_int cfg_enhanced_enabled;
""",
        """extern cfg_int cfg_enhanced_enabled;
""",
        "remove SNES duplicate spatial runtime declaration",
    )

    print("SNESAPU historical Surround + enhanced UI restored")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
