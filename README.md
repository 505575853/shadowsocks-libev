# SSR libev

_Who will use this any more?_

## Disclaimers

This is my own modified version of shadowsocksR libev (client only, Python server is embeded in [`python/`](python/)). It is based on the [last official commit][last commit] of the original repo. This version is by no means a continuation of the original project, nor do I have any connection to it. It only contains some experimental code for my own use. I do not accept any pull requests in this repo. Please contribute to the [upstream project][upstream].

## Extra Features

### Code base cleaned up

* __libsodium support removed__, only support `chacha20-ietf` from mbedTLS
* [Embed mbedTLS][mbedtls] with AES hardware/assembly acceleration:
  * Hardware AES: Intel/ARM 64-bit on any OS, Intel 32-bit on Windows only (CTR-mode is accelerated for any OS on 32-bit Intel)
  * Assembly AES: ARM/MIPS 32-bit on Linux only, using assembly generated by OpenSSL project

### Better Windows support

* Backport upstream MinGW patches originally written by myself
* [IOCP-based libev][libev] backend powered by [wepoll][wepoll]
* TCP Fast Open support on Windows 10
* Support SIP003 multi-process plugin on Windows

### Simpler build using Docker

* Supported OS:
  * Linux/macOS on 64-bit Intel
  * Windows on 32/64-bit Intel
  * Windows on 64-bit ARM (for future-proof, [tested on QEMU](arm.png))
* Support to build using Docker in a simple way:
  * Build all clients by `cd docker/client && make` on Unix-like system
  * Double-click `docker\client\make.bat` on Windows
* All generated executables are __fully portable__:
  * Statically linked with no dependency
  * Just copy the single executable file and run

### GoQuiet support

* [GoQuiet][goquiet] obfuscation protocol implemented as __native SSR obfs plugin__
* Usage:
    * `obfs` set to `go_quiet`
    * `obfs_param` set to the the server name you want to expose in TLS handshake (same as the original [`ServerName` option][gq-config] of GoQuiet)
    * Note that [`Key` option][gq-config] of GoQuiet client is __the same__ as `password` option in SSR and cannot be changed at the moment.
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

### New protocol

* `auth_aes128_fast`: similar to `auth_aes128_*`, but using a simple and fast [SipHash-1-3][siphash] to replace HMAC-MD5/SHA1. The speed is at least __2x faster__ than MD5 or SHA-1 hash. Note that the round is [reduced][rust-siphash] for better throughput while normal use is SipHash-2-4.

## Simple Benchmark for _Crypto_

### Environment

* CPU: 2.3 GHz Intel Core i5 6300HQ
* OS: macOS 10.13.3 (Darwin 17.4.0)
* Both client and server run locally __on the same machine__. So the actual transfer speed should be doubled for running client only.
* The speed is tested using iperf 3.3 tunneled through the client running 10 seconds. The command is `iperf3 -p <port> -c <ip> -R` which measures the *download speed*. The result is a truncated mean of 5 tests.
* All programs are compiled against the same external libraries, including my customized [mbedtls][my-mbedtls] and [libev][my-libev]:
  * [AES-CTR mode][ctr-acc] is accelerated using AES-NI and SSE instructions and well-optimized by unrolling loops.
  * AES-GCM mode is hardware-accelerated via AES-NI by default in mbedtls (but not extremely optimized).
  * MD5, SHA-1 and SHA-2 are optimized using [hand-written x86/x64 assembly][md-asm] by [Project Nayuki][nayuki].
  * Forcibly enable Libev's kqueue backend. (Note that kqueue backend of libev on macOS is [not enabled][no-kqueue] by default which __significantly slows down__ the transfer speed.)

### Result

* ss-libev (ss-tunnel & server version: v3.1.3, commit `eb53d54`)

Crypto|Crypto Type|Speed|% of `aes-256-ctr`
------|-----------|-----|----------
rc4-md5 | Stream | 988 Mbits/sec | 65%
aes-128-cfb | Stream | 894 Mbits/sec| 59%
chacha20 | Stream | 1.42 Gbits/sec| 93%
aes-256-ctr | Stream | 1.52 Gbits/sec| 100%
aes-128-gcm | AEAD | 564 Mbits/sec| 37%
chacha20-ietf-poly1305 | AEAD | 984 Mbits/sec| 65%

* This repo (client version: commit `7b60d2c`; python server is compiled using [nuitka][nuitka]; GoQuiet server version `464a11e` is compiled using Go 1.10)

Crypto|Protocol|Obfuscation|Speed|% of `aes-256-ctr`
------|--------|-----------|-----|----------
none | `plain` | `plain` | 1.90 Gbits/sec | 122%
rc4-md5 | `plain` | `plain` | 1.00 Gbits/sec | 64%
aes-128-cfb | `plain` | `plain` | 925 Mbits/sec | 59%
aes-256-ctr | `plain` | `plain` | 1.56 Gbits/sec | 100%
aes-256-ctr | `auth_aes128_fast` | `plain` | 1.41 Gbits/sec | 90%
aes-256-ctr | `auth_sha1_v4` | `plain` | 1.23 Gbits/sec | 79%
aes-256-ctr | `auth_aes128_md5` | `plain` | 697 Mbits/sec | 45%
none | `auth_chain_a` | `plain` | 295 Mbits/sec | 19%
aes-256-ctr | `auth_aes128_fast` | `go_quiet` | 1.25 Gbits/sec | 80%
aes-256-ctr | `auth_aes128_fast` | `tls1.2_ticket_auth` | 951 Mbits/sec | 61%

## Downloads

See [releases][releases].

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
[siphash]: https://en.wikipedia.org/wiki/SipHash
[rust-siphash]: https://github.com/rust-lang/rust/issues/29754
[my-mbedtls]: https://github.com/linusyang92/mbedtls
[ctr-acc]: https://github.com/linusyang92/mbedtls/blob/mingw/library/aesasm_wrap.c
[nuitka]: http://nuitka.net/
[my-libev]: https://github.com/linusyang92/libev
[no-kqueue]: https://github.com/shadowsocks/libev/blob/9738503d99938dec56c66dd2022b9964cb64dfc3/ev.c#L2763
[nayuki]: https://www.nayuki.io/page/fast-md5-hash-implementation-in-x86-assembly
[md-asm]: https://github.com/linusyang92/mbedtls/blob/mingw/library/md5asm.S
[releases]: https://github.com/linusyang92/shadowsocks-libev/releases
