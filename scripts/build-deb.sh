#!/usr/bin/env bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2024-2025 <Nitrux Latinoamericana S.C. <hello@nxos.org>>


# -- Exit on errors.

set -euo pipefail


SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$SCRIPT_DIR"
# -- Compile Source

mkdir -p build
cd build

HOST_MULTIARCH=$(dpkg-architecture -qDEB_HOST_MULTIARCH)
PACKAGE_VERSION="${PACKAGE_VERSION:-6.80}"
if [[ ! "$PACKAGE_VERSION" =~ ^[0-9][0-9A-Za-z.+:~-]*$ ]]; then
    printf "Invalid Debian package version: %s\n" "$PACKAGE_VERSION" >&2
    exit 1
fi

cmake \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DCMAKE_BUILD_TYPE=Release \
	-DKDE_INSTALL_SYSCONFDIR=/etc \
	-DKDE_INSTALL_LOCALSTATEDIR=/var \
	-DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=ON \
	-DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON \
	-DKDE_INSTALL_RUNSTATEDIR=/run "-GUnix Makefiles" \
	-DCMAKE_VERBOSE_MAKEFILE=ON \
	-DWITH_DECORATIONS=OFF \
	-DKDE_INSTALL_LIBDIR="/usr/lib/${HOST_MULTIARCH}" \
	..
make -j"$(nproc)"


# -- Run checkinstall and Build Debian Package

>> description-pak printf "%s\n" \
	'Nitrux Qt6/KF6 KStyle.' \
	'' \
	'A Qt6/KF6 widget style based on Lightly, customized for Nitrux.' \
	''

checkinstall -D -y \
	--install=no \
	--fstrans=yes \
	--pkgname=nitrux-kstyle-theme \
	--pkgversion="$PACKAGE_VERSION" \
	--pkgarch="$(dpkg --print-architecture)" \
	--pkgrelease="1" \
	--pkglicense=GPL-2 \
	--pkggroup=utils \
	--pkgsource=nitrux-kstyle-theme \
	--pakdir=. \
	--maintainer=uri_herrera@nxos.org \
	--provides=nitrux-kstyle-theme \
	--requires="frameworkintegration6,libkf6configcore6,libkf6coreaddons6,libkf6guiaddons6,libkf6i18n6,libkf6iconthemes6,libkf6kcmutils6,libkf6windowsystem6,libkwaylandclient6,libkirigami6,libqt6core6t64,libqt6dbus6,libqt6gui6,libqt6widgets6" \
	--nodoc \
	--strip=no \
	--stripso=yes \
	--reset-uids=yes \
	--deldesc=yes
