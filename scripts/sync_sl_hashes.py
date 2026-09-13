#!/usr/bin/env python3
"""\
@file sync_sl_hashes.py
@brief Sync SHA1 hashes from official SL autobuild.xml.

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

SL_URL = "https://raw.githubusercontent.com/secondlife/viewer/develop/autobuild.xml"
LOCAL_PATH = r"d:\AllynViewer\autobuild.xml"

sl = urllib.request.urlopen(SL_URL).read().decode()
local = open(LOCAL_PATH).read()

url_pat = re.compile(
    r"(<key>hash</key>\s*<string>)([a-f0-9]{40})(</string>\s*"
    r"<key>hash_algorithm</key>\s*<string>sha1</string>\s*"
    r"<key>url</key>\s*<string>)(https://github\.com/secondlife/[^<]+)(</string>)"
)

sl_hashes = {
    m.group(3): m.group(2) for m in url_pat.finditer(sl)
}

def replacer(m):
    url = m.group(3)
    if url in sl_hashes:
        return m.group(1) + sl_hashes[url] + m.group(4) + url + m.group(5)
    return m.group(0)

updated, count = url_pat.subn(replacer, local)
if count:
    open(LOCAL_PATH, "w", newline="\n").write(updated)
    print(f"Updated {count} hash entries")
else:
    print("No updates needed")
