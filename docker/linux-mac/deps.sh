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
    host=$1
    prefix=${PREFIX}/$arch
    args="--host=${host} --prefix=${prefix} --disable-shared --enable-static"

    # pcre
    cd "$SRC/$PCRE_SRC"
    ./configure $args \
      --enable-jit --disable-cpp \
      --enable-unicode-properties
    make clean
    make install
}

dk_deps() {
    pushd "$SRC/$PCRE_SRC"
    patch -p0 < /pcre.patch
    popd
    for arch in "${HOSTS[@]}"; do
        build_deps $arch
    done
}
