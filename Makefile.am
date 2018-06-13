if USE_SYSTEM_SHARED_LIB
SUBDIRS = sregex libcork libipset libudns src
else
if HAS_GOLANG
SUBDIRS = sregex libcork libipset libudns libev mbedtls goquiet src
else
SUBDIRS = sregex libcork libipset libudns libev mbedtls src
endif
endif

ACLOCAL_AMFLAGS = -I m4

pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = shadowsocks-libev.pc
