noinst_LTLIBRARIES = libmbedcrypto.la
libmbedcrypto_la_SOURCES = library/aes.c library/aesni.c library/arc4.c \
		library/aesarm.c library/aesasm.S library/aesasm_wrap.c \
		library/aesasm_hard.S				\
		library/asn1parse.c library/asn1write.c library/base64.c \
		library/bignum.c library/blowfish.c library/camellia.c \
		library/ccm.c library/cipher.c library/cipher_wrap.c \
		library/cmac.c library/ctr_drbg.c library/des.c \
		library/dhm.c library/ecdh.c library/ecdsa.c \
		library/ecjpake.c library/ecp.c \
		library/ecp_curves.c library/entropy.c library/entropy_poll.c \
		library/error.c library/gcm.c library/havege.c \
		library/hmac_drbg.c library/md.c library/md2.c \
		library/md4.c library/md5.c library/md_wrap.c \
		library/memory_buffer_alloc.c library/oid.c \
		library/padlock.c library/pem.c library/pk.c \
		library/pk_wrap.c library/pkcs12.c library/pkcs5.c \
		library/pkparse.c library/pkwrite.c library/platform.c \
		library/ripemd160.c library/rsa_internal.c library/rsa.c  		\
		library/sha1.c library/sha256.c library/sha512.c \
		library/threading.c library/timing.c library/version.c \
		library/version_features.c library/xtea.c \
		library/md5asm.S library/sha1asm.S library/sha256asm.S

libmbedcrypto_la_CPPFLAGS = -I$(top_srcdir)/mbedtls/include -D_FILE_OFFSET_BITS=64
libmbedcrypto_la_CFLAGS = -O2 -Wall -W -Wdeclaration-after-statement
if BUILD_ARM64
libmbedcrypto_la_CFLAGS += -march=armv8-a+crypto -mcpu=cortex-a53
endif
