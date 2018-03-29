# SSR libev

_Who will use this any more?_

## Disclaimer

This is my own modified version of shadowsocksR libev (client only). It is based on the [last official commit][last commit] of the original repo. This version is by no mean a continuation of the original project, nor do I have any connection to it. It only contains some experimental code for my own use. I do not accept any pull requests in this repo. Please contribute to the [upstream project][upstream].

## Extra Features

### Code base cleaned up

* __libsodium support removed__, no NaCl cryptos
* [Embed mbedTLS][mbedtls] with AES hardware/assembly acceleration:
  * Hardware AES: Intel/ARM 64-bit on any OS, Intel 32-bit on Windows only
  * Assembly AES: ARM/MIPS 32-bit on Linux

### Better Windows support

* Backport upstream MinGW patches originally written by myself
* [IOCP-based libev][libev] backend powered by [wepoll][wepoll]
* TCP Fast Open support on Windows 10
* Support SIP003 multi-process plugin on Windows

### Simpler build using Docker

* Supported OS:
  * `docker/mingw` for Windows on 32/64-bit Intel
  * `docker/mingw-arm64` for Windows on 64-bit ARM (for future-proof, tested it [on QEMU][arm.png])
  * `docker/linux-mac` for Linux/macOS on 64-bit Intel
* Support to build using Docker in a simple way:
  * Type `make` in the directory for Unix-like system
  * Double-click `make.bat` on Windows
* All generated executables are __fully portable__:
  * Statically linked with no dependency
  * Just copy the single file and run

### GoQuiet support

* [GoQuiet][goquiet] obfuscation protocol implemented as __native SSR obfs plugin__
* Usage:
    * `obfs` set to `go_quiet`
    * `obfs_param` set to the the server name you want to expose in TLS handshake (same as the original [`ServerName` option][gq-config] of GoQuiet)
    * Note that [`Key` option][gq-config] of GoQuiet client is the same as `password` and cannot be changed at the moment.
* Support to build using either the [C version][gq-c] (rewritten by myself, by default), or the original [Go version][gq-go] (enable by `--enable-golang` when running `configure`)

### Use other SIP003 plugins

* Use any compatible SIP003 plugins for obfuscation (e.g. [simple-obfs][simple-obfs], [GoQuiet][goquiet])
* Usage: [same][plugin usage] as original project by specifying `--plugin` and `--plugin-opts` in command-line options

### Run as SIP003 plugin

* Run as an SIP003 plugin to provide obfuscation service to other shadowsocks client
* No standalone mode, only accept [environmental variables][sip003] as configuration
* Usage: in other client specify the executable name as `plugin` and setup `plugin-opts` (required)
    * `obfs` is the obfuscation protocol, currently supported: all original SSR protocols (`http_simple, http_post, tls1.2_ticket_auth`) and GoQuiet (`go_quiet`)
    * `param` is the parameters for obfuscation. For SSR protocols, check the [original documents][ssrdoc]. For GoQuiet, `param` is the same as [`ServerName` option][gq-config].
    * `key` is only used for GoQuiet, which is the key for accessing server (same as [`Key` option][gq-config]).
* Example: run as a GoQuiet client plugin

        --plugin "ssr-local" --plugin-opts "obfs=go_quiet;param=bing.com;key=pass"

## Downloads?

Sorry, no downloads. Build on your own if needed.

## License

GPLv3, same as the original project

All rights reserved by original authors


[upstream]: https://github.com/shadowsocks/shadowsocks-libev
[last commit]: https://github.com/linusyang92/shadowsocks-libev/commit/f713aa981169d35ff9483b295d1209c35117d70c
[mbedtls]: https://github.com/linusyang92/shadowsocks-libev/tree/ssr/mbedtls
[wepoll]: https://github.com/piscisaureus/wepoll
[libev]: https://github.com/shadowsocks/libev/tree/mingw
[plugin usage]: https://github.com/shadowsocks/simple-obfs/blob/master/README.md#usage
[sip003]: https://github.com/shadowsocks/shadowsocks-org/issues/28
[ssrdoc]: https://github.com/shadowsocksr-backup/shadowsocks-rss/blob/master/ssr.md
[goquiet]: https://github.com/cbeuw/GoQuiet
[simple-obfs]: https://github.com/shadowsocks/simple-obfs
[gq-config]: https://github.com/cbeuw/GoQuiet/blob/master/README.md#configuration
[gq-c]: https://github.com/linusyang92/shadowsocks-libev/blob/ssr/src/libgoquiet.c
[gq-go]: https://github.com/linusyang92/shadowsocks-libev/tree/ssr/goquiet

