# Windows installer builds

`.github/workflows/windows_release.yml` builds the Windows x64 application with
Release compiler optimization and the testing release channel, installs it with
Qt runtime dependencies, and packages the installation using the repository's
CPack/WiX configuration. Application and installer branding remain MuseScore
Studio. This workflow does not sign the installer.

## Run and download

1. Open **Actions → Windows release → Run workflow**, select the desired branch
   or tag, and run it. Branch and tag pushes also trigger the workflow.
2. Wait for **Build Windows x64 installer** and then **Verify installed Windows
   application** to succeed. The build uploads the installer before verification,
   so it remains downloadable if the smoke test fails; treat it as unverified in
   that case.
3. Download **VirtScore-windows-x64** under the run's **Artifacts** and extract it.
   Open `VirtScore-windows-x64-<run number>.msi` to install. Workflow artifacts
   expire after 30 days.
4. For a tag, the publishing job attaches the same MSI and checksum to that tag's
   GitHub Release. It creates a prerelease if needed, leaving promotion to a
   stable release to the maintainer. Reruns replace the installer/checksum assets.
   Existing release notes and draft/prerelease status are preserved. Branch runs
   upload workflow artifacts without creating GitHub Releases.

Check the download with PowerShell (replace `123` with the run number) and compare
it with `SHA256SUMS.txt`:

```powershell
Get-FileHash .\VirtScore-windows-x64-123.msi -Algorithm SHA256
```

Unsigned builds can show an unknown-publisher warning in Windows.

Verification runs in a separate job on a fresh runner without the build-time Qt
installation. If it fails, download **VirtScore-windows-verification-logs** for
the MSI install log and startup stdout/stderr. Use **Re-run failed jobs** to retry
verification with the saved installer, without rebuilding. Rerunning an older
workflow uses its original workflow file; start a new run on the updated branch
to pick up workflow fixes. Tagged releases are published only after verification
succeeds.

## Permissions and secrets

No custom secrets, signing certificates, FTP, Sentry, or translation service
credentials are required. The build job uses `contents: read`. Only the
tag publishing job requests `contents: write`, using the automatic `GITHUB_TOKEN` as
`GH_TOKEN`. Repository/organization Actions policies must permit release writes.
If publication is denied, that job fails with GitHub's error; the successful
build's installer remains downloadable as a workflow artifact.

## Toolchain and local reproduction

CI uses `windows-2025`, MSVC 2022 x64, Ninja/CMake/CPack, Python, Qt 6.11.2 for
MSVC 2022, and WiX 3.14.1 (WiX 3 is required by the existing installer templates).
The workflow reuses preinstalled WiX 3.14 tools when both `candle.exe` and
`light.exe` exist. Otherwise it installs Chocolatey package
`wixtoolset` version `3.14.1.20250415`; the final digits are a package revision,
not a different WiX toolchain version.
Qt modules are qt5compat, qtnetworkauth, qtshadertools, and qtwebsockets, alongside
base/declarative/svg/tools/translations archives. The workflow uses the same
`aqt` archive workaround as the devcontainer. Qt downloads, Python packages,
Chocolatey, and the project's CMake dependency downloads require network access.
The source checkout includes recursively pinned Git submodules. MSI version
metadata comes from the project's CMake version plus the workflow run number;
a tag names the GitHub Release and does not rewrite the application version.
Update project version metadata before tagging when publishing a new version.

For local packaging, use an x64 MSVC developer PowerShell with those tools and Qt
on `CMAKE_PREFIX_PATH` and `PATH`. Follow the configure flags in the workflow,
including an absolute `CMAKE_INSTALL_PREFIX`, `MUE_RUN_WINDEPLOYQT=ON`,
`MUE_PACK_INSTALL_DIR=ON`, a fresh product GUID, and the workflow's fixed upgrade
GUID. Generate translations as shown there, then run:

```powershell
cmake --build build/windows-release --target install --parallel 4
cpack --config build/windows-release/CPackConfig.cmake -G WIX -B build.artifacts `
  "-DCPACK_WIX_ROOT=${env:ProgramFiles(x86)}/WiX Toolset v3.14"
```

The explicit WiX root overrides the packaging file's legacy WiX 3.11 default.
The workflow checks for exactly one MSI, writes a SHA-256 checksum, installs it
silently in a temporary directory, checks that `platforms/qwindows.dll` was
deployed alongside the executable, and invokes the installed program with
`--version` on the fresh verification runner using the packaged `windows` Qt
platform plugin (`windeployqt` does not package `qoffscreen.dll`). The check
redirects stdout/stderr, requires a successful exit and version output, and sets
`QT_COMMAND_LINE_PARSER_NO_GUI_MESSAGE_BOXES=1` so Qt cannot wait for a version
message box to be dismissed in unattended CI. See the
[Qt command-line parser documentation](https://doc.qt.io/qt-6/qcommandlineparser.html#showMessageAndExit).
The failed verification in run 39838373799 requested `offscreen`, while its MSI
log lists `qwindows.dll` and no `qoffscreen.dll`. The verifier now explicitly
selects `windows` and the installed `bin/platforms` directory, so an inherited
platform setting cannot recreate that mismatch. `QT_FORCE_STDERR_LOGGING=1` and
`QT_DEBUG_PLUGINS=1` capture early Qt/plugin errors in the uploaded stderr log.
Rerunning an old failed job uses its original workflow; push this fix and verify
the new run instead.

This smoke check validates packaged startup; interactive notation behavior still
requires score tests and manual review. The Linux devcontainer cannot execute this Windows build.
