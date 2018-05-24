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

# Project URL
PROJ_SITE=$REPO   # Change REPO in Makefile
PROJ_URL=https://github.com/${PROJ_SITE}/shadowsocks-libev.git
PROJ_REV=$REV     # Change REV in Makefile

dk_build() {
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
    pushd "$SRC/proj/python"
    make ss-server
    make
    popd
    pushd "$SRC/proj/goquiet"
    GOPATH="$PWD" GOOS=linux GOARCH=amd64 \
    go build -ldflags "-s -w -X main.version=1.1.2" \
     -v -o gq-server ./src/github.com/cbeuw/GoQuiet/cmd/gq-server
    popd
}

dk_package() {
    rm -rf "$BASE/pack"
    mkdir -p "$BASE/pack"
    cd "$BASE/pack"
    mkdir -p ssr-static
    cd ssr-static
    mkdir linux-x64-server
    pushd linux-x64-server
    cp "$SRC/proj/python/ss-server" .
    cp "$SRC/proj/python/siphashc.so" .
    cp "$SRC/proj/goquiet/gq-server" .
    popd
    cp -r "$SRC/proj/python/windows" server
    rm -f server/pylib.zip
    mv server/ss-server.exe server/ssr-server.exe
    mv server win32-server
    echo "ShadowsocksR Python Server Release" > $BASE/pack/checksum
    echo "Build $(date +"%y%m%d"): Git-${PROJ_REV}" >> $BASE/pack/checksum
    echo "SHA256 Checksum:" >> $BASE/pack/checksum
    find . -type f | while read f; do
        echo "  $(basename $f):" >> $BASE/pack/checksum
        echo "    $(sha256sum $f | cut -d ' ' -f 1)" >> $BASE/pack/checksum
    done
    sed -e 's/$/\r/' $BASE/pack/checksum > checksum.txt
    rm -f $BASE/pack/checksum
    cd ..
    mv ssr-static ssr-server
    tar zcf /bin.tgz ssr-server
}
