sigc_cflags = @SIGC_CFLAGS@
sigc_libs = @SIGC_LIBS@
sigc_a_libs = @SIGC_A_LIBS@

noinst_PROGRAMS = jidtest itertest filtertest

jidtest_LDADD =  ../src/libjabberoo.a ../libjudo/src/libjudo.la $(sigc_a_libs)
jidtest_LDFLAGS = @JABBEROO_STATIC@
filtertest_LDADD = ../src/libjabberoo.a ../libjudo/src/libjudo.la $(sigc_a_libs)
filtertest_LDFLAGS = @JABBEROO_STATIC@
itertest_LDADD =  ../src/libjabberoo.a ../libjudo/src/libjudo.la $(sigc_a_libs)
itertest_LDFLAGS = @JABBEROO_STATIC@
INCLUDES = -I$(top_srcdir)/libjudo/src/expat -I$(top_srcdir)/libjudo/src -I$(top_srcdir)/include $(sigc_cflags)
LIBS = $(sigc_libs)

jidtest_SOURCES = jidtest.cc
itertest_SOURCES = itertest.cc
filtertest_SOURCES = filtertest.cc
