dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([shadowsocks-libev], [2.5.6], [max.c.lv@gmail.com])
AC_CONFIG_SRCDIR([src/encrypt.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR(auto)
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([subdir-objects foreign -Wno-gnu -Werror])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_MAINTAINER_MODE
AM_DEP_TRACK

dnl Checks for lib
AC_DISABLE_STATIC
AC_DISABLE_SHARED
LT_INIT([dlopen])

dnl Checks for using shared libraries from system
AC_ARG_ENABLE(
  [system-shared-lib],
  AS_HELP_STRING([--enable-system-shared-lib], [build against shared libraries when possible]),
  [
    case "${enableval}" in
      yes) enable_system_shared_lib=true ;;
      no) enable_system_shared_lib=false ;;
      *) AC_MSG_ERROR([bad value ${enableval} for --enable-system-shared-lib]) ;;
    esac], [enable_system_shared_lib=false])
AM_CONDITIONAL([USE_SYSTEM_SHARED_LIB], [test x$enable_system_shared_lib = xtrue])

dnl Checks for programs.
AC_PROG_CC
AM_PROG_AS
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_LANG_SOURCE
AC_PROG_CXX
AC_PROG_CXX_C_O

dnl Checks for golang
AC_ARG_ENABLE(golang,
[AS_HELP_STRING(--enable-golang, Compile go_quiet plugin using Go)],
[
  enable_golang="yes"
],
[
  enable_golang="no"
])

AC_CHECK_PROG([golang],[go],[yes],[no])
AM_CONDITIONAL([HAS_GOLANG], [test "x$enable_golang" = xyes && test "x$golang" = xyes])
if test "x$enable_golang" = xyes && test "x$golang" = xyes; then
  AC_MSG_NOTICE([golang compilation enabled])
  AC_MSG_CHECKING(for golang cross-compilation)
  GOARCH=""
  GOCROSS=""
  GOFLAGS=""
  HOSTARCH="$(echo "$host" | cut -d "-" -f 1)"
  if test "x$HOSTARCH" != x; then
  case $HOSTARCH in
    aarch64)
      GOARCH="GOARCH=arm64"
      ;;
    arm*)
      GOARCH="GOARCH=arm GOARM=7"
      ;;
    mips*)
      GOARCH="GOARCH=$HOSTARCH"
      ;;
    x86_64)
      GOARCH="GOARCH=amd64"
      ;;
    i*86)
      GOARCH="GOARCH=386"
      ;;
    *)
      GOARCH="GOARCH=$HOSTARCH"
      ;;
  esac
  fi
  if test "x$HOSTARCH" != x; then
  case $host in
  *-mingw*)
    GOCROSS="GOOS=windows $GOARCH CC=$CC"
    GOFLAGS="-ldflags=\"-extar $AR\""
    LIBS="$LIBS -lwinmm"
    AC_MSG_RESULT(MinGW)
    ;;
  *-apple-darwin*)
    GOCROSS="GOOS=darwin $GOARCH CC=$CC"
    GOFLAGS="-ldflags=\"-extar $AR\""
    AC_MSG_RESULT(Darwin)
    ;;
  *-linux-android*)
    GOCROSS="GOOS=android $GOARCH CC=$CC"
    GOFLAGS="-ldflags=\"-extar $AR\""
    AC_MSG_RESULT(Android)
    ;;
  *-linux-musl*)
    GOCROSS="GOOS=linux $GOARCH CC=$CC"
    GOFLAGS="-ldflags=\"-extar $AR\""
    AC_MSG_RESULT(Linux Musl)
    ;;
  *)
    AC_MSG_RESULT(Native)
    ;;
  esac
  else
    AC_MSG_RESULT(Native)
  fi
fi
AC_SUBST(GOCROSS)
AC_SUBST(GOFLAGS)

dnl Checks for libev
AM_COND_IF([USE_SYSTEM_SHARED_LIB],
  [],
  [m4_include([libev/libev.m4])])

dnl Add library for mingw
case $host in
  *-mingw*)
    LIBS="$LIBS -lws2_32"
    ;;
  *)
    ;;
esac

dnl Checks for TLS
AX_TLS([:], [:])

dnl Checks for crypto library
AC_DEFINE([USE_CRYPTO_MBEDTLS], [1], [Use mbed TLS library])

dnl Checks for Apple CommonCrypto API
AC_ARG_ENABLE(applecc,
  AS_HELP_STRING([--enable-applecc], [enable Apple CommonCrypto API support]),
  [
    AC_CHECK_HEADERS(CommonCrypto/CommonCrypto.h,
      [],
      [AC_MSG_ERROR([CommonCrypto header files not found.]); break]
    )
    AC_CHECK_FUNCS([CCCryptorCreateWithMode], ,
      [AC_MSG_ERROR([CommonCrypto API needs OS X (>= 10.7) and iOS (>= 5.0).]); break]
    )
    AC_DEFINE([USE_CRYPTO_APPLECC], [1], [Use Apple CommonCrypto library])
  ]
)

dnl Checks for inet_ntop
ss_FUNC_INET_NTOP

dnl Checks for host.
AC_MSG_CHECKING(for what kind of host)
case $host in
  *-linux*)
    os_support=linux
    AC_MSG_RESULT(Linux)
    ;;
  *-mingw*)
    os_support=mingw
    AC_MSG_RESULT(MinGW)
    ;;
  *)
    AC_MSG_RESULT(transparent proxy does not support for $host)
    ;;
esac

AC_MSG_CHECKING(for what arch of host)
case $host in
  aarch64-*)
    os_arch=arm64
    AC_MSG_RESULT(arm64)
    ;;
  arm64-*)
    os_arch=arm64
    AC_MSG_RESULT(arm64)
    ;;
  arm-*hf)
    os_arch=armhf
    AC_MSG_RESULT(armhf)
    ;;
  arm-*)
    os_arch=arm
    AC_MSG_RESULT(arm)
    ;;
  x86_64-*)
    os_arch=amd64
    AC_MSG_RESULT(Intel 64-bit)
    ;;
  i*86-*)
    os_arch=x86
    AC_MSG_RESULT(Intel 32-bit)
    ;;
  *)
    os_arch=other
    AC_MSG_RESULT(other)
    ;;
esac

dnl Checks for stack protector
GGL_CHECK_STACK_PROTECTOR([has_stack_protector=yes], [has_stack_protector=no])
# XXX - disable -fstack-protector due to missing libssp_nonshared
case "$host_os" in
     *aix*)
	AC_MSG_NOTICE([-fstack-protector disabled on AIX])
	has_stack_protector=no
	;;
     *sunos*)
	AC_MSG_NOTICE([-fstack-protector disabled on SunOS])
	has_stack_protector=no
	;;
     *solaris*)
	AC_MSG_NOTICE([-fstack-protector disabled on Solaris])
	has_stack_protector=no
	;;
esac

AC_ARG_ENABLE(ssp,
[AS_HELP_STRING(--disable-ssp,Do not compile with -fstack-protector)],
[
  enable_ssp="no"
],
[
  enable_ssp="yes"
])

if test x$has_stack_protector = xyes && test x$enable_ssp = xyes; then
   CFLAGS="$CFLAGS -fstack-protector"
   AC_MSG_NOTICE([-fstack-protector enabled in CFLAGS])
fi

AM_CONDITIONAL(BUILD_REDIRECTOR, test "$os_support" = "linux")
AM_CONDITIONAL(BUILD_WINCOMPAT, test "$os_support" = "mingw")
AM_CONDITIONAL(BUILD_ARM64, test "$os_arch" = "arm64")
AM_CONDITIONAL(BUILD_AMD64, test "$os_arch" = "amd64")
AM_CONDITIONAL(BUILD_X86, test "$os_arch" = "x86")
AM_CONDITIONAL(BUILD_ARMHF, test "$os_arch" = "armhf")
AM_CONDITIONAL(BUILD_ARM, test "$os_arch" = "arm")

dnl Checks for header files.
AC_CHECK_HEADERS([limits.h stdint.h inttypes.h arpa/inet.h fcntl.h langinfo.h locale.h netdb.h netinet/in.h stdlib.h string.h strings.h unistd.h sys/ioctl.h])

dnl A special check required for <net/if.h> on Darwin. See
dnl http://www.gnu.org/software/autoconf/manual/html_node/Header-Portability.html.
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([net/if.h], [], [],
[
#include <stdio.h>
#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
])

case $host in
  *-mingw*)
    AC_DEFINE([CONNECT_IN_PROGRESS], [WSAEWOULDBLOCK], [errno for incomplete non-blocking connect(2)])
    AC_CHECK_HEADERS([winsock2.h ws2tcpip.h], [], [AC_MSG_ERROR([Missing MinGW headers])], [])
    ;;
  *-linux*)
    AC_DEFINE([CONNECT_IN_PROGRESS], [EINPROGRESS], [errno for incomplete non-blocking connect(2)])
    dnl Checks for netfilter headers
    AC_CHECK_HEADERS([linux/if.h linux/netfilter_ipv4.h linux/netfilter_ipv6/ip6_tables.h],
                     [], [AC_MSG_ERROR([Missing netfilter headers])],
    [[
    #if HAVE_LIMITS_H
    #include <limits.h>
    #endif
    /* Netfilter ip(6)tables v1.4.0 has broken headers */
    #if HAVE_NETINET_IN_H
    #include <netinet/in.h>
    #endif
    #if HAVE_LINUX_IF_H
    #include <linux/if.h>
    #endif
    #if HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
    #endif
    ]])
    ;;
  *)
    # These are POSIX-like systems using BSD-like sockets API.
    AC_DEFINE([CONNECT_IN_PROGRESS], [EINPROGRESS], [errno for incomplete non-blocking connect(2)])
    ;;
esac

AC_C_BIGENDIAN

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SSIZE_T

dnl Checks for header files.
AC_HEADER_ASSERT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T
AC_HEADER_TIME

dnl Checks for library functions.
AC_FUNC_FORK
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([memset select setresuid setreuid strerror getpwnam_r setrlimit])

AC_CHECK_LIB(socket, connect)

dnl Checks for library functions.
AC_CHECK_FUNCS([malloc memset socket])

dnl Add define for libudns to enable IPv6 support
dnl This is an option defined in the origin configure script
AC_DEFINE([HAVE_IPv6], [1], [Enable IPv6 support in libudns])

AC_CONFIG_FILES([ shadowsocks-libev.pc
                 Makefile
                 re2/Makefile
                 libcork/Makefile
                 libipset/Makefile
                 libudns/Makefile
                 libev/Makefile
                 goquiet/Makefile
                 src/Makefile])
AM_COND_IF([USE_SYSTEM_SHARED_LIB],[
    AC_CHECK_LIB([mbedcrypto], [mbedtls_cipher_setup], ,[AC_MSG_ERROR([Couldn't find mbedtls. Try installing libmbedtls-dev@<:@el@:>@.])])
  ],
  [AC_CONFIG_FILES([mbedtls/Makefile])])

AC_OUTPUT
