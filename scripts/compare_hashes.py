#!/usr/bin/env python3
"""\
@file compare_hashes.py
@brief Compare package hashes with official SL autobuild.xml.

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
import urllib.request

sl = urllib.request.urlopen(
    "https://raw.githubusercontent.com/secondlife/viewer/develop/autobuild.xml"
).read().decode()
local = open(r"d:\AllynViewer\autobuild.xml").read()

url_pat = re.compile(
    r"<key>hash</key>\s*<string>([a-f0-9]{40})</string>\s*"
    r"<key>hash_algorithm</key>\s*<string>sha1</string>\s*"
    r"<key>url</key>\s*<string>(https://github\.com/secondlife/[^<]+)</string>"
)

def by_url(text):
    return {m.group(2): m.group(1) for m in url_pat.finditer(text)}

sl_map = by_url(sl)
local_map = by_url(local)

for url, lhash in sorted(local_map.items()):
    shash = sl_map.get(url)
    fname = url.rsplit("/", 1)[-1]
    if shash is None:
        print(f"NO SL HASH {fname}: local={lhash}")
    elif shash != lhash:
        print(f"MISMATCH {fname}")
        print(f"  local={lhash}")
        print(f"  sl   ={shash}")
