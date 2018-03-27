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
    host=$1
    prefix=${DIST}/$arch
    dep=${PREFIX}/$arch
    FLAGS="-L${dep}/lib"
    [[ "$host" != *-darwin* ]] && FLAGS="-all-static ${FLAGS}"
    MUSL=""
    [[ "$host" == *-linux-musl* ]] && MUSL="-I${PREFIX}/sysheader"

    cd "$SRC"
    if ! [ -d proj ]; then
        git clone ${PROJ_URL} proj
        cd proj
        git checkout ${PROJ_REV}
    else
        cd proj
    fi
    ./configure --host=${host} --prefix=${prefix} \
      --with-pcre="$dep" CFLAGS="${MUSL} -DPCRE_STATIC"
    make clean
    make LDFLAGS="$FLAGS"
    make install-strip
}

dk_build() {
    mkdir -p "${PREFIX}/sysheader"
    ln -s /usr/include/linux "${PREFIX}/sysheader/"
    for arch in "${HOSTS[@]}"; do
        build_proj $arch
    done
}

dk_package() {
    pushd "$SRC/proj"
    GIT_REV="$(git rev-parse --short HEAD)"
    popd
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
    echo "Build $(date +"%y%m%d"): Git-${GIT_REV}" >> $BASE/pack/checksum
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
