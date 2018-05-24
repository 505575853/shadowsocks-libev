#!/bin/bash
#
# Functions for building MinGW port in Docker
#
# This file is part of the shadowsocks-libev.
#
# shadowsocks-libev is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# shadowsocks-libev is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with shadowsocks-libev; see the file COPYING. If not, see
# <http://www.gnu.org/licenses/>.
#

# Exit on error
set -e

# Build options
BASE="/build"
SRC="$BASE/src"
DIST="$BASE/dist"

# Supported hosts
declare -a HOSTS=(
    x86_64-linux-musl
    x86_64-apple-darwin15
    i686-w64-mingw32
    x86_64-w64-mingw32
    aarch64-w64-mingw32
)

declare -a HOSTS_SUFFIX=(
    linux-x64
    macos
    win32.exe
    win64.exe
    win-arm64.exe
)

# Project URL
PROJ_SITE=$REPO   # Change REPO in Makefile
PROJ_URL=https://github.com/${PROJ_SITE}/shadowsocks-libev.git
PROJ_REV=$REV     # Change REV in Makefile

build_proj() {
    arch=$1
    host=$1
    prefix=${DIST}/$arch
    cpu="$(nproc --all)"
    LDFLAGS="-all-static"
    CFLAGS=""
    CC="clang"
    CXX="clang++"
    if [[ "$host" == *-darwin* ]]; then
        LDFLAGS=""
        CFLAGS="-mmacosx-version-min=10.9"
    elif [[ "$host" == *-linux-musl* ]]; then
        CFLAGS="-I${BASE}/sysheader ${CFLAGS}"
        CC="gcc"
        CXX="c++"
    elif [[ "$host" == *-mingw32 ]]; then
        LDFLAGS="${LDFLAGS} -lssp"
    fi
    mkdir -p "$SRC"
    cd "$SRC"
    if ! [ -d proj ]; then
        ARC="/archive.tar.gz"
        if [ -f "$ARC" ] && [ -s "$ARC" ]; then
            tar xf "$ARC"
        else
            git clone ${PROJ_URL} proj
            pushd proj
            git checkout ${PROJ_REV}
            popd
        fi
    fi
    cd proj
    (>&2 echo "Building for ${host}...")
    ./configure --host=${host} --prefix=${prefix} \
      CFLAGS="${CFLAGS}" CXXFLAGS="${CFLAGS}" \
      CC=${host}-${CC} CXX=${host}-${CXX} >/dev/null 2>&1
    make clean >/dev/null 2>&1
    make -j$cpu LDFLAGS="${LDFLAGS}"
    make install-strip >/dev/null 2>&1
}

dk_build() {
    mkdir -p "${BASE}/sysheader"
    ln -s /usr/include/linux "${BASE}/sysheader/"
    mkdir -p /System/Library/Frameworks
    for arch in "${HOSTS[@]}"; do
        build_proj $arch
    done
}

dk_package() {
    rm -rf "$BASE/pack"
    mkdir -p "$BASE/pack"
    cd "$BASE/pack"
    mkdir -p ssr-static
    cd ssr-static
    for ((idx=0; idx<${#HOSTS[@]}; ++idx)); do
        host="${HOSTS[idx]}"
        suffix="${HOSTS_SUFFIX[idx]}"
        folder="${suffix%.*}"
        for bin in ${DIST}/$host/bin/*; do
            fullname="$(basename $bin)"
            name="${fullname%.*}-$suffix"
            mkdir -p "${folder}"
            cp $bin "${folder}"/$name
        done
    done
    echo "ShadowsocksR Static Binary Release" > $BASE/pack/checksum
    echo "Build $(date +"%y%m%d"): Git-${PROJ_REV}" >> $BASE/pack/checksum
    echo "SHA256 Checksum:" >> $BASE/pack/checksum
    find . -type f | while read f; do
        echo "  $(basename $f):" >> $BASE/pack/checksum
        echo "    $(sha256sum $f | cut -d ' ' -f 1)" >> $BASE/pack/checksum
    done
    sed -e 's/$/\r/' $BASE/pack/checksum > checksum.txt
    rm -f $BASE/pack/checksum
    cd ..
    tar zcf /bin.tgz ssr-static
}
