import json
import pathlib

root = pathlib.Path(__file__).resolve().parent.parent
examples = {p.name: p.read_text(encoding="utf-8") for p in sorted((root / "tests" / "data").glob("*.txt"))}
out = "window.CLIP_EXAMPLES = " + json.dumps(examples, ensure_ascii=False, indent=2) + ";\n"
(root.parent / "lab5_web" / "examples.js").write_text(out, encoding="utf-8")
print(f"lab5_web/examples.js: {len(examples)} примеров")
