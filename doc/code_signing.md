# Code signing (SignPath Foundation)

Unsigned installers trigger the Windows SmartScreen warning *"Windows protected your PC — unknown publisher"*. The only real fix is an Authenticode signature. Allyn Viewer uses the free Open Source program of the [SignPath Foundation](https://signpath.org): the certificate is issued to the Foundation, the private key lives in SignPath's HSM and every signed binary is verified to come from a GitHub Actions build of this repository.

This page is for maintainers. The user-facing statement lives in the [Code signing policy](../README.md#code-signing-policy) section of the README (SignPath requires that exact section name on the project home page and on release pages).

## How it works

`.github/workflows/build.yaml` does everything; nothing is signed on developer machines.

1. The viewer is built on a GitHub-hosted `windows-2022` runner (SignPath verifies that every job ran on GitHub-hosted runners).
2. **Signing request 1 – executables.** `allyn-viewer-bin.exe` and `SLPlugin.exe` (the two executables built from this repository) are uploaded as a workflow artifact and submitted to SignPath. The signed copies replace the build outputs *before* packaging, so both the portable ZIP and the installer ship signed executables.
3. `viewer_manifest.py` produces the portable ZIP and the NSIS installer (same invocation as `build.bat package`).
4. **Signing request 2 – installer.** `Allyn_Viewer_*_Setup.exe` is submitted and replaced by the signed copy.
5. On a `v*` tag the packages are published as a GitHub Release with a *Code signing policy* footer.

Signing runs when the build is a `v*` tag **or** a manual run with `sign` enabled, **and** the `SIGNPATH_API_TOKEN` secret exists. Pull requests, pushes to `main` and forks produce unsigned packages and never touch SignPath.

Each signing request requires a **manual approval** in SignPath (mandatory for Open Source subscriptions). The workflow waits up to 2 hours per request; the approver receives an e-mail with a link. Two approvals per release.

Third-party binaries (OpenAL Soft, CEF/Dullahan, OpenSSL, APR, …) are **not** signed with this certificate — SignPath forbids signing upstream code with a project's certificate. They may be shipped unsigned inside the signed installer/ZIP.

## Requirements that the repository already fulfils

SignPath enforces file metadata on every signed binary ([conditions](https://signpath.org/terms.html)):

| Requirement | Where it is implemented |
| ----------- | ----------------------- |
| `ProductName` = `Allyn Viewer` on every signed file | `indra/newview/res/viewerRes.rc.in`, `indra/llplugin/slplugin/slplugin.rc.in`, `indra/newview/installers/windows/installer_template.nsi` |
| `ProductVersion` identical on every signed file of a build | all three use `<major>.<minor>.<patch>.<build>` = content of `build-vc-64\newview\viewer_version.txt`; the workflow passes it as the `version` parameter |
| Binaries built from source in a verifiable way | GitHub Actions + SignPath GitHub connector (origin verification) |
| *Code signing policy* section with the required sentence, team roles and privacy statement | `README.md` and the release notes footer written by the `release` job |
| Uninstaller available, no silent system changes | NSIS installer registers an uninstaller; the portable ZIP writes nothing to the registry |
| OSI licence, public repository, released, documented, maintained | LGPL/GPL, this repository, [Releases](https://github.com/allynviewer/Allyn-Viewer/releases) |

Keep these in sync: if the channel/version scheme or the executable names change, update the artifact configurations below.

## Step 1 – Apply

1. Every maintainer with write access must have **two-factor authentication** enabled on GitHub (SignPath checks this).
2. Make sure at least one release with the installer exists on GitHub (SignPath signs only projects that are "already released in the form that should be signed").
3. Fill in the form at <https://signpath.org/apply> with:
   * Repository: `https://github.com/allynviewer/Allyn-Viewer`
   * Licence: GNU LGPL v2.1 / GNU GPL v2 with the Linden Lab FLOSS exception (`LICENSE`)
   * Download page: `https://github.com/allynviewer/Allyn-Viewer/releases`
   * Artifacts to sign: Windows installer (`.exe`, NSIS) and the viewer executables inside the portable ZIP; built by GitHub Actions; only CI builds are submitted.
   * Point to the *Code signing policy* section of the README.
4. Wait for the approval e-mail. SignPath creates an organization for the project and invites you.

## Step 2 – Configure SignPath (once)

Log in at <https://app.signpath.io>. Slugs below are the defaults used by the workflow; if you pick other names, set the corresponding repository variables (see Step 3).

1. **Trusted build system.** *Organization → Trusted build systems*: add the predefined **GitHub.com** system.
2. **GitHub App.** Install the *SignPath* GitHub App on the `Allyn-Viewer` repository (needed for origin verification).
3. **Project.** *Projects → Add*: name `Allyn Viewer`, slug `allyn-viewer`, repository URL `https://github.com/allynviewer/Allyn-Viewer`. Link the GitHub.com trusted build system to the project.
4. **Artifact configuration `binaries`** (default configuration of the project):

   ```xml
   <?xml version="1.0" encoding="utf-8"?>
   <artifact-configuration xmlns="http://signpath.io/artifact-configuration/v1">
     <parameters>
       <parameter name="version" required="true" />
     </parameters>
     <!-- actions/upload-artifact always wraps the files in a ZIP -->
     <zip-file>
       <pe-file path="allyn-viewer-bin.exe" product-name="Allyn Viewer" product-version="${version}">
         <authenticode-sign/>
       </pe-file>
       <pe-file path="SLPlugin.exe" product-name="Allyn Viewer" product-version="${version}">
         <authenticode-sign/>
       </pe-file>
     </zip-file>
   </artifact-configuration>
   ```

5. **Artifact configuration `installer`:**

   ```xml
   <?xml version="1.0" encoding="utf-8"?>
   <artifact-configuration xmlns="http://signpath.io/artifact-configuration/v1">
     <parameters>
       <parameter name="version" required="true" />
     </parameters>
     <zip-file>
       <pe-file path="Allyn_Viewer*_x86_64_Setup.exe" product-name="Allyn Viewer" product-version="${version}">
         <authenticode-sign/>
       </pe-file>
     </zip-file>
   </artifact-configuration>
   ```

6. **Signing policy `release-signing`:** certificate = the SignPath Foundation certificate provided to the project; **Approval process: on**; **Trusted build system verification: on**; **Origin verification: on**; allowed branch names: `main`, `release/*` and tags `v*` (adapt to the branches you tag from). Add yourself as *Approver*.
7. **CI user.** *Users → Add CI user* (e.g. `github-actions`), give it the *Submitter* role on `release-signing`, and copy its **API token**. Note the **Organization ID** (*Organization → Settings*).

## Step 3 – GitHub secrets and variables

*Repository → Settings → Secrets and variables → Actions.*

| Kind | Name | Value |
| ---- | ---- | ----- |
| Secret | `SIGNPATH_API_TOKEN` | API token of the CI user |
| Secret | `SIGNPATH_ORGANIZATION_ID` | SignPath organization ID (GUID) |
| Variable (optional) | `SIGNPATH_PROJECT_SLUG` | default `allyn-viewer` |
| Variable (optional) | `SIGNPATH_SIGNING_POLICY_SLUG` | default `release-signing` |
| Variable (optional) | `SIGNPATH_BINARIES_CONFIG_SLUG` | default `binaries` |
| Variable (optional) | `SIGNPATH_INSTALLER_CONFIG_SLUG` | default `installer` |
| Variable (optional) | `RELEASE_CHANNEL_TYPE` | channel type of tag builds, default `Release` (set `Beta` while the project is in beta so that the packages are named `Allyn_Viewer_Beta_…` like the current releases) |

## Step 4 – Test without publishing

*Actions → Build → Run workflow*: choose `channel_type`, tick **sign**, run. Approve the two signing requests in SignPath when the e-mails arrive (or from *Signing requests* in the SignPath UI). The workflow log prints the signer of every file and the packages are available as the `AllynViewer-packages-<version>-<sha>` artifact. Check one file on a Windows machine:

```powershell
Get-AuthenticodeSignature .\Allyn_Viewer_Beta_1_0_0_5_x86_64_Setup.exe | Format-List Status, SignerCertificate, TimeStamperCertificate
```

`Status` must be `Valid` and the signer `CN=SignPath Foundation`.

## Step 5 – Publish a signed release

Signed releases are produced by CI (a local build cannot be signed: SignPath only accepts binaries built by GitHub Actions).

```
git tag v1.0.0.5          # v<version>, version = VIEWER_VERSION.txt + build number
git push origin v1.0.0.5
```

The workflow builds, waits for the two approvals and creates the GitHub Release `v1.0.0.5` titled `Allyn Viewer 1.0.0.5 <channel type>` (pre-release unless the channel type is `Release`). Release notes come from GitHub's generated notes unless the release already exists (in that case title and notes are left untouched); the *Code signing policy* footer is always appended when the job creates the release.

If the release already exists, the job only uploads the signed packages with `--clobber`, so the unsigned assets are replaced as long as the file names match — i.e. the channel type used locally (`VIEWER_CHANNEL_TYPE` in the CMake cache, default `Beta`) equals `RELEASE_CHANNEL_TYPE` in CI. The release title and notes are left untouched in that case.

The build number is `git rev-list --count HEAD`, so the version of a tag build equals the version of a local build of the same commit.

## Troubleshooting

| Symptom | Cause / fix |
| ------- | ----------- |
| Signing steps are skipped (`SIGN: false` in *Tool versions*) | Not a tag build / `sign` not ticked, or `SIGNPATH_API_TOKEN` missing. Forks never have the secret. |
| `Origin verification failed` | The SignPath GitHub App is not installed on the repository, the trusted build system is not linked to the project, or the run used a self-hosted runner. |
| `File metadata restriction violated` | `ProductName` or `ProductVersion` of a binary differs from the artifact configuration — check the `.rc.in`/`.nsi` files listed above and that `version` equals `viewer_version.txt`. |
| `The artifact does not match the artifact configuration` | The file names inside the uploaded artifact changed (executable renamed, installer pattern) — update the XML. |
| Request 1 signed but request 2 timed out | Approve it in SignPath and re-run the failed job: `re-run failed jobs` re-executes the whole job, producing two new requests (no partial signing state is kept). |
| `Get-AuthenticodeSignature` says `UnknownError`/`NotTrusted` on an old Windows | Root certificate not present offline; Windows 10/11 with automatic root updates report `Valid`. |
