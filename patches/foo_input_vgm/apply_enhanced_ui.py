#!/usr/bin/env python3
"""Add an independent Enhanced preference to the historical foo_input_vgm shell.

The existing Spatial/Omniphony preference keeps its GUID and saved state.
Enhanced defaults off and is not coupled to resampling mode, chip sample rate, or
Spatial. Runtime backends consume cfg_vgm_enhanced_enabled independently.
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
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    args = parser.parse_args()
    root = args.source_dir.resolve()
    config = root / "config_foo_input_vgm.cpp"
    external = root / "my_cfg_external.h"
    resource_h = root / "resource.h"
    resource_rc = root / "config_foo_input_vgm.rc"

    replace_once(
        resource_h,
        """#define IDC_SEM71_ENABLED_VGM           1028
#define IDC_CHIP_TYPE                   1100
""",
        """#define IDC_SEM71_ENABLED_VGM           1028
#define IDC_ENHANCED_ENABLED_VGM        1029
#define IDC_CHIP_TYPE                   1100
""",
        "VGM Enhanced control id",
    )
    replace_once(
        resource_rc,
        """    CONTROL         "Enable Spatial Pre-Conditioning",IDC_SEM71_ENABLED_VGM,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,13,222,120,10
""",
        """    CONTROL         "Spatial",IDC_SEM71_ENABLED_VGM,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,13,222,60,10
    CONTROL         "Enhanced",IDC_ENHANCED_ENABLED_VGM,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,86,222,70,10
""",
        "VGM independent Spatial/Enhanced controls",
    )
    replace_once(
        config,
        """// {A3F12E01-CC47-4D89-B2F1-8DC34E7A1204}
static const GUID guid_cfg_vgm_sem71_enabled =
{ 0xa3f12e01, 0xcc47, 0x4d89, { 0xb2, 0xf1, 0x8d, 0xc3, 0x4e, 0x7a, 0x12, 0x04 } };


cfg_int cfg_resampling_mode(guid_cfg_resampling_mode, 0);
""",
        """// {A3F12E01-CC47-4D89-B2F1-8DC34E7A1204}
static const GUID guid_cfg_vgm_sem71_enabled =
{ 0xa3f12e01, 0xcc47, 0x4d89, { 0xb2, 0xf1, 0x8d, 0xc3, 0x4e, 0x7a, 0x12, 0x04 } };
// {DD6D49F2-2401-4793-83B0-2986B480C9D7}
static const GUID guid_cfg_vgm_enhanced_enabled =
{ 0xdd6d49f2, 0x2401, 0x4793, { 0x83, 0xb0, 0x29, 0x86, 0xb4, 0x80, 0xc9, 0xd7 } };


cfg_int cfg_resampling_mode(guid_cfg_resampling_mode, 0);
""",
        "VGM Enhanced preference GUID",
    )
    replace_once(
        config,
        """cfg_int cfg_surround_sound(guid_cfg_surround_sound, 0);
cfg_int cfg_volume(guid_cfg_volume, 100);
cfg_int cfg_vgm_sem71_enabled(guid_cfg_vgm_sem71_enabled, 1);  // default ON
""",
        """cfg_int cfg_surround_sound(guid_cfg_surround_sound, 0);
cfg_int cfg_volume(guid_cfg_volume, 100);
cfg_int cfg_vgm_sem71_enabled(guid_cfg_vgm_sem71_enabled, 1);  // preserve existing Spatial preference
cfg_int cfg_vgm_enhanced_enabled(guid_cfg_vgm_enhanced_enabled, 0); // protected reference default
""",
        "VGM Enhanced cfg var",
    )
    replace_once(
        config,
        """\t\tCOMMAND_HANDLER_EX(IDC_HARD_STOP_OLD_VGMS, BN_CLICKED, OnButtonClick)
\t\tCOMMAND_HANDLER_EX(IDC_SEM71_ENABLED_VGM, BN_CLICKED, OnButtonClick)
\t\tCOMMAND_HANDLER_EX(IDC_VOLUME, EN_CHANGE, OnEditChange)
""",
        """\t\tCOMMAND_HANDLER_EX(IDC_HARD_STOP_OLD_VGMS, BN_CLICKED, OnButtonClick)
\t\tCOMMAND_HANDLER_EX(IDC_SEM71_ENABLED_VGM, BN_CLICKED, OnButtonClick)
\t\tCOMMAND_HANDLER_EX(IDC_ENHANCED_ENABLED_VGM, BN_CLICKED, OnButtonClick)
\t\tCOMMAND_HANDLER_EX(IDC_VOLUME, EN_CHANGE, OnEditChange)
""",
        "VGM Enhanced message handler",
    )
    replace_once(
        config,
        """\tCheckDlgButton(IDC_HARD_STOP_OLD_VGMS, (UINT)cfg_hard_stop_old_vgms);
\tCheckDlgButton(IDC_SEM71_ENABLED_VGM, (UINT)cfg_vgm_sem71_enabled);

\tSetDlgItemInt(IDC_VOLUME, (UINT)cfg_volume, FALSE);
""",
        """\tCheckDlgButton(IDC_HARD_STOP_OLD_VGMS, (UINT)cfg_hard_stop_old_vgms);
\tCheckDlgButton(IDC_SEM71_ENABLED_VGM, (UINT)cfg_vgm_sem71_enabled);
\tCheckDlgButton(IDC_ENHANCED_ENABLED_VGM, (UINT)cfg_vgm_enhanced_enabled);

\tSetDlgItemInt(IDC_VOLUME, (UINT)cfg_volume, FALSE);
""",
        "VGM Enhanced initialization",
    )
    replace_once(
        config,
        """\t\tcfg_pause_non_looping != GetDlgItemInt(IDC_PAUSE_NON_LOOPING, NULL, FALSE) ||
\t\tcfg_vgm_sem71_enabled != (int)IsDlgButtonChecked(IDC_SEM71_ENABLED_VGM) ||
\t\tcfg_volume != GetDlgItemInt(IDC_VOLUME, NULL, FALSE) ||
""",
        """\t\tcfg_pause_non_looping != GetDlgItemInt(IDC_PAUSE_NON_LOOPING, NULL, FALSE) ||
\t\tcfg_vgm_sem71_enabled != (int)IsDlgButtonChecked(IDC_SEM71_ENABLED_VGM) ||
\t\tcfg_vgm_enhanced_enabled != (int)IsDlgButtonChecked(IDC_ENHANCED_ENABLED_VGM) ||
\t\tcfg_volume != GetDlgItemInt(IDC_VOLUME, NULL, FALSE) ||
""",
        "VGM Enhanced dirty-state tracking",
    )
    replace_once(
        config,
        """\tcfg_hard_stop_old_vgms = IsDlgButtonChecked(IDC_HARD_STOP_OLD_VGMS);
\tcfg_vgm_sem71_enabled = IsDlgButtonChecked(IDC_SEM71_ENABLED_VGM);

\tcfg_volume = GetDlgItemInt(IDC_VOLUME, NULL, FALSE);
""",
        """\tcfg_hard_stop_old_vgms = IsDlgButtonChecked(IDC_HARD_STOP_OLD_VGMS);
\tcfg_vgm_sem71_enabled = IsDlgButtonChecked(IDC_SEM71_ENABLED_VGM);
\tcfg_vgm_enhanced_enabled = IsDlgButtonChecked(IDC_ENHANCED_ENABLED_VGM);

\tcfg_volume = GetDlgItemInt(IDC_VOLUME, NULL, FALSE);
""",
        "VGM Enhanced apply",
    )
    replace_once(
        config,
        """\tCheckDlgButton(IDC_HARD_STOP_OLD_VGMS, BST_UNCHECKED);
\tCheckDlgButton(IDC_SEM71_ENABLED_VGM, BST_CHECKED);  // default ON
\tSetDlgItemInt(IDC_VOLUME, (UINT)100, FALSE);
""",
        """\tCheckDlgButton(IDC_HARD_STOP_OLD_VGMS, BST_UNCHECKED);
\tCheckDlgButton(IDC_SEM71_ENABLED_VGM, BST_CHECKED);  // preserve historical Spatial reset
\tCheckDlgButton(IDC_ENHANCED_ENABLED_VGM, BST_UNCHECKED);
\tSetDlgItemInt(IDC_VOLUME, (UINT)100, FALSE);
""",
        "VGM Enhanced reset",
    )
    replace_once(
        external,
        """extern cfg_int cfg_vgm_sem71_enabled;
extern cfg_int cfg_prefer_jpn_tag;
""",
        """extern cfg_int cfg_vgm_sem71_enabled;
extern cfg_int cfg_vgm_enhanced_enabled;
extern cfg_int cfg_prefer_jpn_tag;
""",
        "VGM Enhanced external declaration",
    )

    print("foo_input_vgm independent Enhanced preference applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
