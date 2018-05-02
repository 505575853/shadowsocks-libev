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

# Supported hosts
declare -a HOSTS=(
    x86_64-linux-musl
    x86_64-apple-darwin14
)

declare -a HOSTS_SUFFIX=(
    linux-x64
    macos
)

# Project URL
PROJ_SITE=$REPO   # Change REPO in Makefile
PROJ_REV=$REV     # Change REV in Makefile
PROJ_URL=https://github.com/${PROJ_SITE}/shadowsocks-libev.git

# Third-party libraries

## PCRE
PCRE_VER=8.41
PCRE_SRC=pcre-${PCRE_VER}
PCRE_URL=https://ftp.pcre.org/pub/pcre/${PCRE_SRC}.tar.gz

# Build steps

dk_prepare() {
    apt-get update -y
    apt-get install -y llvm-4.0 p7zip-full zip libpython2.7-dev python-setuptools
    rm -f /usr/osxcross/bin/x86_64-apple-darwin14-gcc
    echo '#!/bin/bash' > /usr/osxcross/bin/x86_64-apple-darwin14-gcc
    echo 'exec /usr/osxcross/bin/x86_64-apple-darwin14-clang "$@"' >> /usr/osxcross/bin/x86_64-apple-darwin14-gcc
    chmod 755 /usr/osxcross/bin/x86_64-apple-darwin14-gcc
    ln -s /usr/lib/llvm-4.0/lib/libLTO.so /usr/lib/
}

dk_download() {
    mkdir -p "${SRC}"
    cd "${SRC}"
    DOWN="aria2c --file-allocation=trunc -s10 -x10 -j10 -c"
    for pkg in PCRE; do
        src=${pkg}_SRC
        url=${pkg}_URL
        out="${!src}".tar.gz
        $DOWN ${!url} -o "${out}"
        echo "Unpacking ${out}..."
        tar zxf ${out}
    done
}
