# VirtScore development container

Open the repository in VS Code and choose **Dev Containers: Reopen in Container**,
with Docker running. The image supports Linux x64 and ARM64 (including Docker
Desktop on Windows and macOS). Allow several GB of disk space for Qt, dependencies
and build outputs; at least 8 GB of memory is recommended for compilation.

The container includes Ubuntu 24.04, GCC 14, CMake, Ninja, Qt 6.11.2 (matching CI),
ccache, GDB, Uncrustify and PDM. Creation initializes both pinned Git submodules
and installs the Python dependencies from `pdm.lock`. The image also applies a
small workaround for aqt archive detection in `patch-aqt.py`, shared with the
Windows workflow. The first CMake configure
also downloads the project's external C++ dependencies and soundfont.

From a container terminal:

```sh
cmake -S . -B build/devcontainer -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
cmake --build build/devcontainer --parallel 2
```

VS Code's CMake Tools uses the same build directory and options. Adjust the
parallel job count to fit the memory available to Docker. Qt is available through
`QT_DIR`, `QTDIR`, `CMAKE_PREFIX_PATH` and `PATH`.

For a headless invocation, prefix the application command with
`QT_QPA_PLATFORM=offscreen` or `xvfb-run -a`. An interactive desktop requires a
separately configured X11/Wayland display and audio forwarding; the container
does not expose your host display or audio devices automatically.

The CLI equivalent is:

```sh
npx @devcontainers/cli up --workspace-folder .
npx @devcontainers/cli exec --workspace-folder . cmake --version
```

This container builds the Linux application. Windows installers are built by
[the Windows release workflow](../.github/workflows/windows_release.yml) using
MSVC on a Windows runner.
