from pathlib import Path


def replace_once(path: str, old: str, new: str, label: str) -> None:
    target = Path(path)
    text = target.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one anchor in {path}, found {count}")
    target.write_text(text.replace(old, new, 1), encoding="utf-8")


replace_once(
    "components/README.md",
    "For repository-wide navigation, start at [`../CATALOG.md`](../CATALOG.md).",
    "For repository-wide navigation, start at the repository map in [`../README.md`](../README.md).",
    "components root navigation",
)
replace_once(
    "components/README.md",
    "1. Check `CATALOG.md` and the generated repository inventory.",
    "1. Check the root `README.md` repository map and the generated repository inventory.",
    "components navigation step",
)
replace_once(
    "docs/README.md",
    "For repository-wide navigation, start at [`../CATALOG.md`](../CATALOG.md).",
    "For repository-wide navigation, start at the repository map in [`../README.md`](../README.md).",
    "docs root navigation",
)
replace_once(
    "model/README.md",
    "For repository-wide navigation, start at [`../CATALOG.md`](../CATALOG.md).",
    "For repository-wide navigation, start at the repository map in [`../README.md`](../README.md).",
    "model root navigation",
)
replace_once(
    "research/README.md",
    "For repository-wide orientation, start at [`../CATALOG.md`](../CATALOG.md).",
    "For repository-wide orientation, start at the repository map in [`../README.md`](../README.md).",
    "research root navigation",
)
replace_once(
    "tools/README.md",
    "For repository-wide navigation, start at [`../CATALOG.md`](../CATALOG.md).",
    "For repository-wide navigation, start at the repository map in [`../README.md`](../README.md).",
    "tools root navigation",
)
replace_once(
    "tools/spc/README.md",
    "This shelf owns executable SPC research operations. For component semantics, see `../../components/spc/README.md`; for repository-wide navigation, see `../../CATALOG.md`.",
    "This shelf owns executable SPC research operations. For component semantics, see `../../components/spc/README.md`; for repository-wide navigation, see the repository map in `../../README.md`.",
    "SPC tools navigation",
)
replace_once(
    "tools/repository_catalog.py",
    "human-oriented navigation, read `CATALOG.md`.",
    "human-oriented navigation, read the repository map in root `README.md`.",
    "generated catalog human navigation",
)
