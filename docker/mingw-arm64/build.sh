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

build_proj() {
    arch=$1
    host=$arch-w64-mingw32
    prefix=${DIST}/$arch
    dep=${PREFIX}/$arch

    cd "$SRC"
    if ! [ -d proj ]; then
        tar xf /archive.tar.gz
    fi
    cd proj
    ./configure --host=${host} --prefix=${prefix} \
      --with-pcre="$dep" CFLAGS="-DPCRE_STATIC"
    make clean
    make LDFLAGS="-s -all-static -L${dep}/lib"
    make install
}

dk_build() {
    build_proj aarch64
}

dk_package() {
    rm -rf "$BASE/pack"
    mkdir -p "$BASE/pack"
    cd "$BASE/pack"
    mkdir -p ssr-libev-${PROJ_REV}
    cd ssr-libev-${PROJ_REV}
    for bin in local; do
        cp ${DIST}/aarch64/bin/ss-${bin}.exe ssr-${bin}-arm64.exe
    done
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
