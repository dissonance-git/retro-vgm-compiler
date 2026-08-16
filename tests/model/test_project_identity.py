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


def test_scanner_finds_deprecated_name_without_embedding_it_in_this_test(tmp_path):
    module = _load_tool()
    deprecated = "Game" + " Music" + " Interpreter"
    (tmp_path / "README.md").write_text(f"# {deprecated}\n", encoding="utf-8")
    findings = module.scan_repository(tmp_path)
    assert len(findings) == 1
    assert findings[0][0] == Path("README.md")
    assert findings[0][1] == 1
