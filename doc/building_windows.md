# Build instructions for Windows

This page describes every step needed to build Allyn Viewer from source on Windows so you can test your own changes before opening a pull request.

Allyn Viewer is **Windows 64-bit only**. There is no 32-bit, Linux or macOS build.

> [!TIP]
> If you just want the fastest path: install the tools in [Install required development tools](#install-required-development-tools), clone the repository and run `build.bat full`. Everything else on this page explains what that script does and how to do it by hand.

## Overview

| Item | Value |
| ---- | ----- |
| Toolchain | Visual Studio 2022 (official builds) or Visual Studio 2026, "Desktop development with C++" |
| Build system | CMake + [Autobuild](https://github.com/secondlife/autobuild) (Linden Lab) |
| Official configuration | `Release` with `-DUSE_OPENAL:BOOL=ON -DUSE_NVAPI=ON -DUSE_LTO=ON` |
| Build directory | `build-vc-64\` |
| Viewer executable | `build-vc-64\newview\Release\allyn-viewer-bin.exe` (mirrored to `build-vc-64\bin\Release\`) |
| Solution file | `build-vc-64\Allyn.sln` (VS 2022) or `build-vc-64\Allyn.slnx` (VS 2026) |
| Disk space | ~15 GB for the build tree + ~350 MB of downloaded packages |
| Time (first build) | 45–120 minutes depending on CPU; LTO linking alone takes several minutes |

Unlike other third-party viewers you do **not** need:

- Cygwin or any bash shell (autobuild drives `cmake --build` directly)
- a separate "build variables" repository (`AUTOBUILD_VARIABLES_FILE` is not used)
- FMOD Studio (audio uses OpenAL Soft from the Second Life `3p-openal-soft` package)
- KDU/Kakadu (JPEG2000 uses the in-tree OpenJPEG)
- Havok or any proprietary library

## Install required development tools

This only needs to be done once. Install everything with default settings unless told otherwise.

### Automatic installation (winget)

`build.bat tools` installs whatever is missing among Git, CMake, Python 3.12 and **Visual Studio 2022 Build Tools** with the "Desktop development with C++" workload, using the Windows Package Manager (`winget`, included in Windows 10 1809+ and Windows 11):

```
build.bat tools
```

When you use the interactive menu (double-click `build.bat`) and start a build with tools missing, the script offers to run this step for you; answer `Y` (or the localized key, e.g. `S` in Portuguese/Spanish, `J` in German). Visual Studio Build Tools is a multi-gigabyte download, shows a UAC prompt and takes 10–30 minutes. After the tools are installed, open a **new** terminal if a tool is still not found (the `PATH` of an already open terminal is not refreshed). The Python packages (`autobuild`, `llsd`) are installed separately by `build.bat install`, which the build commands run automatically.

If `winget` is not available on your machine, install the tools by hand as described in the next sections.

### Where to put the checkout

Clone the repository to a short path without spaces on a drive with at least **20 GB** free, for example `C:\dev\AllynViewer`. `build.bat check` prints a warning when the path contains spaces, is very long, or is inside a cloud-synced folder (OneDrive, Dropbox, Google Drive) — all three are frequent causes of failed builds — and when the drive has less than 20 GB free.

### Windows

- Windows 10 or Windows 11, 64-bit, fully updated.

### Microsoft Visual Studio

Official builds use **Visual Studio 2022**. Visual Studio 2026 also works (`build.bat` detects whichever is installed, preferring 2022).

- Install [Visual Studio 2022 Community](https://visualstudio.microsoft.com/free-developer-offers) (or Professional/Enterprise/Build Tools).
- On the **Workloads** tab check **Desktop development with C++**. All other workloads can stay unchecked.
- Make sure the following optional components are selected (they normally are with that workload): MSVC v143 build tools, Windows 10/11 SDK, C++ CMake tools for Windows.

### Git

- Install [Git for Windows](https://git-scm.com/downloads/win) with default options.
- When asked about line endings choose **Checkout as-is, commit as-is**.
- Ensure `C:\Program Files\Git\cmd` is in your `PATH` (the installer does this by default).

### CMake

- Install [CMake](https://cmake.org/download/) 3.16 or newer (tested with 3.28 through 4.4).
- At the "Install options" screen select **Add CMake to the system PATH for all users**.
- CMake 4.x is supported; `build.bat` passes `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` so the older third-party CMake files still configure.

### Python 3

- Install the latest **Python 3** (3.10 – 3.14) from [python.org](https://www.python.org/downloads/windows/).
- Tick **Add python.exe to PATH** and make sure **pip** is included.
- Do not rely on the Microsoft Store "python" alias alone. It works for `build.bat` but CMake's Python detection looks in the standard python.org install locations and the registry.

> [!TIP]
> On Windows 10/11 you may want to disable the app execution alias for Python: open Settings, search for "Manage app execution aliases" and turn off `python.exe` / `python3.exe`.

### NSIS (optional)

Only needed if you want to produce the `Allyn_Viewer_*_Setup.exe` installer (`build.bat package`). `build.bat tools` installs NSIS 3 with `winget` and then runs `scripts\setup-nsis-plugins.ps1`, which adds the two plugins the installer script needs and stock NSIS does not ship (StdUtils and INetC; the script asks for UAC elevation to write into the NSIS folder). To do it by hand: install NSIS from the [NSIS website](https://nsis.sourceforge.io), then run `powershell -ExecutionPolicy Bypass -File scripts\setup-nsis-plugins.ps1`. Without NSIS the compiled viewer still runs normally and `build.bat package` still produces the portable ZIP; only the installer is skipped.

### Intermediate check

Open a **new** Command Prompt (`cmd.exe`) and confirm every tool is found:

```
git --version
cmake --version
python --version
pip --version
```

If any of them reports "not recognized", fix your `PATH` before continuing.

## Set up Autobuild

Autobuild is the Linden Lab tool that downloads the prebuilt third-party packages listed in `autobuild.xml` and drives CMake. The versions used by the project are pinned in `requirements.txt`.

```
cd \path\to\AllynViewer
pip install -r requirements.txt
autobuild --version
```

Autobuild **3.10 or newer** is required (the Second Life packages are `.tar.zst` archives). `build.bat install` runs the same command for you.

If you prefer to keep the Python packages isolated:

```
python -m venv .venv
.venv\Scripts\activate.bat
pip install -r requirements.txt
```

Activate the virtual environment in every new terminal before building.

## Get the source code

```
cd \path\where\you\keep\code
git clone https://github.com/allynviewer/Allyn-Viewer.git AllynViewer
cd AllynViewer
```

If you plan to contribute, fork the repository on GitHub first and clone your fork instead, then add the upstream remote:

```
git remote add upstream https://github.com/allynviewer/Allyn-Viewer.git
```

## Building with build.bat (recommended)

`build.bat` is a Windows batch script at the repository root. It checks your tools, installs the Python packages, configures, compiles and can run a quick smoke test. Run it from a Command Prompt or PowerShell in the repository root, or just double-click it to get a menu.

### Language

When started without arguments (double-click) the script first asks which language to use — Deutsch, English (US), Español, Français, Italiano, 日本語, Português (Brasil), Русский or Türkçe — and then shows the menu in that language. Option `L` in the menu switches language at any time.

When started with a command (`build.bat viewer`, `build.bat smoke`, …) there is no prompt; the script uses the `BUILD_LANG` environment variable and falls back to `en-us`:

```
set BUILD_LANG=pt
build.bat check
```

Valid codes: `de`, `en-us`, `es`, `fr`, `it`, `ja`, `pt`, `ru`, `tr`. The strings live in `build-lang\<code>.cmd` (UTF-8 without BOM); to fix or add a translation edit that file — do not use the characters `!`, `^`, `%` or `"` inside a message, and keep the `{0}`/`{1}` placeholders. The script switches the console to code page 65001 (UTF-8) while it runs and restores the previous one on exit. The legacy `conhost` font has no CJK glyphs, so Japanese text may show as boxes there; Windows Terminal renders it correctly, or set the console font to *MS Gothic*.

### Commands

| Command | What it does |
| ------- | ------------ |
| `build.bat check` | Verifies Python, autobuild, Git, CMake, Visual Studio, MSBuild, NSIS, the repository layout, the checkout path and free disk space |
| `build.bat tools` | Installs missing Git / CMake / Python / Visual Studio 2022 Build Tools / NSIS (with its plugins) using `winget` |
| `build.bat install` | Installs/updates the packages from `requirements.txt` |
| `build.bat full` | Clean + check + install + configure + build everything (first build) |
| `build.bat configure` | Runs `autobuild configure` only (generates the solution) |
| `build.bat build` | Runs `autobuild build --no-configure` (incremental build of everything) |
| `build.bat viewer` | Compiles only `allyn-viewer-bin` and its dependencies (`autobuild build -- --target allyn-viewer-bin`; fastest after editing viewer code) |
| `build.bat run` | Starts the compiled viewer |
| `build.bat smoke` | Starts the viewer, waits 45 s, checks it is still running and wrote its start marker, then closes it |
| `build.bat package` | Runs `viewer_manifest.py` to produce the distribution packages in `build-vc-64\dist` (portable ZIP always; NSIS installer when NSIS is installed) |
| `build.bat skins` | Copies `indra\newview\skins` into the build output without recompiling (UI/XML work) |
| `build.bat rebuild` | Deletes `build-vc-64` and does a full build |
| `build.bat clean` | Deletes `build-vc-64` |

Typical contributor loop:

```
build.bat full        REM first time only
...edit code...
build.bat viewer      REM recompile the viewer binary
build.bat smoke       REM make sure it still starts
```

### Packaging

`build.bat package` stages the viewer exactly as it is shipped (via `indra\newview\viewer_manifest.py`) and writes two files to `build-vc-64\dist\`:

| File | Content |
| ---- | ------- |
| `Allyn_Viewer_<Type>_<version>_x86_64.zip` | Portable build: a single `AllynViewer<Type>` folder with `AllynViewer<Type>.exe` and everything it needs. Unzip anywhere and run the `.exe`; no installation, no registry changes. |
| `Allyn_Viewer_<Type>_<version>_x86_64_Setup.exe` | NSIS installer (only when NSIS and its plugins are installed). Installs to `Program Files`, creates shortcuts and an uninstaller. Supports `/S` (silent), `/D=<dir>`, `/SKIP_AUTORUN`. |

`<Type>` is the channel type from the CMake cache (`Beta` by default, `Release` for release builds — see `VIEWER_CHANNEL_TYPE` under [Configuration switches](#configuration-switches)); `<version>` is `viewer_version.txt` with underscores (e.g. `1_0_0_1`). The ZIP and the installer contain the same file list, so testing the ZIP is a valid test of the installer's payload.

The script sets `AUTOBUILD_VSVER` (170 for VS 2022, 180 for VS 2026), `AUTOBUILD_WIN_CMAKE_GEN`, `AUTOBUILD_ADDRSIZE=64` and stores downloaded packages in `.cache\autobuild` inside the repository so a `rebuild` does not download them again.

The viewer version's build number comes from `git rev-list --count HEAD` (exported as the `revision` environment variable). Builds made from a `.zip` download without Git history get build number `0`.

## Building manually with autobuild

Everything `build.bat` does can be typed by hand. Use a Command Prompt in the repository root.

### Configure

```
set AUTOBUILD_VSVER=170
set AUTOBUILD_ADDRSIZE=64
set AUTOBUILD_INSTALLABLE_CACHE=%CD%\.cache\autobuild
autobuild configure -A 64 -c Release -- -DUSE_OPENAL:BOOL=ON -DUSE_NVAPI=ON -DUSE_LTO=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

Use `AUTOBUILD_VSVER=180` for Visual Studio 2026.

The first configure downloads the third-party packages referenced by `autobuild.xml` (Second Life `3p-*` GitHub releases plus a few legacy archives) and clones the CMake `FetchContent` dependencies (`fmt`, `nlohmann_json`, `abseil`, `c-ares`, `xmlrpc-epi`). This needs internet access and takes a few minutes. Add `-v` to `autobuild configure` to watch the downloads.

### Build targets

| Configuration | Notes |
| ------------- | ----- |
| `Release` | Official configuration: `/O2`, LTO, OpenAL, NVAPI. Use this for testing and for anything you distribute. |
| `RelWithDebInfo` | On Windows this compiles with `/Od` (no optimisation). Only use it when you need to step through code in the debugger. Performance is not representative. |

### Configuration switches

Pass switches after the `--` separator of `autobuild configure`. `OFF`/`NO`/`FALSE` mean false, anything else means true.

- **`-DUSE_OPENAL:BOOL=ON`** – OpenAL Soft audio (on in official builds).
- **`-DUSE_NVAPI:BOOL=ON`** – NVIDIA driver profile integration (on in official builds; harmless on AMD/Intel).
- **`-DUSE_LTO:BOOL=ON`** – link-time optimisation. Turn **off** for faster link times while iterating: `-DUSE_LTO:BOOL=OFF`.
- **`-DLL_TESTS:BOOL=ON`** – build and run the unit tests (off by default).
- **`-DPACKAGE:BOOL=OFF`** – skip the `llpackage` target (installer). On by default; without NSIS it only warns.
- **`-DUSE_PRECOMPILED_HEADERS:BOOL=OFF`** – disable PCH.
- **`-DCMAKE_POLICY_VERSION_MINIMUM=3.5`** – required with CMake 4.x.
- **`-DVIEWER_CHANNEL_BASE:STRING="Allyn Viewer"`** and **`-DVIEWER_CHANNEL_TYPE:STRING=Beta`** – channel name. Set `VIEWER_CHANNEL_TYPE` to `Test` for private builds so they are not confused with releases. Both can also be provided as environment variables.
- **`-DUSE_CRASHPAD:BOOL=ON`** – crash reporting (off; requires a Crashpad endpoint).

### Build

```
autobuild build -A 64 -c Release --no-configure
```

This runs `cmake --build . --config Release --parallel` inside `build-vc-64`. Compiling takes a long time; the final LTO link of `allyn-viewer-bin.exe` can take several minutes with no visible progress.

To compile only the viewer executable (and the libraries it depends on), pass a CMake target after `--`:

```
autobuild build -A 64 -c Release --no-configure -- --target allyn-viewer-bin
```

This is exactly what `build.bat viewer` does. Always build through autobuild (or `build.bat`): autobuild sets up the Visual Studio environment before calling CMake, and MSBuild invoked from a plain shell sees a different toolchain environment and recompiles every source file again. Alternating between `build.bat build` and `build.bat viewer` is incremental.

### Building from within Visual Studio

After configuring, open `build-vc-64\Allyn.sln` (VS 2022) or `build-vc-64\Allyn.slnx` (VS 2026).

- Select the **Release** configuration and **x64** platform.
- Build → Build Solution, or right-click `allyn-viewer-bin` → Build to compile just the viewer.
- `allyn-viewer-bin` is already the startup project and its debugger working directory is `indra\newview`, so **Debug → Start** launches the viewer with the source skins and settings.

## Running and testing your build

The complete viewer (executable, DLLs, `app_settings`, `skins`, `fonts`, `llplugin`) is assembled by `viewer_manifest.py` in:

```
build-vc-64\newview\Release\
```

`build.bat` also mirrors that folder to `build-vc-64\bin\Release\` and launches the viewer from there (`build.bat run`).

### Smoke test

```
build.bat smoke
```

Starts the viewer, waits `SMOKE_SECONDS` (default 45) and reports:

- `[OK] Viewer continua em execucao` – the process reached the login screen without crashing.
- `[OK] Marker de execucao gravado` – `%APPDATA%\AllynViewer\logs\Allyn.exec_marker` was written, proving directories, settings and OpenGL initialised.

Set `SMOKE_SECONDS=90` before running it on slower machines.

### Logs

Allyn Viewer uses **crash-only logging**: `Allyn.log` is written only when the viewer crashes. In a normal session you will find:

| File | Purpose |
| ---- | ------- |
| `%APPDATA%\AllynViewer\logs\Allyn.exec_marker` | Present while the viewer runs; left behind after a crash |
| `%APPDATA%\AllynViewer\logs\Allyn-debug\static_debug_info.log` | Build/version/GPU information captured at startup |
| `%APPDATA%\AllynViewer\logs\Allyn.log` | Only after a crash |
| `%APPDATA%\AllynViewer\user_settings\` | Per-machine settings |

### Iterating on UI/XML only

If you only changed files under `indra\newview\skins`, run `build.bat skins` to copy them to the output directories and restart the viewer; no compile needed.

## Updating your build

```
git pull
build.bat build
```

`autobuild build --no-configure` is incremental. If `autobuild.xml` changed (new package versions), CMake re-runs `autobuild install` automatically for the affected packages. If a pull changes many headers or CMake files, a clean `build.bat rebuild` is often faster than a huge incremental build.

## Troubleshooting

### `AUTOBUILD_VSVER=170 unsupported, is Visual Studio 170 installed?`

Autobuild locates Visual Studio through `%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe`.

- Confirm the C++ workload is installed for the version you selected (170 = 2022, 180 = 2026).
- Some terminals (embedded IDE terminals, CI shells) do not export `ProgramFiles(x86)`; `build.bat` sets it, but if you run `autobuild` yourself do `set "ProgramFiles(x86)=C:\Program Files (x86)"` first.

### `python` opens the Microsoft Store or prints "Python was not found"

The Store alias shadows the real interpreter. Install Python from python.org with "Add python.exe to PATH" and disable the app execution aliases (see [Python 3](#python-3)).

### `No SOURCES given to target: xmlrpc-epi`

Older revisions filtered the fetched `xmlrpc-epi` sources with a regex applied to the full path, so any checkout under a folder containing `test` or `sample` (for example `C:\dev\test\AllynViewer`) lost all sources. Update to a revision that includes the fix in `indra/deps/CMakeLists.txt` or move the checkout to a path without those words.

### `Failed to download or unpack prebuilt '<package>'`

A package from `autobuild.xml` could not be fetched. Check your internet connection and proxy, then delete `build-vc-64\packages\cmake_tracking\<package>_installed` and re-run the configure step. Downloaded archives are cached in `.cache\autobuild`; delete a corrupt archive from there to force a fresh download.

### `mt.exe` not found during the post-build step

The Manifest Tool ships with the Windows SDK. In the Visual Studio Installer make sure a **Windows 10/11 SDK** component is installed. `build.bat` searches `C:\Program Files (x86)\Windows Kits\10\bin\<version>\x64\mt.exe` and adds it to `PATH` automatically.

### `UnicodeEncodeError` from `packages-formatter.py`

Happens on consoles using code page 1252 when a package license contains non-ASCII characters. The script now reconfigures stdout to UTF-8; if you still hit it, set `PYTHONUTF8=1` in the environment.

### The link step seems stuck or the machine runs out of memory

LTO linking of the viewer needs several GB of RAM and can take 5–10 minutes with no output. 16 GB RAM is recommended. For faster iteration configure with `-DUSE_LTO:BOOL=OFF` (your test build will be slightly slower at runtime).

### CMake complains about `cmake_minimum_required` / policy versions

You are using CMake 4.x. Pass `-DCMAKE_POLICY_VERSION_MINIMUM=3.5` to the configure step (`build.bat` already does).

### The viewer exits immediately

Look for a fresh `%APPDATA%\AllynViewer\logs\Allyn.log` (written on crash) and check `static_debug_info.log`. Make sure another `allyn-viewer-bin.exe` is not already running and that the GPU driver is up to date. See the "Known Crashes & Fixes" section of `AGENTS.md` for issues already diagnosed.

## From a change to a pull request

The complete flow for a community contribution, from an empty machine to a PR waiting for review:

1. **Fork** `https://github.com/allynviewer/Allyn-Viewer` on GitHub (button *Fork*, top right).
2. **Clone your fork** to a short path and add the original repository as `upstream`:

   ```
   git clone https://github.com/<your-user>/AllynViewer.git C:\dev\AllynViewer
   cd C:\dev\AllynViewer
   git remote add upstream https://github.com/allynviewer/Allyn-Viewer.git
   ```

3. **Build once**: `build.bat full` (or double-click `build.bat`, pick a language and choose option 1). If tools are missing the script installs them (see [Automatic installation](#automatic-installation-winget)); if the check still fails, fix what `build.bat check` reports before going on.
4. **Create a branch** for the change: `git checkout -b fix/short-description`.
5. **Edit, compile, test**: `build.bat viewer` after editing C++ (`build.bat skins` for XML/UI only), then `build.bat smoke` or `build.bat run` and try the change in the viewer.
6. **Commit and push** to your fork:

   ```
   git add -A
   git commit -m "Fix crash when opening inventory with empty filter"
   git push -u origin fix/short-description
   ```

7. **Open the pull request**: GitHub shows a *Compare & pull request* banner on your fork; the base is `allynviewer/Allyn-Viewer` `main`. Fill in the template (what changed, why, how you tested — which `build.bat` commands, Windows version, GPU). GitHub Actions builds every PR with the same `build.bat`/autobuild flow, and a maintainer reviews, tests and merges it or asks for changes.
8. **Keep the branch up to date** while the PR is open:

   ```
   git fetch upstream
   git rebase upstream/main
   git push --force-with-lease
   ```

See [CONTRIBUTING.md](../CONTRIBUTING.md) for the review rules (one change per PR, `// <Allyn>` tags around upstream code, commit message style).

## Getting help

- Open an [issue](https://github.com/allynviewer/Allyn-Viewer/issues) with the exact error, your Visual Studio/CMake/Python/autobuild versions (`build.bat check` prints them) and the relevant log excerpt.
- Read [CONTRIBUTING.md](../CONTRIBUTING.md) before opening a pull request.
