if USE_SYSTEM_SHARED_LIB
SUBDIRS = libcork libipset mbedtls src
else
SUBDIRS = libcork libipset libudns libev mbedtls src
endif

ACLOCAL_AMFLAGS = -I m4

pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = shadowsocks-libev.pc
