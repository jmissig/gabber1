
noinst_LTLIBRARIES = libjudo.la

libjudo_la_SOURCES = judo.hpp \
                     judo.cpp \
                     Element.cpp \
                     ElementStream.cpp

libjudo_la_LIBADD = ./expat/libexpat.la

INCLUDES = -I./expat \
           -I.

if TEST
SUBDIRS = expat . test
else
SUBDIRS = expat . 
endif
