@echo off
pushd %~dp0
set "REPO=linusyang92"
set "REV=ssr"
set "IMAGE=ssr-build-server"
set "DIST=ssr-server.tar.gz"
type NUL > archive.tar.gz
docker build --force-rm -t %IMAGE% ^
      --build-arg REV=%REV% --build-arg REPO=%REPO% ^
      --build-arg REBUILD=%RANDOM% .
docker run --rm --entrypoint cat %IMAGE% /bin.tgz > %DIST%
pause
