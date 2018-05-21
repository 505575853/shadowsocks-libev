if USE_SYSTEM_SHARED_LIB
SUBDIRS = libcork libipset mbedtls src
else
if HAS_GOLANG
SUBDIRS = re2 libcork libipset libudns libev mbedtls goquiet src
else
SUBDIRS = re2 libcork libipset libudns libev mbedtls src
endif
endif

ACLOCAL_AMFLAGS = -I m4

pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = shadowsocks-libev.pc
