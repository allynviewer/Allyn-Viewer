#!/usr/bin/env python3
"""Audit PT XUI files for real UI issues.

Checks:
  [ENCODING] invalid UTF-8
  [MOJIBAKE] double-encoded UTF-8
  [XML] malformed XML
  [LONG] fixed-width UI strings that overflow the English layout

Skips false positives:
  - notifications.xml / strings.xml / teleport_strings.xml / ui_strings.xml
  - <url> elements
  - elements whose EN counterpart has word_wrap=true
  - tooltips / help strings
  - already-broken multiline text whose longest line is <= 72
"""
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

sys.stdout.reconfigure(encoding="utf-8")

XUI = Path(__file__).resolve().parents[1] / "indra/newview/skins/default/xui"
PT = XUI / "pt"
EN = XUI / "en-us"

MOJIBAKE = re.compile(
    "\u00c3[\u0080-\u00bf\u2018\u2019\u201c\u201d\u0160\u0161\u00a1-\u00bf]"
    "|\u00e2\u20ac|\u00c2[\u00a0-\u00bf]"
)

SKIP_LONG_FILES = {
    "notifications.xml",
    "strings.xml",
    "teleport_strings.xml",
    "ui_strings.xml",
}

NO_WRAP_TAGS = {
    "check_box", "button", "spinner", "radio_item", "combo_item",
    "menu_item_call", "menu_item_check", "column",
}


def max_line_len(s: str) -> int:
    parts = re.split(r"[\n\r]|&#10;", s)
    return max((len(p.strip()) for p in parts), default=0)


def index_en(root):
    """Map (tag, name) -> (text_or_label, word_wrap)."""
    out = {}
    for el in root.iter():
        tag = el.tag.split("}")[-1] if "}" in el.tag else el.tag
        name = el.get("name") or ""
        wrap = (el.get("word_wrap") or "").lower() in ("true", "1")
        if el.get("label") is not None:
            out[(tag, name, "label")] = (el.get("label") or "", wrap)
        if el.text and el.text.strip():
            out[(tag, name, "text")] = (el.text, wrap)
    return out


def collect_pt(root):
    out = []
    for el in root.iter():
        tag = el.tag.split("}")[-1] if "}" in el.tag else el.tag
        name = el.get("name") or ""
        if tag == "url":
            continue
        if "tooltip" in name.lower() or "help" in name.lower():
            continue

        label = el.get("label")
        if label and tag in NO_WRAP_TAGS:
            out.append((tag, name, "label", label))

        # Only <text> in panels (not <string> message templates — those wrap).
        if tag == "text" and el.text and el.text.strip():
            out.append((tag, name, "text", el.text))
    return out


def main() -> int:
    problems = 0
    for path in sorted(PT.rglob("*.xml")):
        rel = path.relative_to(PT)
        try:
            raw = path.read_text(encoding="utf-8")
        except UnicodeDecodeError as e:
            print(f"[ENCODING] {rel}: {e}")
            problems += 1
            continue

        for m in MOJIBAKE.finditer(raw):
            line = raw[: m.start()].count("\n") + 1
            ctx = raw[max(0, m.start() - 30): m.start() + 30].replace("\n", " ")
            print(f"[MOJIBAKE] {rel}:L{line}: ...{ctx}...")
            problems += 1

        try:
            pt_root = ET.fromstring(raw.encode("utf-8"))
        except ET.ParseError as e:
            print(f"[XML] {rel}: {e}")
            problems += 1
            continue

        if rel.name in SKIP_LONG_FILES:
            continue

        en_path = EN / rel
        if not en_path.exists():
            continue
        try:
            en_root = ET.parse(en_path).getroot()
        except ET.ParseError:
            continue

        en_idx = index_en(en_root)

        for tag, name, kind, s in collect_pt(pt_root):
            en_s, en_wrap = en_idx.get((tag, name, kind), ("", False))
            if en_wrap:
                continue  # layout wraps — length is fine

            pt_len = max_line_len(s)
            en_len = max_line_len(en_s) if en_s else 0

            # Multilevel texts already broken: only care about long lines
            has_break = ("\n" in s) or ("&#10;" in s)
            if has_break:
                if pt_len <= 72:
                    continue
                if en_len and pt_len <= en_len + 12:
                    continue
            else:
                if pt_len < 55:
                    continue
                if en_len and pt_len <= en_len * 1.25 + 6:
                    continue
                if not en_len and pt_len < 66:
                    continue

            preview = re.sub(r"\s+", " ", s).strip()[:90]
            print(f"[LONG] {rel} <{tag} name={name}> {kind} "
                  f"pt={pt_len} en={en_len}: {preview}")
            problems += 1

    print(f"\n{problems} problema(s) reais encontrados")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
