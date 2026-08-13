#!/usr/bin/env bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2024-2025 <Nitrux Latinoamericana S.C. <hello@nxos.org>>


# -- Exit on errors.

set -euo pipefail


# -- Check if running as root.

if [ "$EUID" -ne 0 ]; then
    APT_COMMAND=(sudo apt-get)
else
    APT_COMMAND=(apt-get)
fi


# -- Install build packages.

"${APT_COMMAND[@]}" update -q
"${APT_COMMAND[@]}" install -y --no-install-recommends \
    build-essential \
    cmake \
    extra-cmake-modules \
    libkf6coreaddons-dev \
    libkf6i18n-dev \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-svg-dev \
    libkf6colorscheme-dev \
    libkf6config-dev \
    libkf6configwidgets-dev \
    libkf6guiaddons-dev \
    libkf6iconthemes-dev \
    libkf6kcmutils-dev \
    kwayland-dev \
    libkirigami-dev \
    qt6-wayland-dev
