## Summary

<!-- What does this change do and why is it needed? Link the issue: Fixes #123 -->

## How it was tested

<!--
Every PR must be built and run locally. Fill in:
- build.bat commands used (e.g. `build.bat viewer`, `build.bat smoke`)
- Windows version and GPU
- What you did in the viewer to verify the change
-->

- [ ] Built in **Release** (`build.bat viewer` or `build.bat full`)
- [ ] `build.bat smoke` passes (viewer starts and stays running)
- [ ] Verified the change in-world / in the affected floater

## Checklist

- [ ] One fix/feature per PR; no unrelated reformatting
- [ ] Upstream (LL/Singularity/Firestorm) code that was changed is wrapped in `// <Allyn> ... // </Allyn>` comments with the original lines preserved
- [ ] New files carry the project license header
- [ ] New UI strings were added to `skins/default/xui/en`
- [ ] Docs updated if a build option, setting or user-visible feature changed

See [CONTRIBUTING.md](../CONTRIBUTING.md) and [doc/building_windows.md](../doc/building_windows.md).
