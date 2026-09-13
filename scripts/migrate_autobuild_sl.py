#!/usr/bin/env python3
"""\
@file migrate_autobuild_sl.py
@brief Migrate autobuild.xml packages from official SL viewer.

Copyright (c) 2025-2026, Allyn Viewer Contributors.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
"""

import re
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
AUTOBUILD = ROOT / "autobuild.xml"
SL_URL = "https://raw.githubusercontent.com/secondlife/viewer/develop/autobuild.xml"
SL_CACHE = Path(__file__).resolve().parent / "sl_autobuild.xml"

# Allyn package key -> SL package key (when different)
PKG_MAP = {
    "openal": "openal",
    "boost": "boost",
    "colladadom": "colladadom",
    "freetype": "freetype",
    "libpng": "libpng",
    "libxml2": "libxml2",
    "libhunspell": "libhunspell",
    "ogg_vorbis": "ogg_vorbis",
    "nvapi": "nvapi",
    "vlc-bin": "vlc-bin",
    "zlib": "zlib-ng",
    "jpeglib": "libjpeg-turbo",
    "crashpad": "bugsplat",
}

EXTRA_META = {
    "openal": {
        "license_file": "LICENSES/openal-soft.txt",
        "description": "OpenAL Soft is a software implementation of the OpenAL 3D audio API.",
    },
}


def fetch_sl() -> str:
    if SL_CACHE.exists() and SL_CACHE.stat().st_mtime > AUTOBUILD.stat().st_mtime - 86400:
        return SL_CACHE.read_text(encoding="utf-8")
    data = urllib.request.urlopen(SL_URL).read()
    SL_CACHE.write_bytes(data)
    return data.decode("utf-8")


def extract_package_block(text: str, pkg: str) -> str | None:
    marker = f"<key>{pkg}</key>"
    start = text.find(marker)
    if start == -1:
        return None
    map_start = text.find("<map>", start)
    if map_start == -1:
        return None
    depth = 0
    i = map_start
    while i < len(text):
        if text.startswith("<map>", i):
            depth += 1
            i += 5
            continue
        if text.startswith("</map>", i):
            depth -= 1
            end = i + 6
            i = end
            if depth == 0:
                return text[start:end]
            continue
        i += 1
    return None


def extract_windows64_archive(pkg_block: str) -> dict | None:
    m = re.search(
        r"<key>windows64</key>\s*<map>\s*<key>archive</key>\s*<map>(.*?)</map>",
        pkg_block,
        flags=re.S,
    )
    if not m:
        return None
    arch = m.group(1)

    def get(key):
        km = re.search(rf"<key>{key}</key>\s*<string>([^<]*)</string>", arch)
        return km.group(1) if km else None

    return {
        "hash": get("hash"),
        "hash_algorithm": get("hash_algorithm") or "sha1",
        "url": get("url"),
    }


def extract_version(pkg_block: str) -> str | None:
    m = re.search(r"<key>version</key>\s*<string>([^<]+)</string>", pkg_block)
    return m.group(1) if m else None


def patch_windows64_block(pkg_body: str, meta: dict) -> str:
    archive = (
        f"<key>hash</key>\n              <string>{meta['hash']}</string>\n"
        f"              <key>hash_algorithm</key>\n              <string>{meta['hash_algorithm']}</string>\n"
        f"              <key>url</key>\n              <string>{meta['url']}</string>"
    )
    pkg_body = re.sub(
        r"<key>windows64</key>\s*<map>\s*<key>archive</key>\s*<map>.*?</map>\s*<key>name</key>\s*<string>windows64</string>\s*</map>",
        "<key>windows64</key>\n          <map>\n            <key>archive</key>\n            <map>\n"
        + archive
        + "\n            </map>\n            <key>name</key>\n            <string>windows64</string>\n          </map>",
        pkg_body,
        count=1,
        flags=re.S,
    )
    if meta.get("version"):
        pkg_body = re.sub(
            r"(<key>version</key>\s*<string>)[^<]+(</string>)",
            rf"\g<1>{meta['version']}\g<2>",
            pkg_body,
            count=1,
        )
    if meta.get("license_file"):
        if re.search(r"<key>license_file</key>", pkg_body):
            pkg_body = re.sub(
                r"(<key>license_file</key>\s*<string>)[^<]+(</string>)",
                rf"\g<1>{meta['license_file']}\g<2>",
                pkg_body,
                count=1,
            )
        else:
            pkg_body = re.sub(
                r"(<key>license</key>\s*<string>[^<]+</string>)",
                rf"\g<1>\n        <key>license_file</key>\n        <string>{meta['license_file']}</string>",
                pkg_body,
                count=1,
            )
    if meta.get("description"):
        if re.search(r"<key>description</key>", pkg_body):
            pkg_body = re.sub(
                r"(<key>description</key>\s*<string>)[^<]+(</string>)",
                rf"\g<1>{meta['description']}\g<2>",
                pkg_body,
                count=1,
            )
    return pkg_body


def remove_fmodstudio(text: str) -> str:
    block = extract_package_block(text, "fmodstudio")
    if not block:
        return text
    return text.replace(block, "\n      ", 1)


def main() -> int:
    sl = fetch_sl()
    text = AUTOBUILD.read_text(encoding="utf-8")
    text = remove_fmodstudio(text)
    print("OK: removed fmodstudio")

    for local_pkg, sl_pkg in PKG_MAP.items():
        sl_block = extract_package_block(sl, sl_pkg)
        if not sl_block:
            print(f"WARN: SL package not found: {sl_pkg}", file=sys.stderr)
            continue
        arch = extract_windows64_archive(sl_block)
        version = extract_version(sl_block)
        if not arch or not arch.get("url") or not arch.get("hash"):
            print(f"WARN: no windows64 archive for SL {sl_pkg}", file=sys.stderr)
            continue

        meta = {**arch, "version": version, **EXTRA_META.get(local_pkg, {})}

        marker = f"<key>{local_pkg}</key>"
        start = text.find(marker)
        if start == -1:
            print(f"WARN: local package not found: {local_pkg}", file=sys.stderr)
            continue
        local_block = extract_package_block(text, local_pkg)
        if not local_block:
            print(f"WARN: could not parse local block: {local_pkg}", file=sys.stderr)
            continue

        patched = patch_windows64_block(local_block, meta)
        text = text[:start] + patched + text[start + len(local_block) :]
        print(f"OK: {local_pkg} <- {sl_pkg} ({version})")

    AUTOBUILD.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
