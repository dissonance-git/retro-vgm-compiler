#!/usr/bin/env python3
"""Static contract for the canonical SPC parent/child patch graph."""

from __future__ import annotations

from pathlib import Path
import re


REPO = Path(__file__).resolve().parents[2]
PATCHES = REPO / "patches" / "snesapu"


def main() -> int:
    wrapper = (PATCHES / "apply_prebrr_transport_complete.py").read_text(encoding="utf-8")
    child = (PATCHES / "apply_current_child_source_transport.py").read_text(encoding="utf-8")
    parent = (PATCHES / "apply_current_parent_source_transport.py").read_text(encoding="utf-8")

    assert 'apply_current_parent_source_transport.py' in wrapper
    assert 'apply_current_child_source_transport.py' in wrapper
    for retired in (
        'fix_prebrr_pointer_callback.py',
        'apply_studio_source_transport.py',
    ):
        assert retired not in wrapper, f"retired migration helper returned to active chain: {retired}"

    match = re.search(
        r"static\s+u32\s+__stdcall\s+retro_prebrr_callback\s*\((.*?)\)\s*\{",
        child,
        re.S,
    )
    assert match, "canonical child is missing explicit stdcall pre-BRR wrapper"
    params = [part.strip() for part in match.group(1).split(',') if part.strip()]
    assert len(params) == 4, f"pre-BRR wrapper ABI drifted to {len(params)} parameters: {params}"
    assert 'SetDSPPreBrrProvider' in child
    assert 'SetDSPStudioSourceProvider' in child
    assert 'SPCP_HEADER_VERSION    3' in child
    assert 'invalid verified studio-source packet or BRR witness mismatch' in child

    assert 'SetPreBrrPacket' in parent
    assert 'SetStudioSourcePacket' in parent
    assert '.prebrr' in parent
    assert '.studiosrc' in parent
    assert 'cfg_enhanced_enabled' in parent

    print("active SPC patch contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
