#!/usr/bin/env python3
"""Normalize OPM-added host shapes so the pinned Genesis HQ-FM patch applies.

This is transformation-order glue only. It temporarily lets the existing HQ-FM
patch own shared Genesis anchors while preserving the independent OPM source
view and storage. No audio or source-selection semantics change here.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(path: Path, old: str, new: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir", type=Path)
    header = parser.parse_args().source_dir.resolve() / "source_aware_vgm_player.h"

    replace_once(
        header,
        "            if (rendered > kOutputCapacity) {\n"
        "                m_ym_block_valid = m_psg_block_valid = false;\n"
        "                m_opm_block_valid = false;\n"
        "            }\n",
        "            if (rendered > kOutputCapacity)\n"
        "                m_ym_block_valid = m_psg_block_valid = false;\n",
        "pre-HQ shared overflow anchor",
    )

    replace_once(
        header,
        "    bool m_unsupported_genesis_topology = false;\n"
        "    bool m_unsupported_opm_topology = false;\n"
        "    bool m_ym_block_valid = false;\n"
        "    bool m_psg_block_valid = false;\n"
        "    bool m_opm_block_valid = false;\n",
        "    bool m_unsupported_genesis_topology = false;\n"
        "    bool m_ym_block_valid = false;\n"
        "    bool m_psg_block_valid = false;\n"
        "    bool m_unsupported_opm_topology = false;\n"
        "    bool m_opm_block_valid = false;\n",
        "pre-HQ shared validity-member anchor",
    )

    replace_once(
        header,
        "    const stereo_sample* source_output(source_lane lane) const noexcept\n"
        "    {\n"
        "        const std::size_t index = static_cast<std::size_t>(lane);\n"
        "        return index < kLaneCount ? m_output[index].data() : nullptr;\n"
        "    }\n\n"
        "    const stereo_sample* opm_source_output(foobar_vgm::ym2151::source_lane lane) const noexcept\n"
        "    {\n"
        "        const std::size_t index = static_cast<std::size_t>(lane);\n"
        "        return index < kOpmLaneCount ? m_opm_output[index].data() : nullptr;\n"
        "    }\n\n"
        "protected:\n",
        "    const stereo_sample* opm_source_output(foobar_vgm::ym2151::source_lane lane) const noexcept\n"
        "    {\n"
        "        const std::size_t index = static_cast<std::size_t>(lane);\n"
        "        return index < kOpmLaneCount ? m_opm_output[index].data() : nullptr;\n"
        "    }\n\n"
        "    const stereo_sample* source_output(source_lane lane) const noexcept\n"
        "    {\n"
        "        const std::size_t index = static_cast<std::size_t>(lane);\n"
        "        return index < kLaneCount ? m_output[index].data() : nullptr;\n"
        "    }\n\n"
        "protected:\n",
        "pre-HQ public source-view anchor",
    )

    replace_once(
        header,
        "    YmCapture m_ym{};\n"
        "    PsgCapture m_psg{};\n"
        "    OpmCapture m_opm{};\n"
        "    std::array<std::array<stereo_sample, kOutputCapacity>, kLaneCount> m_output{};\n",
        "    YmCapture m_ym{};\n"
        "    OpmCapture m_opm{};\n"
        "    PsgCapture m_psg{};\n"
        "    std::array<std::array<stereo_sample, kOutputCapacity>, kLaneCount> m_output{};\n",
        "pre-HQ host-output storage anchor",
    )

    print("prepared YM2151 host additions for exact Genesis HQ-FM patch ordering")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
