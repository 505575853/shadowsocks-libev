@echo off
pushd %~dp0
set "REPO=linusyang92"
set "REV=ssr"
set "IMAGE=ssr-build-linux-mac"
set "DIST=ssr-linux-mac.zip"
docker build --force-rm -t %IMAGE% ^
      --build-arg REV=%REV% --build-arg REPO=%REPO% ^
      --build-arg REBUILD=%RANDOM% .
docker run --rm --entrypoint cat %IMAGE% /%DIST% > %DIST%
pause
