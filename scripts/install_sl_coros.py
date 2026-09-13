#!/usr/bin/env python3
"""\
@file install_sl_coros.py
@brief Install SL coroutine sources adapted for Allyn's LLSingleton.

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

import shutil
from pathlib import Path

ROOT = Path(r"d:\AllynViewer\indra\llcommon")

REPLACEMENTS = {
    "llcoros.h": [
        (
            "    LLSINGLETON(LLCoros);\n    ~LLCoros();\n\n    void cleanupSingleton() override;",
            "    friend class LLSingleton<LLCoros>;\n    LLCoros();\n    ~LLCoros();",
        ),
    ],
    "llcoros.cpp": [
        ("wasDeleted()", "destroyed()"),
        (
            "    if (LLApp::instance()->reportCrashToBugsplat((void*)exception_infop))\n    {\n        // Handled\n        return EXCEPTION_CONTINUE_SEARCH;\n    }\n    else if (code == STATUS_MSC_EXCEPTION)",
            "    if (code == STATUS_MSC_EXCEPTION)",
        ),
        (
            "        for (auto& cd : CoroData::instance_snapshot())\n        {\n            F64 life_time = time - cd.mCreationTime;\n            LL_CONT << LL_NEWLINE\n                    << cd.getKey() << ' ' << cd.mStatus << \" life: \" << life_time;\n        }\n        LL_CONT << LL_ENDL;",
            "        LL_CONT << \" (detailed coroutine list requires instance_snapshot)\" << LL_ENDL;",
        ),
    ],
}

def install(name: str):
    src = ROOT / f"{name}.sl"
    dst = ROOT / name
    text = src.read_text(encoding="utf-8")
    for old, new in REPLACEMENTS.get(name, []):
        if old not in text:
            print(f"WARN: pattern missing in {name}")
        text = text.replace(old, new)
    if name == "llcoros.cpp":
        # Move cleanupSingleton body into destructor
        text = text.replace(
            "LLCoros::~LLCoros()\n{\n}",
            "LLCoros::~LLCoros()\n{\n    cleanupSingleton();\n}",
        )
        text = text.replace("void LLCoros::cleanupSingleton()", "void LLCoros::cleanupSingleton_()")
        text = text.replace(
            "void LLCoros::cleanupSingleton_()",
            "void LLCoros::cleanupSingleton()",
            1,
        )
    dst.write_text(text, encoding="utf-8")
    print(f"OK: {name}")

for f in ["llcoros.h", "llcoros.cpp", "lleventcoro.h", "lleventcoro.cpp"]:
    install(f)

# cleanup .sl staging files
for f in ROOT.glob("*.sl"):
    f.unlink()
