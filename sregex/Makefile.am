noinst_LTLIBRARIES = libsregex.la
libsregex_la_SOURCES = \
   src/sregex/sre_palloc.c \
   src/sregex/sre_regex.c \
   src/sregex/sre_yyparser.c \
   src/sregex/sre_regex_compiler.c \
   src/sregex/sre_vm_bytecode.c \
   src/sregex/sre_vm_thompson.c \
   src/sregex/sre_vm_pike.c \
   src/sregex/sre_capture.c

libsregex_la_CPPFLAGS = -I$(top_srcdir)/sregex/src
libsregex_la_CFLAGS = -O3 -std=gnu99 -Wall -Werror
