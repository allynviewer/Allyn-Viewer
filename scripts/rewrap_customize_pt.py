#!/usr/bin/env python3
"""Rewrap floater_customize 'not worn instructions' to fit width=284 (~45 cols)."""
import re
from pathlib import Path

PATH = Path(__file__).resolve().parents[1] / "indra/newview/skins/default/xui/pt/floater_customize.xml"
WIDTH = 45


def wrap(s: str, width: int = WIDTH) -> str:
    words = s.split()
    lines, cur = [], []
    for w in words:
        trial = " ".join(cur + [w]) if cur else w
        if len(trial) <= width:
            cur.append(w)
        else:
            if cur:
                lines.append(" ".join(cur))
            cur = [w]
    if cur:
        lines.append(" ".join(cur))
    return "\n".join(lines)


def main() -> None:
    text = PATH.read_text(encoding="utf-8")

    def repl(m: re.Match) -> str:
        body = " ".join(m.group(1).split())
        wrapped = wrap(body)
        indented = wrapped.replace("\n", "\n\t\t\t\t")
        return f'<text name="not worn instructions">\n\t\t\t\t{indented}\n\t\t\t</text>'

    new, n = re.subn(
        r'<text name="not worn instructions">\s*(.*?)\s*</text>',
        repl,
        text,
        flags=re.S,
    )
    PATH.write_text(new, encoding="utf-8", newline="\n")
    print(f"rewrapped {n} blocks")
    for m in re.finditer(
        r'<text name="not worn instructions">\s*(.*?)\s*</text>', new, re.S
    ):
        mx = max((len(l.strip()) for l in m.group(1).splitlines() if l.strip()), default=0)
        if mx > 48:
            print("STILL LONG", mx, m.group(1)[:80].replace("\n", " "))


if __name__ == "__main__":
    main()
