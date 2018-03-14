@echo off
pushd %~dp0
set "REPO=linusyang92"
set "REV=mingw"
set "IMAGE=ss-build-mingw-asm"
set "DIST=ss-libev-win-dist.tar.gz"
docker build --force-rm -t %IMAGE% ^
      --build-arg REV=%REV% --build-arg REPO=%REPO% ^
      --build-arg REBUILD=%RANDOM% .
docker run --rm --entrypoint cat %IMAGE% /bin.tgz > %DIST%
pause
