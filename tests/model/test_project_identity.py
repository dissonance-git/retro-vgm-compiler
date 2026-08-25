from pathlib import Path
import importlib.util


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "check_project_identity.py"


def _load_tool():
    spec = importlib.util.spec_from_file_location("check_project_identity", TOOL)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_current_repository_has_no_deprecated_project_name():
    module = _load_tool()
    assert module.scan_repository(ROOT) == []


def test_scanner_finds_each_deprecated_name_without_embedding_it_in_this_test(tmp_path):
    module = _load_tool()
    deprecated = (
        "Game" + " Music" + " Interpreter",
        "Retro" + " VGM" + " Compiler",
    )
    for index, name in enumerate(deprecated):
        (tmp_path / f"old-{index}.md").write_text(f"# {name}\n", encoding="utf-8")
    findings = module.scan_repository(tmp_path)
    assert len(findings) == 2
    assert {finding[0] for finding in findings} == {Path("old-0.md"), Path("old-1.md")}


def test_scanner_allows_historical_lineage(tmp_path):
    module = _load_tool()
    deprecated = "Retro" + " VGM" + " Compiler"
    history = tmp_path / "docs" / "history"
    history.mkdir(parents=True)
    (history / "identity.md").write_text(f"# {deprecated}\n", encoding="utf-8")
    assert module.scan_repository(tmp_path) == []
