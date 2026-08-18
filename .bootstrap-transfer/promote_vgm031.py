from __future__ import annotations

import base64
import hashlib
from pathlib import Path, PurePosixPath
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TRANSFER = ROOT / ".bootstrap-transfer"
ARCHIVE = ROOT / "imports" / "foo_input_vgm-0.31.zip"
ARCHIVE_SIZE = 66250
ARCHIVE_SHA256 = "e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1"
TREE_SHA256 = "36a25ee0cc5d9e8df6c7f7f3f0f06ce305dbbe27dbf8abbc28caa97a8ddb64fc"
BASE64_SIZE = 88336
BASE64_SHA256 = "e07742de1f5f5c2b5e8e5d58a7cb2710374615382afbc7c77dfe6c2961cf838b"

PARTS = (
    ("part00", 14000, "6d8932dd910ba94fb918e067d2f698c2e22c44c94cbc29a6ed519c6444806dfa"),
    ("part01", 14000, "63cd6680e97d5493f374b67f584c04b9069835c715618543fe51659f7b92ee4f"),
    ("part02", 14000, "a155a352178a78ab2f40790fbcf35a2ad318465875ef7c7687c5e344165b2129"),
    ("part03a", 10000, "a3feeae7692a983c98ddfad2d93ac6204ab0b407563a340e46a619f075e1d0f3"),
    ("part03b", 10000, "f5709ef1a0357a399b7a7b10f2e1157fd73387156c4efb5d493dcd8a8f06016f"),
    ("part04", 4000, "3763b8e6f25a057507736f159a7870637227c5ba7900cb70b460a8ac166a8515"),
    ("part05", 4000, "a5273a6768b4dfc4843782c3db6db9d6772f835d7461e2394cfd0cf23280fd61"),
    ("part06", 4000, "d4d5996ccc2d7a518963d5bad85242df8047f9ca6eb2b33a721b86ec6811c593"),
    ("part07", 4000, "6c2996f72632aad43a385889a8cc2eccf8790b06729ac02ef044f391b35709e4"),
    ("part08", 4000, "394751ebf24bd0d799086c1ea55a86c480a20c25c30c173543d27afdecfa4023"),
    ("part09", 4000, "67edb938f43af9060a06891b29e9b9e7db5a75cd75e504459c50ed880deb6331"),
    ("part10", 2336, "0829b0600b4f8d3e69c8015aa085f9ac0d3d1fe90dd8897fcf46b908ff65e0cc"),
)


def sha256(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one match, found {count}")
    return text.replace(old, new, 1)


def reconstruct_archive() -> None:
    pieces: list[bytes] = []
    for name, expected_size, expected_sha in PARTS:
        path = TRANSFER / f"foo_input_vgm-0.31.b64.{name}"
        payload = path.read_bytes()
        actual_sha = sha256(payload)
        print(f"{name}: size={len(payload)} sha256={actual_sha}")
        if len(payload) != expected_size or actual_sha != expected_sha:
            raise RuntimeError(f"{name} transfer mismatch")
        pieces.append(payload)

    encoded = b"".join(pieces)
    if len(encoded) != BASE64_SIZE or sha256(encoded) != BASE64_SHA256:
        raise RuntimeError(
            f"full base64 mismatch: size={len(encoded)} sha256={sha256(encoded)}"
        )
    decoded = base64.b64decode(encoded, validate=True)
    if len(decoded) != ARCHIVE_SIZE or sha256(decoded) != ARCHIVE_SHA256:
        raise RuntimeError(
            f"archive mismatch: size={len(decoded)} sha256={sha256(decoded)}"
        )
    ARCHIVE.write_bytes(decoded)
    print(f"verified archive: size={len(decoded)} sha256={sha256(decoded)}")


def verify_source_tree() -> None:
    with zipfile.ZipFile(ARCHIVE) as archive:
        names = [name for name in archive.namelist() if not name.endswith("/")]
        markers = [
            name
            for name in names
            if name.endswith("foo_input_vgm/src/my_component_client.cpp")
        ]
        if len(markers) != 1:
            raise RuntimeError(f"expected one version marker, found {markers}")
        marker = PurePosixPath(markers[0])
        root = marker.parents[1].as_posix().rstrip("/") + "/"
        entries: list[tuple[str, str]] = []
        for name in sorted(candidate for candidate in names if candidate.startswith(root)):
            relative = name[len(root) :]
            if relative:
                entries.append((relative, sha256(archive.read(name))))
        manifest = "".join(
            f"{digest}  {relative}\n" for relative, digest in entries
        ).encode("utf-8")
        tree_sha = sha256(manifest)
        version = archive.read(markers[0]).decode("utf-8-sig")
        print(f"source tree: files={len(entries)} sha256={tree_sha}")
        if len(entries) != 41:
            raise RuntimeError(f"expected 41 source files, got {len(entries)}")
        if tree_sha != TREE_SHA256:
            raise RuntimeError(f"source-tree mismatch: {tree_sha}")
        if '"0.31"' not in version:
            raise RuntimeError("component version marker is not 0.31")


def patch_builder() -> None:
    path = ROOT / "tools" / "build_private_foobar_components.ps1"
    text = path.read_text(encoding="utf-8")
    text = replace_once(
        text,
        "$VgmBootstrapUrl = 'https://uu.getuploader.com/foobar2000/download/248'\n$VgmBootstrapSha256 = '93d71695fdad062dee47aefa3f857683e4a057302d1a069958eecf5dd18c60ff'",
        "$VgmBootstrapVersion = '0.31'\n$VgmBootstrapSource = 'imports/foo_input_vgm-0.31.zip'\n$VgmBootstrapArchiveSha256 = 'e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1'\n$VgmBootstrapTreeSha256 = '36a25ee0cc5d9e8df6c7f7f3f0f06ce305dbbe27dbf8abbc28caa97a8ddb64fc'",
        "VGM bootstrap constants",
    )
    text = replace_once(
        text,
        "$VgmBootstrap = Join-Path $WorkRoot 'foo_input_vgm_v0.30.7z'",
        "$VgmBootstrap = Join-Path $RetroRoot 'imports\\foo_input_vgm-0.31.zip'",
        "VGM bootstrap path",
    )
    text = replace_once(
        text,
        "Write-Host '== 0. Recover and verify the historical foo_input_vgm bootstrap =='\n& (Join-Path $RetroRoot 'tools\\fetch_foo_input_vgm_bootstrap.ps1') -OutputPath $VgmBootstrap -DownloadPage $VgmBootstrapUrl -ExpectedSha256 $VgmBootstrapSha256\n$env:RETRO_VGM_BOOTSTRAP_ARCHIVE = $VgmBootstrap",
        "Write-Host '== 0. Verify the in-repository foo_input_vgm 0.31 bootstrap =='\nRequire-File $VgmBootstrap 'foo_input_vgm 0.31 bootstrap archive'\n$actualVgmBootstrapSha256 = (Get-FileHash -LiteralPath $VgmBootstrap -Algorithm SHA256).Hash.ToLowerInvariant()\nif ($actualVgmBootstrapSha256 -ne $VgmBootstrapArchiveSha256) { throw \"foo_input_vgm 0.31 archive drift: expected $VgmBootstrapArchiveSha256, got $actualVgmBootstrapSha256\" }\n$env:RETRO_VGM_BOOTSTRAP_ARCHIVE = $VgmBootstrap\n$env:RETRO_VGM_BOOTSTRAP_TREE_SHA256 = $VgmBootstrapTreeSha256",
        "VGM bootstrap stage",
    )
    text = replace_once(
        text,
        "foo_input_vgm_bootstrap = [ordered]@{ source = $VgmBootstrapUrl; sha256 = $VgmBootstrapSha256 }",
        "foo_input_vgm_bootstrap = [ordered]@{ version = $VgmBootstrapVersion; source = $VgmBootstrapSource; archive_sha256 = $VgmBootstrapArchiveSha256; source_tree_sha256 = $VgmBootstrapTreeSha256 }",
        "VGM manifest record",
    )
    path.write_text(text, encoding="utf-8", newline="\n")


def patch_materializer() -> None:
    path = ROOT / "tools" / "materialize_foo_input_vgm.py"
    text = path.read_text(encoding="utf-8")
    text = replace_once(
        text,
        "import argparse\nfrom pathlib import Path",
        "import argparse\nimport hashlib\nimport os\nfrom pathlib import Path",
        "materializer imports",
    )
    text = replace_once(
        text,
        "import tempfile\n\n\n# These files are project-owned additions.",
        "import tempfile\n\n\nDEFAULT_BOOTSTRAP_VERSION = \"0.31\"\nDEFAULT_BOOTSTRAP_TREE_SHA256 = \"36a25ee0cc5d9e8df6c7f7f3f0f06ce305dbbe27dbf8abbc28caa97a8ddb64fc\"\n\n\n# These files are project-owned additions.",
        "materializer constants",
    )
    helper = '''\n\ndef source_tree_sha256(root: Path) -> str:\n    entries: list[tuple[str, str]] = []\n    for path in sorted(candidate for candidate in root.rglob("*") if candidate.is_file()):\n        relative = path.relative_to(root).as_posix()\n        digest = hashlib.sha256(path.read_bytes()).hexdigest()\n        entries.append((relative, digest))\n    manifest = "".join(f"{digest}  {relative}\\n" for relative, digest in entries).encode("utf-8")\n    return hashlib.sha256(manifest).hexdigest()\n\n\ndef require_bootstrap_identity(root: Path, expected_tree_sha256: str) -> None:\n    actual = source_tree_sha256(root)\n    expected = expected_tree_sha256.strip().lower()\n    if actual != expected:\n        raise RuntimeError(\n            "foo_input_vgm bootstrap source-tree mismatch: "\n            f"expected {expected}, got {actual}"\n        )\n    version_file = root / "src" / "my_component_client.cpp"\n    if not version_file.is_file():\n        raise RuntimeError(f"foo_input_vgm bootstrap version file missing: {version_file}")\n    version_text = version_file.read_text(encoding="utf-8-sig", errors="strict")\n    if f'"{DEFAULT_BOOTSTRAP_VERSION}"' not in version_text:\n        raise RuntimeError(f"foo_input_vgm bootstrap is not version {DEFAULT_BOOTSTRAP_VERSION}")\n'''
    text = replace_once(text, "\n\ndef main() -> int:", helper + "\n\ndef main() -> int:", "materializer helper")
    text = replace_once(
        text,
        'archive = (selected_archive or (repo / "imports" / "foo_input_vgm.7z")).resolve()',
        'archive = (selected_archive or (repo / "imports" / "foo_input_vgm-0.31.zip")).resolve()',
        "materializer default archive",
    )
    text = replace_once(
        text,
        '        require_files(bootstrap, REQUIRED_BOOTSTRAP_FILES, "foo_input_vgm bootstrap")\n        shutil.copytree(bootstrap, component)',
        '        require_files(bootstrap, REQUIRED_BOOTSTRAP_FILES, "foo_input_vgm bootstrap")\n        expected_tree = os.environ.get("RETRO_VGM_BOOTSTRAP_TREE_SHA256", DEFAULT_BOOTSTRAP_TREE_SHA256)\n        require_bootstrap_identity(bootstrap, expected_tree)\n        print(f"verified foo_input_vgm {DEFAULT_BOOTSTRAP_VERSION} source tree: {expected_tree.lower()}")\n        shutil.copytree(bootstrap, component)',
        "materializer identity gate",
    )
    text = text.replace(
        "The historical upstream plugin is preserved immutably in imports/foo_input_vgm.7z.\nCurrent Retro VGM Compiler additions live under components/vgm/ and the guarded\ntransformations live under patches/foo_input_vgm/.  This tool combines those\nthree sources into a disposable build tree without consulting the retired\nvgmspc repository or copying its stale patched host tree.",
        "The pristine foo_input_vgm 0.31 source is preserved as a verified in-repository bootstrap archive.\nCurrent Retro VGM Compiler additions live under components/vgm/ and the guarded\ntransformations live under patches/foo_input_vgm/. This tool verifies the exact\nbootstrap source tree, combines it with those project-owned additions, and builds\na disposable tree without consulting the retired vgmspc repository.",
    )
    path.write_text(text, encoding="utf-8", newline="\n")


def patch_manifest() -> None:
    path = ROOT / "imports" / "MANIFEST.md"
    text = path.read_text(encoding="utf-8")
    old = (
        "The archive identity is immutable, but the current GitHub transport copy is known to be truncated and is not accepted as a build input. "
        "Canonical Windows builds recover `foo_input_vgm_v0.30.7z` from `https://uu.getuploader.com/foobar2000/download/248` into disposable build state and require the SHA-256 above before extraction. "
        "This is a recovery route for the same audited source object, not permission to substitute v0.31 or another release. Normal development occurs from the expanded source under `components/vgm/`; the historical archive is never edited or repacked."
    )
    new = (
        "The original 0.30 archive identity remains immutable historical provenance. Its repository transport copy is truncated and the former network recovery route no longer reproduces the recorded bytes, so it is not a production build input.\n\n"
        "### Current VGM build baseline: foo_input_vgm 0.31\n\n"
        "- Repository archive: `imports/foo_input_vgm-0.31.zip`\n"
        "- Exact supplied archive size: `66,250` bytes\n"
        "- Exact supplied archive SHA-256: `e2c08ee82b10efd3b31f2304d0c9a7c0f5eae0e07a241e91108c81c3bedd01e1`\n"
        "- Exact source identity: `41` files rooted at `foo_input_vgm/`, canonical source-tree SHA-256 `36a25ee0cc5d9e8df6c7f7f3f0f06ce305dbbe27dbf8abbc28caa97a8ddb64fc`\n"
        "- Component version marker: `0.31`\n"
        "- License in supplied source: Mozilla Public License 2.0\n\n"
        "Canonical Windows builds use the in-repository 0.31 archive directly. They verify the exact archive SHA before extraction, and the materializer independently verifies the extracted source-tree digest and version before applying any project-owned overlay or guarded patch."
    )
    path.write_text(replace_once(text, old, new, "manifest 0.30 paragraph"), encoding="utf-8", newline="\n")


def main() -> None:
    reconstruct_archive()
    verify_source_tree()
    patch_builder()
    patch_materializer()
    patch_manifest()
    print("foo_input_vgm 0.31 bootstrap is ready for repository validation")


if __name__ == "__main__":
    main()
