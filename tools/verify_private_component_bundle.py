#!/usr/bin/env python3
"""Audit the outer private VGM/SPC bundle after final ZIP creation.

The component archives are already validated individually. This last-mile gate
proves that the combined bundle contains those exact bytes, that SHA256SUMS and
the JSON manifest describe the embedded packages, and that the intentional VGM/
and SPC/ manual-runtime directories contain byte-identical copies of the files
inside those component archives. The embedded component verifier is then run
again on copies extracted from the final bundle, so Windows runtime ABI/startup
checks apply to the exact artifacts being handed to the user.
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import tempfile
import zipfile


VGM_COMPONENT = "foo_input_vgm.private.fb2k-component"
SPC_COMPONENT = "foo_snesapu.private.fb2k-component"
BUNDLE_NAME = "private-foobar-vgm-spc.zip"
COMPONENTS = (VGM_COMPONENT, SPC_COMPONENT)
EXPECTED_OUTPUTS = [VGM_COMPONENT, SPC_COMPONENT, BUNDLE_NAME]

TOP_LEVEL_ENTRIES = {
    VGM_COMPONENT,
    SPC_COMPONENT,
    "build-manifest.json",
    "SHA256SUMS.txt",
    "README.txt",
}
RUNTIME_ENTRIES = {
    "VGM/foo_input_vgm.dll",
    "VGM/omniphony_source.dll",
    "SPC/foo_snesapu.dll",
    "SPC/spcplayer.exe",
    "SPC/SNESAPU.dll",
    "SPC/omniphony_source.dll",
}
EXPECTED_ENTRIES = TOP_LEVEL_ENTRIES | RUNTIME_ENTRIES
RUNTIME_PACKAGE_MEMBERS = {
    "VGM/foo_input_vgm.dll": (VGM_COMPONENT, "foo_input_vgm.dll"),
    "VGM/omniphony_source.dll": (VGM_COMPONENT, "omniphony_source.dll"),
    "SPC/foo_snesapu.dll": (SPC_COMPONENT, "foo_snesapu.dll"),
    "SPC/spcplayer.exe": (SPC_COMPONENT, "spcplayer.exe"),
    "SPC/SNESAPU.dll": (SPC_COMPONENT, "SNESAPU.dll"),
    "SPC/omniphony_source.dll": (SPC_COMPONENT, "omniphony_source.dll"),
}
HEX40 = re.compile(r"^[0-9a-fA-F]{40}$")
HEX64 = re.compile(r"^[0-9a-fA-F]{64}$")
PROPER_ENHANCED = re.compile(r"\bEnhanced\b")


def _safe_bundle_names(archive: zipfile.ZipFile) -> list[str]:
    infos = [info for info in archive.infolist() if not info.is_dir()]
    names: list[str] = []
    unsafe: list[str] = []
    folded: dict[str, str] = {}

    for info in infos:
        raw = info.filename
        pure = PurePosixPath(raw)
        parts = pure.parts
        allowed_shape = (
            len(parts) == 1
            or (len(parts) == 2 and parts[0] in {"VGM", "SPC"})
        )
        raw_parts = raw.split("/")
        if (
            not raw
            or raw.startswith("/")
            or "\\" in raw
            or any(part in {"", ".", ".."} for part in raw_parts)
            or not allowed_shape
        ):
            unsafe.append(raw)
            continue

        name = pure.as_posix()
        key = name.casefold()
        if key in folded:
            raise AssertionError(
                "private bundle has case-insensitive duplicate entries: "
                f"{folded[key]!r}, {name!r}"
            )
        folded[key] = name
        names.append(name)

    if unsafe:
        raise AssertionError(f"private bundle has unsafe entries: {unsafe}")
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


def _require_commit_pin(manifest: dict[str, object], key: str) -> str:
    value = manifest.get(key)
    if not isinstance(value, dict):
        raise AssertionError(f"manifest {key} entry must be an object")
    commit = value.get("commit")
    if not isinstance(commit, str) or not HEX40.fullmatch(commit):
        raise AssertionError(f"manifest {key}.commit must be an exact 40-hex commit")
    return commit


def _verify_manifest(manifest: dict[str, object]) -> None:
    built_at = manifest.get("built_at_utc")
    if not isinstance(built_at, str) or not built_at.strip():
        raise AssertionError("manifest built_at_utc must be a non-empty timestamp")

    retro_commit = manifest.get("retro_vgm_compiler_commit")
    if not isinstance(retro_commit, str) or not HEX40.fullmatch(retro_commit):
        raise AssertionError(
            "manifest retro_vgm_compiler_commit must be the exact 40-hex source commit"
        )

    outputs = manifest.get("outputs")
    if outputs != EXPECTED_OUTPUTS:
        raise AssertionError(
            f"manifest outputs mismatch: expected {EXPECTED_OUTPUTS!r}, got {outputs!r}"
        )

    bootstrap = manifest.get("foo_input_vgm_bootstrap")
    if not isinstance(bootstrap, dict):
        raise AssertionError("manifest foo_input_vgm_bootstrap must be an object")
    if not isinstance(bootstrap.get("source_page"), str) or not bootstrap["source_page"]:
        raise AssertionError("manifest foo_input_vgm_bootstrap.source_page is missing")
    bootstrap_sha = bootstrap.get("sha256")
    if not isinstance(bootstrap_sha, str) or not HEX64.fullmatch(bootstrap_sha):
        raise AssertionError("manifest foo_input_vgm_bootstrap.sha256 must be 64 hex")

    sdk = manifest.get("foobar_sdk")
    if not isinstance(sdk, dict):
        raise AssertionError("manifest foobar_sdk must be an object")
    for key in ("release_date", "source"):
        if not isinstance(sdk.get(key), str) or not sdk[key]:
            raise AssertionError(f"manifest foobar_sdk.{key} is missing")
    for key in ("sdk_project_git_blob", "pfc_project_git_blob"):
        value = sdk.get(key)
        if not isinstance(value, str) or not HEX40.fullmatch(value):
            raise AssertionError(f"manifest foobar_sdk.{key} must be 40 hex")

    _require_commit_pin(manifest, "libvgm")
    _require_commit_pin(manifest, "wtl")
    _require_commit_pin(manifest, "spcplay")
    _require_commit_pin(manifest, "omniphony")
    omniphony = manifest["omniphony"]
    assert isinstance(omniphony, dict)
    rust_toolchain = omniphony.get("rust_toolchain")
    if not isinstance(rust_toolchain, str) or not rust_toolchain.strip():
        raise AssertionError("manifest omniphony.rust_toolchain is missing")


def _package_members(package_bytes: bytes, label: str) -> dict[str, bytes]:
    try:
        package = zipfile.ZipFile(io.BytesIO(package_bytes), "r")
    except zipfile.BadZipFile as exc:
        raise AssertionError(f"embedded {label} component archive is invalid") from exc
    with package:
        members: dict[str, bytes] = {}
        for info in package.infolist():
            if info.is_dir():
                continue
            pure = PurePosixPath(info.filename)
            if len(pure.parts) != 1 or ".." in pure.parts:
                raise AssertionError(
                    f"embedded {label} component contains nested/unsafe member: {info.filename}"
                )
            name = pure.name
            if name.casefold() in {existing.casefold() for existing in members}:
                raise AssertionError(
                    f"embedded {label} component contains duplicate member: {name}"
                )
            members[name] = package.read(info)
        return members


def _verify_runtime_copies(archive: zipfile.ZipFile) -> None:
    packages = {
        VGM_COMPONENT: _package_members(archive.read(VGM_COMPONENT), "VGM"),
        SPC_COMPONENT: _package_members(archive.read(SPC_COMPONENT), "SPC"),
    }
    for bundle_path, (package_name, member_name) in RUNTIME_PACKAGE_MEMBERS.items():
        package_members = packages[package_name]
        if member_name not in package_members:
            raise AssertionError(
                f"{package_name} is missing runtime member required by bundle: {member_name}"
            )
        if archive.read(bundle_path) != package_members[member_name]:
            raise AssertionError(
                f"manual bundle runtime copy differs from component payload: {bundle_path}"
            )


def verify_bundle_metadata(bundle: Path) -> dict[str, object]:
    if not bundle.is_file():
        raise RuntimeError(f"private bundle missing: {bundle}")
    if not zipfile.is_zipfile(bundle):
        raise RuntimeError(f"private bundle is not a ZIP archive: {bundle}")

    with zipfile.ZipFile(bundle, "r") as archive:
        names = _safe_bundle_names(archive)
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
        _verify_manifest(manifest)

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

        _verify_runtime_copies(archive)

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
        f"{manifest['retro_vgm_compiler_commit']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
