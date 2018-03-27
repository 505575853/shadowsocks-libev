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

build_deps() {
    arch=$1
    host=$arch-w64-mingw32
    prefix=${PREFIX}/$arch
    args="--host=${host} --prefix=${prefix} --disable-shared --enable-static"

    # pcre
    cd "$SRC/$PCRE_SRC"
    ./configure $args \
      --enable-jit --disable-cpp \
      --enable-unicode-properties
    make clean
    make install

    # libssp
    cd "$SRC/$GCC_SRC/libssp"
    rm -Rf build
    mkdir -p build
    cd build
    ../configure $args --disable-multilib
    make clean
    make install

    # pthread
    cd "$SRC/$PTHREAD_SRC/mingw-w64-libraries/winpthreads"
    ./configure $args
    make clean
    make install
}

dk_deps() {
    pushd /build/prefix/bin
    mv x86_64-w64-mingw32-windresreal aarch64-w64-mingw32-windresreal
    sed -i 's/x86_64/aarch64/g' aarch64-w64-mingw32-windres
    popd
    build_deps aarch64
}
