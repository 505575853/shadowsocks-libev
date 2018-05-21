#
# Makefile for building MinGW port
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

REPO=linusyang92
REV=0fe8f47
IMAGE=ssr-build-mingw-arm64
DIST=ssr-win-arm64.tar.gz

all: build

deps:
	docker build --force-rm -t linusyang/mingw-deps -f Dockerfile.deps .

build:
	OLDPWD="$$PWD"; \
		cd "$$(git rev-parse --show-toplevel)"; \
		git archive --format=tar.gz --prefix=proj/ $(REV) > $$OLDPWD/archive.tar.gz
	docker build --force-rm -t $(IMAGE) \
	  --build-arg REV=$(REV) \
	  --build-arg REPO=$(REPO) \
	  --build-arg REBUILD="$$(date +%Y-%m-%d-%H-%M-%S)" .
	docker run --rm --entrypoint cat $(IMAGE) /bin.tgz > $(DIST)

clean:
	rm -f $(DIST) archive.tar.gz

distclean: clean
	docker rmi $(IMAGE) || true

.PHONY: all clean build distclean
