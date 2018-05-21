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
PREFIX="$BASE/stage"
SRC="$BASE/src"
DIST="$BASE/dist"

# Third-party libraries

## PCRE
PCRE_VER=8.42
PCRE_SRC=pcre-${PCRE_VER}
PCRE_SUFFIX=tar.gz
PCRE_URL=https://ftp.pcre.org/pub/pcre/${PCRE_SRC}.${PCRE_SUFFIX}

## GCC
GCC_VER=7.3.0
GCC_SRC=gcc-${GCC_VER}
GCC_SUFFIX=tar.xz
GCC_URL=https://ftp.gnu.org/gnu/gcc/${GCC_SRC}/${GCC_SRC}.${GCC_SUFFIX}

## PTHREAD
PTHREAD_VER=arm64
PTHREAD_SRC=mingw-w64-${PTHREAD_VER}
PTHREAD_SUFFIX=tar.gz
PTHREAD_URL=https://github.com/linusyang92/mingw-w64/archive/${PTHREAD_VER}.${PTHREAD_SUFFIX}

# Build steps

dk_prepare() {
    apt-get update -y
    apt-get install --no-install-recommends -y \
      aria2 git make xz-utils ca-certificates
}

dk_download() {
    mkdir -p "${SRC}"
    cd "${SRC}"
    DOWN="aria2c --file-allocation=trunc -s10 -x10 -j10 -c"
    for pkg in PCRE GCC PTHREAD; do
        src=${pkg}_SRC
        url=${pkg}_URL
        suffix=${pkg}_SUFFIX
        out="${!src}"."${!suffix}"
        $DOWN ${!url} -o "${out}"
        echo "Unpacking ${out}..."
        tar xf ${out}
    done
}
