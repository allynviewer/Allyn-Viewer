# Contributing to Allyn Viewer

Thank you for helping improve Allyn Viewer. This guide explains how to set up a build, what we expect in a pull request and how to keep the code easy to merge with upstream changes from the Second Life viewer and Firestorm.

## Before you start

1. **Build the viewer once.** Follow [doc/building_windows.md](doc/building_windows.md). On a fresh machine `build.bat tools` installs the required tools with `winget` and `build.bat full` does the rest. If you cannot run `build.bat full` successfully, fix that first (`build.bat check` tells you what is missing); every change must be tested in a local build.
2. **Look for an existing issue.** Search the [issue tracker](https://github.com/allynviewer/Allyn-Viewer/issues). For larger features open an issue or a [discussion](https://github.com/allynviewer/Allyn-Viewer/discussions) before writing code so the approach can be agreed on.
3. **Work on a branch in your fork.** Keep `main` in your fork identical to upstream and create one branch per fix or feature.

## Scope of the project

Allyn Viewer is a Second Life viewer for **Windows 64-bit** with three priorities: performance, practicality and AI translation inside the viewer. Changes that add Linux/macOS/32-bit support, OpenSim-only features or in-world HUD dependencies are outside the current scope and will not be merged.

## Development workflow

```
git checkout -b fix/short-description
build.bat viewer        # compile only the viewer after editing code
build.bat smoke         # confirm the viewer still starts
build.bat skins         # if you only touched indra\newview\skins (XML/UI)
git push -u origin fix/short-description   # then open the PR on GitHub against main
```

The complete fork → clone → build → branch → PR sequence, including how to keep the branch up to date with `upstream/main`, is in [doc/building_windows.md](doc/building_windows.md#from-a-change-to-a-pull-request). Every PR is compiled by GitHub Actions ([build.yaml](.github/workflows/build.yaml)); a PR whose build fails will not be reviewed until it compiles.

- The official configuration is **Release** (`-DUSE_OPENAL:BOOL=ON -DUSE_NVAPI=ON -DUSE_LTO=ON`). Test in Release. `RelWithDebInfo` on Windows disables optimisation and is for stepping through code in the debugger only.
- Do not commit anything under `build-vc-64\`, `.cache\`, or generated Visual Studio files; `.gitignore` already excludes them.
- Do not modify `autobuild.xml` in a PR unless the change is precisely about updating a third-party package. If you need a private package for local testing, copy the file to `my_autobuild.xml` (ignored by Git) and set `AUTOBUILD_CONFIG_FILE=my_autobuild.xml`.

## Pull request guidelines

1. **Descriptive title.** Say what the change does, e.g. `Fix crash when opening inventory with empty filter`.

2. **Related issues.** Reference the issue number in the description (`Fixes #123`). Add it to the commit header as well when it helps: `[#123] Prevent inventory crash on empty filter`.

3. **Description.** Explain *why* the change is needed and what problem it solves. If it changes behaviour the user can see, add a screenshot or a short clip.

4. **Testing.** Describe exactly how you tested: which `build.bat` commands you ran, the Windows version, the GPU, and what you did in the viewer (login, open floater X, etc.). PRs that were not built and run locally will be sent back.

5. **Keep PRs focused.** One fix or feature per PR. Reformatting, renaming or unrelated cleanups make review and upstream merges harder; send them separately.

6. **Comment tags for upstream code (important).** Large parts of `indra/` come from the Linden Lab viewer (via Singularity) and from Firestorm. When you modify code that originates upstream, keep the original lines in a comment so whoever merges the next upstream update can see both versions:

   ```c++
   // <Allyn> [#123] Prevent inventory crash on empty filter
   // if (filter.empty()) return nullptr;
   if (filter.empty())
   {
       return &sEmptyFilter;
   }
   // </Allyn>
   ```

   A one-line change can use the short form:

   ```c++
   bool break_stuff = false; // </Allyn> [#456] don't break stuff
   ```

   You may add your initials, e.g. `<Allyn:AB>`. Tags are **not** needed in files created by this project (anything that only exists in Allyn Viewer) or when you are editing code that is already inside an `// <Allyn>` block; just keep the surrounding comment accurate.

7. **Match the surrounding style.** Tabs/spaces, brace placement and naming follow the file you are editing (see `.editorconfig` for defaults). Do not run a formatter over whole files.

8. **License headers.** New source files must carry the viewer LGPL header used across the tree (see [LICENSE](LICENSE)):

   ```c++
   /**
    * @file myfile.cpp
    * @brief One-line description.
    *
    * $LicenseInfo:firstyear=2026&license=viewerlgpl$
    * $/LicenseInfo$
    */
   ```

9. **Translations.** UI strings live in `indra/newview/skins/default/xui/<lang>/`. Add new strings to `en` first; other languages are optional but welcome (`pt` is actively maintained).

10. **Documentation.** If you add a build option, a setting or a user-visible feature, update `doc/building_windows.md`, the relevant `settings.xml` description or the README accordingly.

## Commit messages

- Imperative mood, short summary line (≤ 72 characters), blank line, then details if needed.
- No generated trailers (`Co-authored-by` for tools, `Made-with`, etc.).

## Review process

A maintainer will build the branch, test it and either merge, request changes or explain why it cannot be accepted. Please respond to review comments in the same PR; force-pushing rewritten history is fine while the PR is open.

## Reporting bugs

Open an issue with:

- Viewer version/channel (Help → About) or the commit hash you built.
- Windows version and GPU/driver.
- Steps to reproduce and what you expected.
- `%APPDATA%\AllynViewer\logs\Allyn.log` if the viewer crashed (it is only written on crash) and `static_debug_info.log` from the same folder.

Thank you for contributing.
