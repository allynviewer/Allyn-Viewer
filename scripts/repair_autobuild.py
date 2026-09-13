#!/usr/bin/env python3
"""\
@file repair_autobuild.py
@brief Repair corrupted autobuild.xml hash/url entries.

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

# Build url -> hash map from SL
sl_pat = re.compile(
    r"<key>hash</key>\s*<string>([a-f0-9]{40})</string>\s*"
    r"<key>hash_algorithm</key>\s*<string>sha1</string>\s*"
    r"<key>url</key>\s*<string>(https://github\.com/secondlife/[^<]+)</string>"
)
sl_hashes = {m.group(2): m.group(1) for m in sl_pat.finditer(sl)}

# Fix merged hash+url in local file
merged_pat = re.compile(
    r"(<key>hash</key>\s*<string>)([a-f0-9]{40})(https://github\.com/secondlife/[^<]+)(</string>\s*"
    r"<key>hash_algorithm</key>\s*<string>sha1</string>\s*"
    r"<key>url</key>\s*<string>)(</string>)"
)

def fix_merged(m):
    url = m.group(3)
    h = sl_hashes.get(url)
    if not h:
        # fallback: use hash from merged string if present
        h = m.group(2)
        print(f"WARN: no SL hash for {url}, keeping {h}")
    else:
        print(f"Fixed: {url.rsplit('/',1)[-1]}")
    return m.group(1) + h + m.group(4) + url + m.group(5)

fixed, count = merged_pat.subn(fix_merged, local)
open(LOCAL_PATH, "w", newline="\n").write(fixed)
print(f"Repaired {count} entries")

# Also fix windows64 entries that have URL in hash but empty url (libpng/libxml2 style)
url_only_pat = re.compile(
    r"(<key>hash</key>\s*<string>)(https://github\.com/secondlife/[^<]+)(</string>\s*"
    r"<key>hash_algorithm</key>\s*<string>sha1</string>\s*"
    r"<key>url</key>\s*<string>)(https://github\.com/secondlife/[^<]+)(</string>)"
)

def fix_url_in_hash(m):
    url = m.group(4)
    h = sl_hashes.get(url, "")
    if not h:
        print(f"WARN: cannot fix url-in-hash for {url}")
        return m.group(0)
    print(f"Fixed url-in-hash: {url.rsplit('/',1)[-1]}")
    return m.group(1) + h + m.group(3) + url + m.group(5)

fixed2, count2 = url_only_pat.subn(fix_url_in_hash, fixed)
if count2:
    open(LOCAL_PATH, "w", newline="\n").write(fixed2)
    print(f"Repaired {count2} url-in-hash entries")
