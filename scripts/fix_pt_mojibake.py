#!/usr/bin/env python3
"""Fix UTF-8 mojibake in Portuguese XUI XML files."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1] / "indra/newview/skins/default/xui/pt"


def fix_mojibake(s: str) -> str:
    if "\u00c3" not in s and "\u00e2" not in s:
        return s
    for encoding in ("latin-1", "cp1252"):
        try:
            fixed = s.encode(encoding).decode("utf-8")
            if fixed != s:
                return fixed
        except (UnicodeDecodeError, UnicodeEncodeError):
            continue
    return s


def fix_content(text: str) -> str:
    def repl_attr(m: re.Match) -> str:
        return '"' + fix_mojibake(m.group(1)) + '"'

    def repl_text(m: re.Match) -> str:
        return ">" + fix_mojibake(m.group(1)) + "<"

    text = re.sub(
        r'"([^"]*\u00c3.[^"]*)"',
        repl_attr,
        text,
    )
    text = re.sub(
        r">([^<]*\u00c3.[^<]*)<",
        repl_text,
        text,
    )
    return text


def main() -> int:
    changed = []
    for path in sorted(ROOT.rglob("*.xml")):
        try:
            raw = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            print(f"SKIP (invalid UTF-8): {path.relative_to(ROOT)}")
            continue
        if "\u00c3" not in raw and "\u00e2" not in raw:
            continue
        fixed = fix_content(raw)
        if fixed != raw:
            path.write_text(fixed, encoding="utf-8", newline="\n")
            changed.append(str(path.relative_to(ROOT)))

    print(f"Fixed {len(changed)} files:")
    for name in changed:
        print(f"  {name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
