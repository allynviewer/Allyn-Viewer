# Allyn Viewer Build & Debug Notes

Agent-oriented notes for the **current** Windows 64-bit tree. Contributor-facing docs live in `doc/building_windows.md` and `CONTRIBUTING.md`.

## Build

- Platform: Windows 64-bit only (`build-vc-64`)
- Toolchain: Visual Studio 2022 (CI, `AUTOBUILD_VSVER=170`) or Visual Studio 2026 (`180`), MSVC, **Release**
- Official flags: `-DUSE_OPENAL:BOOL=ON -DUSE_NVAPI=ON -DUSE_LTO=ON`
- Do **not** use `RelWithDebInfo` on Windows — it builds with `/Od` (no optimization). Use `Release`.
- Python deps: `pip install -r requirements.txt` (`autobuild>=3.10.0`, `llsd>=1.2.0`)
- Commands (repo root):
  - First full build: `build.bat full`
  - Viewer-only rebuild: `build.bat viewer` → `autobuild build --no-configure … -- --target allyn-viewer-bin`
  - Full rebuild without clean configure: `build.bat build`
 - Smoke test: `build.bat smoke`
 - Packages: `build.bat package` → `build-vc-64\dist\Allyn_Viewer_<Type>_<ver>_x86_64.zip` (portable, always) + `..._Setup.exe` (NSIS, only when NSIS + the StdUtils plugin are installed; `scripts\setup-nsis-plugins.ps1` installs the plugin). Both come from `viewer_manifest.py` (`package_zip()` / `package_finish()`), same file list.
 - NSIS installer UI: `installers\windows\installer_template.nsi` + `allyn_theme.nsh` (Cyber theme: DWM dark caption, `DarkMode_Explorer` buttons/scrollbars/checkbox boxes, flat `WS_BORDER` instead of the light `WS_EX_CLIENTEDGE`, `SetCtlColors` for the rest — no hand-painted controls. Checkbox labels are `Static` overlays because the dark theme paints them black; clicks fall through via `HTTRANSPARENT`). The theme include emits `Function`s, so it must stay **after** `SetCompressor`. `lang_*.nsi` are UTF-8 without BOM: `viewer_manifest.py` passes `/INPUTCHARSET UTF8` to `makensis`; running `makensis` by hand without it garbles every accented `LangString`.
 - Visual C++ runtime (`msvcp140.dll`, `vcruntime140.dll`, `vcruntime140_1.dll`, …) is staged into `build-vc-64\sharedlibs\Release` by `Copy3rdPartyLibs.cmake` (`InstallRequiredSystemLibraries`, fallback `VCToolsRedistDir`) and packaged by `viewer_manifest.py`. The installer must **not** download/execute `vc_redist.exe` (INetC): that pattern triggers Windows Defender / SmartScreen heuristics on unsigned installers. `viewer_manifest.py` refuses to package when the runtime DLLs are missing → re-run configure.
 - Code signing (SignPath Foundation, `doc/code_signing.md`): only in CI. Pushing a `v*` tag makes `.github/workflows/build.yaml` build, sign `allyn-viewer-bin.exe` + `SLPlugin.exe` (request 1), package, sign the `_Setup.exe` (request 2) and publish/refresh the GitHub Release. Needs secrets `SIGNPATH_API_TOKEN` / `SIGNPATH_ORGANIZATION_ID`; each request is approved manually in SignPath. Signed files must keep `ProductName = "Allyn Viewer"` and the same `ProductVersion` (`viewerRes.rc.in`, `slplugin.rc.in`, `installer_template.nsi`).
 - Install missing tools (Git, CMake, Python, VS 2022 Build Tools, NSIS) via winget: `build.bat tools` (offered automatically in interactive mode)
  - `build.bat check` also warns about paths with spaces / long paths / cloud-synced folders and < 20 GB free disk
  - Run: `build.bat run`
- Executable: `build-vc-64\newview\Release\allyn-viewer-bin.exe` (mirrored to `build-vc-64\bin\Release\` by `build.bat`); packaged copies are renamed to `AllynViewer<Type>.exe` (`final_exe()` in `viewer_manifest.py`)
- Branding: `VIEWER_CHANNEL_BASE` (`Variables.cmake`) and `CHANNEL_VENDOR_BASE` (`llmanifest.py`) must both be `Allyn Viewer`; channel = `"Allyn Viewer <Type>"`, branding id `allyn-viewer`
- Build number = `git rev-list --count HEAD` (`-i` to autobuild); version = `indra/newview/VIEWER_VERSION.txt` + build number → `build-vc-64\newview\viewer_version.txt`. CI tag builds (`v<major>.<minor>.<patch>.<build>`) take the build number from the tag and fail if `major.minor.patch` differs from `VIEWER_VERSION.txt`, so a tag `v1.0.0.2` always yields `Allyn_Viewer_*_1_0_0_2_*` packages.
- Solution: `build-vc-64\Allyn.sln` (VS 2022) or `build-vc-64\Allyn.slnx` (VS 2026)
- Force one-file recompile: delete `build-vc-64\newview\allyn-viewer-bin.dir\Release\llappviewer.obj`
- Always drive builds through `build.bat` / autobuild (vcvars). Plain MSBuild from a normal shell can force a full viewer recompile.
- `build.bat` is localized: interactive mode asks for the language first; CLI mode uses `BUILD_LANG` (`de`, `en-us`, `es`, `fr`, `it`, `ja`, `pt`, `ru`, `tr`; default `en-us`). Strings live in `build-lang\<code>.cmd` (UTF-8 no BOM, no `!` `^` `%` `"` inside messages, `{0}`/`{1}` placeholders). New user-facing text in `build.bat` must be added to all nine files.
- `build.bat` runs under code page 65001; `set /p` returns empty when stdin is redirected under 65001, so scripted tests must pass commands as arguments, not via piped input.

## Audio

- Engine: **OpenAL Soft** (`github.com/secondlife/3p-openal-soft`)
- Runtime DLLs: `OpenAL32.dll`, `alut.dll`

## Dependencies

- Autobuild packages (`autobuild.xml`): mostly `github.com/secondlife/3p-*` windows64 archives
- FetchContent (`indra/deps/CMakeLists.txt`): fmt, nlohmann_json, abseil, c-ares, xmlrpc-epi (`3p-xmlrpc-epi`)
- xmlrpc-epi sources are filtered with a **relative** path glob; absolute-path filters break checkouts whose path contains `test` or `sample`

## Fonts

- Package: SL `3p-viewer-fonts`
- Primary UI font files: `DejaVuSans.ttf` (+ Bold / Oblique / Mono variants)
- `fonts.xml` and `viewer_manifest.py` must match those names
- Garbled UI glyphs → look for `Couldn't load font` or accidental use of `seguisym.ttf` as the main font

## Logging

Crash-only logging:

| Path | Meaning |
| ---- | ------- |
| `%APPDATA%\AllynViewer\logs\Allyn.exec_marker` | Written while the viewer runs / after a successful start |
| `%APPDATA%\AllynViewer\logs\Allyn.start_marker` | Start marker |
| `%APPDATA%\AllynViewer\logs\Allyn-debug\static_debug_info.log` | Build / GPU / version info at startup |
| `%APPDATA%\AllynViewer\logs\Allyn.log` | Written only on crash |

## Testing

- Preferred: `build.bat smoke` — starts `allyn-viewer-bin.exe`, waits `SMOKE_SECONDS` (default 45), checks the process is still alive and `Allyn.exec_marker` was updated, then kills the viewer
- Manual: `build.bat run` or start `build-vc-64\bin\Release\allyn-viewer-bin.exe`
- CI: `.github/workflows/build.yaml` (windows-2022, Release + OpenAL + NVAPI + LTO)

## Known crashes & fixes (still relevant)

### 1. aicurl assertion on startup

- Crash: `assertion failed: mCurlHandleMap.empty()` in `aicurl.cpp`
- Cause: `AICurlInterface::initCurl()` after `LLErrorThread` starts
- Fix: call `initCurl()` before `setupErrorHandling()` in `llappviewer.cpp`

### 2. packages-formatter.py UnicodeEncodeError

- Crash: `UnicodeEncodeError` on cp1252 terminals during build
- Fix: `sys.stdout.reconfigure(encoding='utf-8')` after the locale check

### 3. AMD Radeon RX 580 heap corruption (`0xC0000005`)

- Symptom: `FaultTolerantHeap` + `STATUS_ACCESS_VIOLATION` during startup
- Driver: AMD 22.20.27.09.230330 (Mar 2023) can corrupt the heap during OpenGL init
- First hit often: `LLViewerJoystick::init(false)` → `ndof_libinit()` → DirectInput
- Fix in `llappviewer.cpp`: SEH wrapper `safeJoystickInit()` (standalone function — no C++ objects with destructors inside `__try`/`__except`, MSVC C2712)

### 4. `lldir.cpp` getLindenUserDir before login

- Assert: `llassert(empty_ok || !mLindenUserDir.empty())`
- Current behavior: return `mOSUserAppDir` as fallback when `mLindenUserDir` is empty

### 5. `llvieweroctree.cpp` `mGroups.empty()` ASSERT

- Heap corruption can leave groups in `mGroups` during `~LLViewerOctreePartition()`
- Current behavior: warn and skip instead of hard assert

### 6. `pipeline.cpp` `GL_LIGHTING` checkEnabled ASSERT

- OpenGL state tracking can desync after AMD heap corruption
- Current behavior: check + re-enable (`glEnable(GL_LIGHTING)` and sync `staticData.currentState`) on the HW light and deferred/SSAO paths

### 7. `LL_PATH_PER_SL_ACCOUNT` before login

- Use `getLindenUserDir(true)` and return `""` when empty so pre-login paths do not point at `OSUserAppDir`

## Other notes

- CEF splash "Couldn't navigate": known, harmless
- `Failed to enable LFH for heap: 1 Error: 87`: normal on current Windows builds
- `build.bat` sets `ProgramFiles(x86)` if missing so `vswhere` works in non-interactive shells
