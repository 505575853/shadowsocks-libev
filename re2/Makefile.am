noinst_LTLIBRARIES = libre2.la
libre2_la_SOURCES = \
    re2/bitstate.cc \
    re2/compile.cc \
    re2/dfa.cc \
    re2/filtered_re2.cc \
    re2/mimics_pcre.cc \
    re2/nfa.cc \
    re2/onepass.cc \
    re2/parse.cc \
    re2/perl_groups.cc \
    re2/prefilter.cc \
    re2/prefilter_tree.cc \
    re2/prog.cc \
    re2/re2.cc \
    re2/regexp.cc \
    re2/set.cc \
    re2/simplify.cc \
    re2/stringpiece.cc \
    re2/tostring.cc \
    re2/unicode_casefold.cc \
    re2/unicode_groups.cc \
    util/rune.cc \
    util/strutil.cc \
    cre2.cpp

libre2_la_CPPFLAGS = -I$(top_srcdir)/re2
libre2_la_CXXFLAGS = -O3 -std=gnu++11 -Wall -Werror \
    -Wno-missing-field-initializers \
    -Wno-sign-compare \
    -Wno-tautological-undefined-compare \
    -Wno-unused-local-typedef \
    -Wno-unused-parameter
