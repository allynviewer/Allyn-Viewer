First public build of **Allyn Viewer**, a third-party viewer for Second Life on Windows 64-bit, focused on performance, practicality and AI translation inside the viewer.

## Downloads

| File | Use it when |
| ---- | ----------- |
| `Allyn_Viewer_Beta_1_0_0_1_x86_64_Setup.exe` | You want a normal installation: shortcuts, Start menu entry and an uninstaller. See [Code signing policy](#code-signing-policy) below about SmartScreen. |
| `Allyn_Viewer_Beta_1_0_0_1_x86_64.zip` | You prefer a portable copy: unzip anywhere and run `AllynViewerBeta\AllynViewerBeta.exe`. Nothing is written to the registry. |

Both packages contain exactly the same files.

## Requirements

* Windows 10 or 11, 64-bit
* A GPU with OpenGL support and up-to-date drivers
* A Second Life account

## What is in this build

* Release configuration with link-time optimisation, OpenAL Soft audio, NVAPI support
* WebRTC voice (`llwebrtc.dll`), CEF-based web media, libVLC media streaming
* UI in Deutsch, English, Español, Français, Italiano, 日本語, Português (Brasil), Русский and Türkçe

## Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io), certificate by [SignPath Foundation](https://signpath.org).

This release’s packages were built before SignPath signing was enabled, so Windows SmartScreen may still warn about an unknown publisher: choose *More info → Run anyway*. Upcoming releases will be signed by SignPath from GitHub Actions builds of this repository. Full policy (team roles and privacy statement): [Code signing policy](https://github.com/allynviewer/Allyn-Viewer#code-signing-policy).

## Known issues

* Until SignPath signing is active on release builds, Windows SmartScreen and some antivirus tools may warn on first launch.
* The CEF splash screen may log `Couldn't navigate`; this is harmless.
* Older AMD drivers (2023 and earlier) can crash during OpenGL initialisation; update the driver if the viewer closes right after the splash screen.

## Feedback

Report problems in [Issues](https://github.com/allynviewer/Allyn-Viewer/issues) with your Windows version, GPU/driver and, if the viewer crashed, `%APPDATA%\AllynViewer\logs\Allyn.log`.

## Checksums (SHA-256)

```
7a662d3648ecb5dcc0a4437f72d98acf16370b212f121408636961e316616888  Allyn_Viewer_Beta_1_0_0_1_x86_64.zip
07addc5dc7b573eb6a1fbc7d19fbd4b80f702735063747e3759e402dc9b4ab62  Allyn_Viewer_Beta_1_0_0_1_x86_64_Setup.exe
```
