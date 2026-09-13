#!/usr/bin/env python3
"""List PT preference labels/texts too long for the prefs window (~500px)."""
import re
import sys
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")

ROOT = Path(__file__).resolve().parents[1] / "indra/newview/skins/default/xui"
LIMIT = int(sys.argv[1]) if len(sys.argv) > 1 else 66


def main() -> int:
    pt = ROOT / "pt"
    files = sorted(pt.glob("panel_preferences*.xml"))
    files += [pt / "floater_preferences.xml"]
    for path in files:
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8")
        hits = []
        for m in re.finditer(r'label="([^"]{%d,})"' % LIMIT, text):
            line = text[: m.start()].count("\n") + 1
            hits.append((line, "label", m.group(1)))
        for m in re.finditer(r">([^<>{}\n]{%d,})<" % LIMIT, text):
            line = text[: m.start()].count("\n") + 1
            hits.append((line, "text", m.group(1).strip()))
        if hits:
            print("===", path.name)
            for line, kind, s in sorted(hits):
                print(f"  L{line} {kind} ({len(s)}): {s}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
