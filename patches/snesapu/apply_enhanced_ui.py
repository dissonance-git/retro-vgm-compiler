#!/usr/bin/env python3
"""Add an independent Enhanced preference to the historical foobar SNESAPU shell.

This patch is configuration/UI only. It deliberately does not reinterpret the
legacy interpolation combo and does not couple Enhanced to Omniphony/Spatial.
The runtime consumes cfg_enhanced_enabled separately.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    raw = path.read_bytes()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    text = raw.decode("utf-8-sig")
    old_file = old.replace("\n", newline)
    new_file = new.replace("\n", newline)
    count = text.count(old_file)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    encoded = text.replace(old_file, new_file, 1).encode("utf-8")
    if raw.startswith(b"\xef\xbb\xbf"):
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
    args = parser.parse_args()
    root = args.source_dir.resolve()
    prefs = root / "preferences_snesapu.cpp"
    resource_h = root / "resource.h"
    resource_rc = root / "resource.rc"
    input_cpp = root / "input_snesapu.cpp"

    replace_once(
        resource_h,
        """#define IDC_SPATIAL_DEPTH_LABEL         1032


// Next default values for new objects
""",
        """#define IDC_SPATIAL_DEPTH_LABEL         1032
#define IDC_ENHANCED_ENABLED            1033


// Next default values for new objects
""",
        "SNES Enhanced control id",
    )
    replace_once(
        resource_h,
        """#define _APS_NEXT_CONTROL_VALUE         1033
""",
        """#define _APS_NEXT_CONTROL_VALUE         1034
""",
        "SNES next control id",
    )
    replace_once(
        resource_rc,
        """    CONTROL         "Enable Spatial Pre-Conditioning",IDC_SEM71_ENABLED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,170,65,145,15
""",
        """    CONTROL         "Spatial",IDC_SEM71_ENABLED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,170,65,60,15
    CONTROL         "Enhanced",IDC_ENHANCED_ENABLED,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,240,65,70,15
""",
        "SNES independent Spatial/Enhanced controls",
    )

    replace_once(
        prefs,
        """#ifdef _WIN64
static const GUID guid_sem71_enabled =
{ 0xb7c3a912, 0x6f11, 0x4d8e, { 0x8a, 0x15, 0xc2, 0x3d, 0x67, 0xf9, 0x01, 0xae } };
#endif
""",
        """#ifdef _WIN64
static const GUID guid_sem71_enabled =
{ 0xb7c3a912, 0x6f11, 0x4d8e, { 0x8a, 0x15, 0xc2, 0x3d, 0x67, 0xf9, 0x01, 0xae } };
static const GUID guid_enhanced_enabled =
{ 0x47f27b61, 0x8ac2, 0x4f30, { 0xb9, 0x11, 0x72, 0xd8, 0x91, 0x51, 0x5a, 0x03 } };
#endif
""",
        "SNES Enhanced preference GUID",
    )
    replace_once(
        prefs,
        """#ifdef _WIN64
cfg_int cfg_sem71_enabled             (guid_sem71_enabled,             1);  // default ON
#endif
""",
        """#ifdef _WIN64
cfg_int cfg_sem71_enabled             (guid_sem71_enabled,             1);  // preserve existing Spatial preference
cfg_int cfg_enhanced_enabled          (guid_enhanced_enabled,          0);  // protected reference remains default
#endif
""",
        "SNES Enhanced cfg var",
    )
    replace_once(
        prefs,
        """#ifdef _WIN64
\t\tCOMMAND_ID_HANDLER_EX(IDC_SEM71_ENABLED,    OnEditChange)
#endif
""",
        """#ifdef _WIN64
\t\tCOMMAND_ID_HANDLER_EX(IDC_SEM71_ENABLED,    OnEditChange)
\t\tCOMMAND_ID_HANDLER_EX(IDC_ENHANCED_ENABLED, OnEditChange)
#endif
""",
        "SNES Enhanced message handler",
    )
    replace_once(
        prefs,
        """#ifdef _WIN64
\tSendDlgItemMessage(IDC_SEM71_ENABLED, BM_SETCHECK, cfg_sem71_enabled, 0);
#else
""",
        """#ifdef _WIN64
\tSendDlgItemMessage(IDC_SEM71_ENABLED, BM_SETCHECK, cfg_sem71_enabled, 0);
\tSendDlgItemMessage(IDC_ENHANCED_ENABLED, BM_SETCHECK, cfg_enhanced_enabled, 0);
#else
""",
        "SNES Enhanced initialization",
    )
    replace_once(
        prefs,
        """#else
\t// Semantic 7.1 is x64-only; disable the checkbox on x86
\t::EnableWindow(GetDlgItem(IDC_SEM71_ENABLED), FALSE);
#endif
""",
        """#else
\t// Source-aware Spatial/Enhanced paths are x64-only in this shell.
\t::EnableWindow(GetDlgItem(IDC_SEM71_ENABLED), FALSE);
\t::EnableWindow(GetDlgItem(IDC_ENHANCED_ENABLED), FALSE);
#endif
""",
        "SNES x86 control disable",
    )
    replace_once(
        prefs,
        """#ifdef _WIN64
\t((CButton)GetDlgItem(IDC_SEM71_ENABLED)).SetCheck(1);  // default ON
#endif
""",
        """#ifdef _WIN64
\t((CButton)GetDlgItem(IDC_SEM71_ENABLED)).SetCheck(1);  // preserve historical Spatial reset
\t((CButton)GetDlgItem(IDC_ENHANCED_ENABLED)).SetCheck(0); // reference synthesis default
#endif
""",
        "SNES Enhanced reset",
    )
    replace_once(
        prefs,
        """#ifdef _WIN64
\tcfg_sem71_enabled = SendDlgItemMessage(IDC_SEM71_ENABLED, BM_GETCHECK, 0, 0);
#endif
""",
        """#ifdef _WIN64
\tcfg_sem71_enabled = SendDlgItemMessage(IDC_SEM71_ENABLED, BM_GETCHECK, 0, 0);
\tcfg_enhanced_enabled = SendDlgItemMessage(IDC_ENHANCED_ENABLED, BM_GETCHECK, 0, 0);
#endif
""",
        "SNES Enhanced apply",
    )
    replace_once(
        prefs,
        """#ifdef _WIN64
\tif (m_IDLastChanged == -1 || m_IDLastChanged == IDC_SEM71_ENABLED) {
\t\tif (SendDlgItemMessage(IDC_SEM71_ENABLED, BM_GETCHECK) != cfg_sem71_enabled) return true;
\t}
#endif
""",
        """#ifdef _WIN64
\tif (m_IDLastChanged == -1 || m_IDLastChanged == IDC_SEM71_ENABLED) {
\t\tif (SendDlgItemMessage(IDC_SEM71_ENABLED, BM_GETCHECK) != cfg_sem71_enabled) return true;
\t}
\tif (m_IDLastChanged == -1 || m_IDLastChanged == IDC_ENHANCED_ENABLED) {
\t\tif (SendDlgItemMessage(IDC_ENHANCED_ENABLED, BM_GETCHECK) != cfg_enhanced_enabled) return true;
\t}
#endif
""",
        "SNES Enhanced dirty-state tracking",
    )
    replace_once(
        input_cpp,
        """extern cfg_int cfg_sem71_enabled;
""",
        """extern cfg_int cfg_sem71_enabled;
extern cfg_int cfg_enhanced_enabled;
""",
        "SNES Enhanced runtime declaration",
    )

    print("SNESAPU independent Enhanced preference applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
