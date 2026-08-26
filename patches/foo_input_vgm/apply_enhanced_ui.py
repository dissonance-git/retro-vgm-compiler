#!/usr/bin/env python3
"""Keep the future enhanced preference inert while exposing Surround.

The historical foo_input_vgm preference is relabeled exactly "Surround". The
enhanced preference remains in the generated ABI because later source-capture
patches consume its identifier, but the control is visibly disabled and runtime
code hard-gates the value to zero for this project phase.
"""

from __future__ import annotations

import argparse
from pathlib import Path


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
    parser.add_argument("source_dir", type=Path, help="foo_input_vgm/src directory")
    args = parser.parse_args()
    root = args.source_dir.resolve()
    config = root / "config_foo_input_vgm.cpp"
    external = root / "my_cfg_external.h"
    resource_h = root / "resource.h"
    resource_rc = root / "config_foo_input_vgm.rc"

    # 0.31 already owns the real historical Surround control. Add only the
    # independent source-quality control beside it. No second spatial switch.
    replace_once(
        resource_h,
        """#define IDC_BPS                         1027
#define IDC_CHIP_TYPE                   1100
""",
        """#define IDC_BPS                         1027
#define IDC_ENHANCED_ENABLED_VGM        1028
#define IDC_CHIP_TYPE                   1100
""",
        "VGM enhanced control id",
    )
    replace_once(
        resource_rc,
        '''    CONTROL         """Surround"" sound",IDC_SURROUND_SOUND,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,13,222,73,10
    LTEXT           "Volume",IDC_STATIC,13,242,24,8
''',
        '''    CONTROL         "Surround",IDC_SURROUND_SOUND,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,13,222,73,10
    CONTROL         "enhanced (later)",IDC_ENHANCED_ENABLED_VGM,"Button",BS_AUTOCHECKBOX | WS_TABSTOP | WS_DISABLED,97,222,86,10
    LTEXT           "Volume",IDC_STATIC,13,242,24,8
''',
        "VGM enhanced checkbox beside existing Surround",
    )
    replace_once(
        config,
        """// {1320E264-D0DF-4019-955C-B685694191D7}
static const GUID guid_cfg_bps =
{ 0x1320e264, 0xd0df, 0x4019, { 0x95, 0x5c, 0xb6, 0x85, 0x69, 0x41, 0x91, 0xd7 } };


cfg_int cfg_resampling_mode(guid_cfg_resampling_mode, 0);
""",
        """// {1320E264-D0DF-4019-955C-B685694191D7}
static const GUID guid_cfg_bps =
{ 0x1320e264, 0xd0df, 0x4019, { 0x95, 0x5c, 0xb6, 0x85, 0x69, 0x41, 0x91, 0xd7 } };
// {DD6D49F2-2401-4793-83B0-2986B480C9D7}
static const GUID guid_cfg_vgm_enhanced_enabled =
{ 0xdd6d49f2, 0x2401, 0x4793, { 0x83, 0xb0, 0x29, 0x86, 0xb4, 0x80, 0xc9, 0xd7 } };


cfg_int cfg_resampling_mode(guid_cfg_resampling_mode, 0);
""",
        "VGM enhanced preference GUID",
    )
    replace_once(
        config,
        """cfg_int cfg_surround_sound(guid_cfg_surround_sound, 0);
cfg_int cfg_volume(guid_cfg_volume, 100);
""",
        """cfg_int cfg_surround_sound(guid_cfg_surround_sound, 0);
cfg_int cfg_volume(guid_cfg_volume, 100);
cfg_int cfg_vgm_enhanced_enabled(guid_cfg_vgm_enhanced_enabled, 0);
""",
        "VGM enhanced cfg var",
    )
    replace_once(
        config,
        """\t\tCOMMAND_HANDLER_EX(IDC_HARD_STOP_OLD_VGMS, BN_CLICKED, OnButtonClick)
\t\tCOMMAND_HANDLER_EX(IDC_SURROUND_SOUND, BN_CLICKED, OnButtonClick)
\t\tCOMMAND_HANDLER_EX(IDC_VOLUME, EN_CHANGE, OnEditChange)
""",
        """\t\tCOMMAND_HANDLER_EX(IDC_HARD_STOP_OLD_VGMS, BN_CLICKED, OnButtonClick)
\t\tCOMMAND_HANDLER_EX(IDC_SURROUND_SOUND, BN_CLICKED, OnButtonClick)
\t\tCOMMAND_HANDLER_EX(IDC_ENHANCED_ENABLED_VGM, BN_CLICKED, OnButtonClick)
\t\tCOMMAND_HANDLER_EX(IDC_VOLUME, EN_CHANGE, OnEditChange)
""",
        "VGM enhanced message handler",
    )
    replace_once(
        config,
        """\tCheckDlgButton(IDC_HARD_STOP_OLD_VGMS, (UINT)cfg_hard_stop_old_vgms);
\tCheckDlgButton(IDC_SURROUND_SOUND, (UINT)cfg_surround_sound);

\tSetDlgItemInt(IDC_VOLUME, (UINT)cfg_volume, FALSE);
""",
        """\tCheckDlgButton(IDC_HARD_STOP_OLD_VGMS, (UINT)cfg_hard_stop_old_vgms);
\tCheckDlgButton(IDC_SURROUND_SOUND, (UINT)cfg_surround_sound);
\tcfg_vgm_enhanced_enabled = 0;
\tCheckDlgButton(IDC_ENHANCED_ENABLED_VGM, BST_UNCHECKED);

\tSetDlgItemInt(IDC_VOLUME, (UINT)cfg_volume, FALSE);
""",
        "VGM enhanced initialization",
    )
    replace_once(
        config,
        """\t\tcfg_surround_sound != IsDlgButtonChecked(IDC_SURROUND_SOUND) ||
\t\tcfg_volume != GetDlgItemInt(IDC_VOLUME, NULL, FALSE) ||
""",
        """\t\tcfg_surround_sound != IsDlgButtonChecked(IDC_SURROUND_SOUND) ||
\t\tcfg_vgm_enhanced_enabled != IsDlgButtonChecked(IDC_ENHANCED_ENABLED_VGM) ||
\t\tcfg_volume != GetDlgItemInt(IDC_VOLUME, NULL, FALSE) ||
""",
        "VGM enhanced dirty-state tracking",
    )
    replace_once(
        config,
        """\tcfg_hard_stop_old_vgms = IsDlgButtonChecked(IDC_HARD_STOP_OLD_VGMS);
\tcfg_surround_sound = IsDlgButtonChecked(IDC_SURROUND_SOUND);

\tcfg_volume = GetDlgItemInt(IDC_VOLUME, NULL, FALSE);
""",
        """\tcfg_hard_stop_old_vgms = IsDlgButtonChecked(IDC_HARD_STOP_OLD_VGMS);
\tcfg_surround_sound = IsDlgButtonChecked(IDC_SURROUND_SOUND);
\tcfg_vgm_enhanced_enabled = 0;

\tcfg_volume = GetDlgItemInt(IDC_VOLUME, NULL, FALSE);
""",
        "VGM enhanced apply",
    )
    replace_once(
        config,
        """\tCheckDlgButton(IDC_HARD_STOP_OLD_VGMS, BST_UNCHECKED);
\tCheckDlgButton(IDC_SURROUND_SOUND, BST_UNCHECKED);
\tSetDlgItemInt(IDC_VOLUME, (UINT)100, FALSE);
""",
        """\tCheckDlgButton(IDC_HARD_STOP_OLD_VGMS, BST_UNCHECKED);
\tCheckDlgButton(IDC_SURROUND_SOUND, BST_UNCHECKED);
\tCheckDlgButton(IDC_ENHANCED_ENABLED_VGM, BST_UNCHECKED);
\tSetDlgItemInt(IDC_VOLUME, (UINT)100, FALSE);
""",
        "VGM enhanced reset",
    )
    replace_once(
        external,
        """extern cfg_int cfg_surround_sound;
extern cfg_int cfg_volume;
extern cfg_int cfg_prefer_jpn_tag;
""",
        """extern cfg_int cfg_surround_sound;
extern cfg_int cfg_volume;
extern cfg_int cfg_vgm_enhanced_enabled;
extern cfg_int cfg_prefer_jpn_tag;
""",
        "VGM enhanced external declaration",
    )

    print("foo_input_vgm Surround + enhanced preference surface applied successfully")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
