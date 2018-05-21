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

. /prepare.sh

# Project URL
PROJ_SITE=$REPO   # Change REPO in Makefile
PROJ_URL=https://github.com/${PROJ_SITE}/shadowsocks-libev.git
PROJ_REV=$REV     # Change REV in Makefile

build_proj() {
    arch=$1
    host=$arch-w64-mingw32
    prefix=${DIST}/$arch
    dep=${PREFIX}/$arch

    mkdir -p "$SRC"
    cd "$SRC"
    if ! [ -d proj ]; then
        if [ -f /archive.tar.gz ]; then
            tar xf /archive.tar.gz
        else
            git clone ${PROJ_URL} proj
            pushd proj
            git checkout ${PROJ_REV}
            popd
        fi
    fi
    cd proj
    ./configure --host=${host} --prefix=${prefix} CFLAGS="" CXXFLAGS=""
    make clean
    make LDFLAGS="-all-static -L${dep}/lib -lssp"
    make install-strip
}

dk_build() {
    update-alternatives --set i686-w64-mingw32-g++ /usr/bin/i686-w64-mingw32-g++-posix
    update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
    for arch in i686 x86_64; do
        build_proj $arch
    done
    pushd "$SRC/proj/python"
    make
    popd
}

dk_package() {
    rm -rf "$BASE/pack"
    mkdir -p "$BASE/pack"
    cd "$BASE/pack"
    mkdir -p ssr-libev-${PROJ_REV}
    cd ssr-libev-${PROJ_REV}
    for bin in local; do
        cp ${DIST}/i686/bin/ss-${bin}.exe ssr-${bin}-x86.exe
        cp ${DIST}/x86_64/bin/ss-${bin}.exe ssr-${bin}-x64.exe
    done
    cp -r "$SRC/proj/python/windows" server
    rm -f server/pylib.zip
    mv server/ss-server.exe server/ssr-server.exe
    echo "SHA1 checksum for build $(date +"%y%m%d")-${PROJ_REV}" > checksum
    for f in *.exe; do
        echo "  $f:" >> checksum
        echo "    $(sha1sum $f | cut -d ' ' -f 1)" >> checksum
    done
    sed -e 's/$/\r/' checksum > checksum.txt
    rm -f checksum
    cd ..
    tar zcf /bin.tgz ssr-libev-${PROJ_REV}
}
