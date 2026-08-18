#!/usr/bin/env python3
"""Audit the outer private VGM/SPC bundle after final ZIP creation.

The component archives are already validated individually. This last-mile gate
proves that the combined bundle contains those exact bytes, that SHA256SUMS and
the JSON manifest describe the embedded packages, and that no unexpected or
nested files slipped into the release envelope. The embedded component verifier
is then run again on copies extracted from the final bundle, so Windows runtime
ABI/startup checks apply to the exact artifacts being handed to the user.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import tempfile
import zipfile


VGM_COMPONENT = "foo_input_vgm.private.fb2k-component"
SPC_COMPONENT = "foo_snesapu.private.fb2k-component"
COMPONENTS = (VGM_COMPONENT, SPC_COMPONENT)
EXPECTED_ENTRIES = {
    VGM_COMPONENT,
    SPC_COMPONENT,
    "build-manifest.json",
    "SHA256SUMS.txt",
    "README.txt",
}
EXPECTED_ARCHITECTURE = {
    "foo_input_vgm": "x64",
    "foo_snesapu": "x64",
    "omniphony_source": "x64",
    "spcplayer": "x86",
    "SNESAPU": "x86",
}
HEX40 = re.compile(r"^[0-9a-fA-F]{40}$")
HEX64 = re.compile(r"^[0-9a-fA-F]{64}$")
PROPER_ENHANCED = re.compile(r"\bEnhanced\b")


def _safe_flat_names(archive: zipfile.ZipFile) -> list[str]:
    infos = [info for info in archive.infolist() if not info.is_dir()]
    names = [PurePosixPath(info.filename).as_posix() for info in infos]
    unsafe = [
        name
        for name in names
        if not name
        or name.startswith("/")
        or ".." in PurePosixPath(name).parts
        or len(PurePosixPath(name).parts) != 1
    ]
    if unsafe:
        raise AssertionError(f"private bundle has unsafe/nested entries: {unsafe}")
    folded: dict[str, str] = {}
    for name in names:
        key = name.casefold()
        if key in folded:
            raise AssertionError(
                "private bundle has case-insensitive duplicate entries: "
                f"{folded[key]!r}, {name!r}"
            )
        folded[key] = name
    return names


def parse_sha256sums(text: str) -> dict[str, str]:
    entries: dict[str, str] = {}
    for line_number, raw in enumerate(text.splitlines(), start=1):
        line = raw.strip()
        if not line:
            continue
        parts = line.split(None, 1)
        if len(parts) != 2:
            raise AssertionError(f"invalid SHA256SUMS line {line_number}: {raw!r}")
        digest, filename = parts[0].lower(), parts[1].strip()
        if filename.startswith("*"):
            filename = filename[1:]
        if not HEX64.fullmatch(digest):
            raise AssertionError(f"invalid SHA-256 on line {line_number}: {digest!r}")
        if filename in entries:
            raise AssertionError(f"duplicate SHA256SUMS filename: {filename}")
        entries[filename] = digest
    return entries


def verify_bundle_metadata(bundle: Path) -> dict[str, object]:
    if not bundle.is_file():
        raise RuntimeError(f"private bundle missing: {bundle}")
    if not zipfile.is_zipfile(bundle):
        raise RuntimeError(f"private bundle is not a ZIP archive: {bundle}")

    with zipfile.ZipFile(bundle, "r") as archive:
        names = _safe_flat_names(archive)
        actual = set(names)
        if actual != EXPECTED_ENTRIES:
            raise AssertionError(
                "private bundle payload mismatch; "
                f"missing={sorted(EXPECTED_ENTRIES - actual)}, "
                f"extra={sorted(actual - EXPECTED_ENTRIES)}"
            )
        empty = sorted(
            info.filename
            for info in archive.infolist()
            if not info.is_dir() and info.file_size == 0
        )
        if empty:
            raise AssertionError(f"private bundle contains zero-byte files: {empty}")

        try:
            manifest = json.loads(archive.read("build-manifest.json").decode("utf-8-sig"))
        except (UnicodeDecodeError, json.JSONDecodeError) as exc:
            raise AssertionError(f"invalid build-manifest.json: {exc}") from exc
        if not isinstance(manifest, dict):
            raise AssertionError("build-manifest.json must contain one JSON object")

        packages = manifest.get("packages")
        if not isinstance(packages, list) or set(packages) != set(COMPONENTS) or len(packages) != 2:
            raise AssertionError(f"manifest packages do not match bundle components: {packages!r}")
        if manifest.get("final_playback_contract_hz") != 48000:
            raise AssertionError(
                "manifest final_playback_contract_hz must be exactly 48000"
            )
        if manifest.get("binary_architecture") != EXPECTED_ARCHITECTURE:
            raise AssertionError(
                "manifest binary_architecture mismatch: "
                f"{manifest.get('binary_architecture')!r}"
            )
        retro_commit = manifest.get("retro_vgm_compiler")
        if not isinstance(retro_commit, str) or not HEX40.fullmatch(retro_commit):
            raise AssertionError(
                "manifest retro_vgm_compiler must be the exact 40-hex source commit"
            )

        sums_text = archive.read("SHA256SUMS.txt").decode("ascii")
        sums = parse_sha256sums(sums_text)
        if set(sums) != set(COMPONENTS):
            raise AssertionError(
                f"SHA256SUMS must describe exactly the two component archives: {sorted(sums)}"
            )
        for name in COMPONENTS:
            actual_hash = hashlib.sha256(archive.read(name)).hexdigest()
            if sums[name] != actual_hash:
                raise AssertionError(
                    f"SHA256SUMS mismatch for {name}: expected {sums[name]}, got {actual_hash}"
                )

        readme = archive.read("README.txt").decode("utf-8-sig")
        if PROPER_ENHANCED.search(readme):
            raise AssertionError("bundle README must use lowercase descriptive 'enhanced'")
        if "enhanced" not in readme or "Surround" not in readme:
            raise AssertionError("bundle README is missing playback-control description")

    return manifest


def verify_embedded_components(bundle: Path) -> None:
    verifier = Path(__file__).with_name("verify_private_component_packages.py")
    with tempfile.TemporaryDirectory(prefix="private-bundle-components-") as temporary:
        root = Path(temporary)
        with zipfile.ZipFile(bundle, "r") as archive:
            for name in COMPONENTS:
                (root / name).write_bytes(archive.read(name))
        completed = subprocess.run(
            [
                sys.executable,
                str(verifier),
                str(root / VGM_COMPONENT),
                str(root / SPC_COMPONENT),
            ],
            check=False,
        )
        if completed.returncode != 0:
            raise AssertionError(
                "embedded private component verification failed with exit code "
                f"{completed.returncode}"
            )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("bundle", type=Path)
    args = parser.parse_args()
    bundle = args.bundle.resolve()

    manifest = verify_bundle_metadata(bundle)
    verify_embedded_components(bundle)
    print(
        "private component bundle verified at source commit "
        f"{manifest['retro_vgm_compiler']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
