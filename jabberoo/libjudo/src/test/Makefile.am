SUBDIRS = .

EXTRA_DIST = judo_testall.sh

noinst_PROGRAMS = judo_test

judo_test_SOURCES = judo_test.hpp \
                    judo_test.cpp \
                    GlobalsTest.cpp \
                    CDATATest.cpp \
                    ElementTest.cpp  \
                    ElementStreamTest.cpp

judo_test_LDADD = $(top_srcdir)/libcppunit/src/libcppunit.la \
                     ../libjudo.la

INCLUDES = -I$(top_srcdir)/libcppunit/src \
           -I$(top_srcdir)/libcppunit/src/extensions \
           -I.. \
           -I../expat \
           -I.

