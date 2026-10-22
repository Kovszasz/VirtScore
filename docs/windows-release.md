# Windows installer builds

`.github/workflows/windows_release.yml` builds the Windows x64 application with
Release compiler optimization and the testing release channel, installs it with
Qt runtime dependencies, and packages the installation using the repository's
CPack/WiX configuration. Application and installer branding remain MuseScore
Studio. This workflow does not sign the installer.

## Run and download

1. Open **Actions → Windows release → Run workflow**, select the desired branch
   or tag, and run it. Branch and tag pushes also trigger the workflow.
2. Wait for **Build Windows x64 installer**, including the silent MSI installation
   and installed executable startup check, to succeed.
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
silently in a temporary directory, and invokes the installed program with
`--version` after removing the CI Qt tools from PATH. This smoke check validates
packaged startup; interactive notation behavior still requires score tests and
manual review. The Linux devcontainer cannot execute this Windows build.
