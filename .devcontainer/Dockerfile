FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG QT_VERSION=6.11.2
ARG TARGETARCH

# GCC 14 and CMake >= 3.28 match the top-level CMakeLists.txt requirements.
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential g++-14 cmake ninja-build ccache gdb git git-lfs \
    ca-certificates curl sudo pkg-config python3 python3-venv pipx \
    uncrustify unzip zip p7zip-full autoconf automake libtool \
    libasound2-dev libcups2-dev libsndfile1-dev libvulkan-dev \
    libavcodec-dev libavformat-dev libswscale-dev \
    libgl1-mesa-dev libegl1-mesa-dev libfontconfig1-dev libfreetype-dev \
    libxkbcommon-dev libxkbcommon-x11-0 libxcb-cursor0 libxcb-icccm4 \
    libxcb-keysyms1 libxcb-shape0 libxcb-xinerama0 libxcb-randr0 \
    libxcb-render-util0 libxcb-xfixes0 libxcb-sync1 libxcb-xkb1 \
    libdbus-1-dev libudev-dev xvfb xauth \
    && rm -rf /var/lib/apt/lists/*

COPY patch-aqt.py /tmp/patch-aqt.py
RUN python3 -m venv /opt/tools \
    && /opt/tools/bin/pip install --no-cache-dir 'aqtinstall @ git+https://github.com/miurahr/aqtinstall.git' \
        'py7zr @ git+https://github.com/miurahr/py7zr.git' pdm \
    && /opt/tools/bin/python /tmp/patch-aqt.py \
    && case "${TARGETARCH}" in \
        amd64) qt_host=linux; qt_arch=linux_gcc_64 ;; \
        arm64) qt_host=linux_arm64; qt_arch=linux_gcc_arm64 ;; \
        *) echo "Unsupported architecture: ${TARGETARCH}"; exit 1 ;; \
       esac \
    && /opt/tools/bin/aqt install-qt "${qt_host}" desktop "${QT_VERSION}" "${qt_arch}" \
        --outputdir /opt/qt \
        --archives icu qtbase qtdeclarative qtsvg qttools qttranslations qtwayland \
        --modules qt5compat qtnetworkauth qtshadertools qtwebsockets \
    && qt_toolchain=$(find /opt/qt -name qt.toolchain.cmake -print -quit) \
    && test -n "${qt_toolchain}" \
    && ln -s "${qt_toolchain%/lib/cmake/Qt6/qt.toolchain.cmake}" /opt/qt/current

ENV QT_DIR=/opt/qt/current
ENV QTDIR=${QT_DIR}
ENV PATH=${QT_DIR}/bin:/opt/tools/bin:${PATH}
ENV CMAKE_PREFIX_PATH=${QT_DIR}
ENV CC=gcc-14 CXX=g++-14

# Ubuntu's base image already provides an unprivileged user with UID 1000.
RUN usermod -l vscode -d /home/vscode -m ubuntu \
    && echo 'vscode ALL=(ALL) NOPASSWD:ALL' > /etc/sudoers.d/vscode \
    && chmod 0440 /etc/sudoers.d/vscode
USER vscode
WORKDIR /workspaces/VirtScore
