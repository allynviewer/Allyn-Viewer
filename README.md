<p align="center">
  <img width="250" height="250" alt="Allyn Viewer Logo" src="https://github.com/user-attachments/assets/560b172b-4f13-421e-8977-6e91dcde022a" />
</p>


**A third-party viewer for Second Life.**

This software is not provided or supported by Linden Lab, the makers of Second Life.

Three priorities: **performance**, **practicality**, and **AI translation inside the viewer** — no in-game HUD.

---

## What it is

Allyn Viewer is a client for playing Second Life on **Windows 64-bit**.

It is not a generic “virtual worlds” viewer and it is not an AI platform. It is a viewer for Second Life players who want:

1. **Performance** — an optimized client (Release, LTO, OpenAL, NVAPI) to keep FPS and stability in-world.
2. **Practicality** — what the player needs in the viewer, without depending on objects, scripts, or HUDs attached to the avatar.
3. **AI translation** — chat translated inside the viewer (incoming and outgoing). You talk in the other player's language without an in-world translator.

Translation runs in the client. No HUD, in-world vendor, or attachment required. It is off until you turn it on and supply your own API key.

Compared with the official Second Life viewer, this build is Windows 64-bit only. Restrained Love (RLVa) is included and is on by default. A script in an object you wear can restrict the viewer, including detaching items, chat, inventory, and teleport. Turn it off in preferences (Restrained Love API). That change applies after a restart.

---

## Download

| Platform | Package |
| -------- | ------- |
| Windows 64-bit | [Latest release](https://github.com/allynviewer/Allyn-Viewer/releases/latest) |

Each release ships two files:

* `Allyn_Viewer_*_Setup.exe` — installer (creates shortcuts and an uninstaller).
* `Allyn_Viewer_*_x86_64.zip` — portable build: unzip anywhere and run `allyn-viewer-bin.exe`. No installation and no registry changes. Delete that folder to remove it.

All versions: [Releases](https://github.com/allynviewer/Allyn-Viewer/releases).

The viewer can offer a newer release. Nothing is downloaded or installed until you accept.

Allyn Viewer does not provide customer support. Bug reports and questions go to [Issues](https://github.com/allynviewer/Allyn-Viewer/issues) and [Discussions](https://github.com/allynviewer/Allyn-Viewer/discussions). Privacy policy: [PRIVACY.md](PRIVACY.md).

---

## Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io), certificate by [SignPath Foundation](https://signpath.org).

The Windows installer (`Allyn_Viewer_*_Setup.exe`) and the executables built from this repository (`allyn-viewer-bin.exe`, `SLPlugin.exe`) are built by [GitHub Actions](.github/workflows/build.yaml) from the tagged source code and signed by SignPath. Third-party libraries shipped alongside them (OpenAL Soft, CEF, OpenSSL, …) are redistributed unsigned or with their upstream signatures. Every signing request is approved manually by a project approver. Details for maintainers: [doc/code_signing.md](doc/code_signing.md).

Team roles:

* Committers and reviewers: [@allynviewer](https://github.com/allynviewer)
* Approvers: [@allynviewer](https://github.com/allynviewer)

The viewer connects to the Second Life grid you log into ([Linden Lab privacy policy](https://lindenlab.com/privacy)). While logged into Second Life, a presence heartbeat (avatar UUID and viewer version) is sent to the project site. The site and the login screen show only the resulting online counts, not the UUID. Passwords are not sent. Details: [PRIVACY.md](PRIVACY.md).

---

## Requirements

* Windows 64-bit
* A Second Life account

There is no official build for Windows 32-bit, Linux, or macOS.

---

## Source

Open-source repository.

Build environment: Visual Studio 2022, **Release** configuration. RelWithDebInfo on Windows compiles with no optimization (`/Od`) and is not the official build.

---

## Build instructions

Anyone can build the viewer to test their own changes. The full step-by-step guide is in **[doc/building_windows.md](doc/building_windows.md)**.

Short version, in a Command Prompt:

```
git clone https://github.com/allynviewer/Allyn-Viewer.git AllynViewer
cd AllynViewer
build.bat full
```

`build.bat` checks the tools (Visual Studio 2022/2026 with C++, CMake, Git, Python 3), installs `autobuild` from `requirements.txt`, downloads the third-party packages, configures and compiles. On a machine without the tools, `build.bat tools` installs Git, CMake, Python and the Visual Studio 2022 Build Tools (C++) through `winget`; the interactive menu offers this automatically. Afterwards:

| Command | Purpose |
| ------- | ------- |
| `build.bat tools` | Install missing build tools with `winget` |
| `build.bat viewer` | Recompile only the viewer after editing code |
| `build.bat smoke` | Start the viewer and confirm it stays running |
| `build.bat run` | Launch the compiled viewer |
| `build.bat package` | Create the portable ZIP and the installer in `build-vc-64\dist` |

Double-clicking `build.bat` opens a menu; it first asks for the language (Deutsch, English, Español, Français, Italiano, 日本語, Português, Русский, Türkçe). On the command line set `BUILD_LANG=pt` (or another code) to get translated output.

Output: `build-vc-64\newview\Release\allyn-viewer-bin.exe`. Every pull request is also compiled by [GitHub Actions](.github/workflows/build.yaml) with the same configuration.

> [!NOTE]
> We do not provide support for compiling the viewer on your own. Build problems can still be reported in [Issues](https://github.com/allynviewer/Allyn-Viewer/issues) with the output of `build.bat check`.

---

## Contribute

Help make Allyn Viewer better: report bugs, suggest improvements or send pull requests. Read **[CONTRIBUTING.md](CONTRIBUTING.md)** first; it explains the branch workflow, how to test a change locally and the comment tags we use when modifying code inherited from the Second Life viewer.

In short: fork this repository, clone your fork, run `build.bat full` once, create a branch, edit and test with `build.bat viewer` / `build.bat smoke`, push the branch to your fork and open a pull request against `main`. The step-by-step version is in [doc/building_windows.md](doc/building_windows.md#from-a-change-to-a-pull-request).

---

## License

Source files follow their original licenses (GNU LGPL v2.1 and, in parts, GNU GPL v2 with the Linden Lab Viewer FLOSS License Exception). Third-party code remains under its original license.

See [LICENSE](LICENSE).

---

## Issues and code

* [Issues](https://github.com/allynviewer/Allyn-Viewer/issues)
* [Discussions](https://github.com/allynviewer/Allyn-Viewer/discussions)
* [Contributing guide](CONTRIBUTING.md) · [Build instructions](doc/building_windows.md)

Bug reports and pull requests are welcome. There is no private or live customer support.

---

## Origin

The base is **Singularity Viewer**, reviewed and improved in this project.

It also uses code from the **official Second Life viewer** (Linden Lab), plus features that match or are inspired by **Firestorm Viewer**.

Second Life is a trademark of Linden Research, Inc. Allyn Viewer is not affiliated with Linden Lab.
