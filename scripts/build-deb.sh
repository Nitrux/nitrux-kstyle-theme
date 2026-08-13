#!/usr/bin/env bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2024-2025 <Nitrux Latinoamericana S.C. <hello@nxos.org>>


# -- Exit on errors.

set -euo pipefail


# -- Compile Source

mkdir -p build && cd build

HOST_MULTIARCH=$(dpkg-architecture -qDEB_HOST_MULTIARCH)

cmake \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DENABLE_BSYMBOLICFUNCTIONS=OFF \
	-DQUICK_COMPILER=ON \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_SYSCONFDIR=/etc \
	-DCMAKE_INSTALL_LOCALSTATEDIR=/var \
	-DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=ON \
	-DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON \
	-DCMAKE_INSTALL_RUNSTATEDIR=/run "-GUnix Makefiles" \
	-DCMAKE_VERBOSE_MAKEFILE=ON \
	-DCMAKE_INSTALL_LIBDIR="/usr/lib/${HOST_MULTIARCH}" \
	..

make -j"$(nproc)"

make install


# -- Stage the install tree and build the Debian package.

DESTDIR="$(mktemp -d "$PWD/pkg.XXXXXX")"
trap 'rm -rf "$DESTDIR"' EXIT

install -d "$DESTDIR/usr/lib/${HOST_MULTIARCH}/qt6/plugins/styles"
DESTDIR="$DESTDIR" cmake --install . --config Release

mkdir -p "$DESTDIR/DEBIAN"

PKGNAME="nitrux-kstyle-theme"
ARCHITECTURE="${TARGET_ARCH:-$(dpkg --print-architecture)}"
if [[ ! "${ARCHITECTURE}" =~ ^[a-z0-9][a-z0-9+.-]*$ ]]; then
    echo "TARGET_ARCH contains invalid Debian architecture characters" >&2
    exit 1
fi

cat > "$DESTDIR/DEBIAN/control" <<EOF
Package: $PKGNAME
Version: $PACKAGE_VERSION
Section: utils
Priority: optional
Architecture: $ARCHITECTURE
Maintainer: uri_herrera@nxos.org
Provides: nitrux-kstyle-theme
Depends: frameworkintegration6, libkf6configcore6, libkf6coreaddons6, libkf6guiaddons6, libkf6i18n6, libkf6iconthemes6, libkf6kcmutils6, libkwaylandclient6, libkirigami6, libqt6core6t64, libqt6dbus6, libqt6gui6, libqt6widgets6
Description: Nitrux Qt6/KF6 KStyle.
 A Qt6/KF6 widget style based on Lightly, customized for Nitrux.
EOF

PACKAGE_FILE="$PWD/${PKGNAME}_${PACKAGE_VERSION}_${ARCHITECTURE}.deb"
dpkg-deb --build "$DESTDIR" "$PACKAGE_FILE"
