if SIGC_INSTALLED
sigc_cflags = @SIGC_CFLAGS@
sigc_libs = @SIGC_LIBS@
sigc_a_libs = @SIGC_A_LIBS@
else
sigc_cflags = -I$(top_srcdir)/libsigc++ -I$(top_srcdir)/libsigc++/sigc++/config
sigc_libs = 
sigc_a_libs = ../libsigc++/sigc++/libsigc.a
endif

noinst_LIBRARIES = libjabberoo.a
libjabberoo_a_SOURCES = jutil.cc \
                        jabberoo-packet.cc \
                        jabberoo-session.cc \
                        jabberoo-message.cc \
                        jabberoo-presence.cc \
                        jabberoo-presencedb.cc \
                        jabberoo-roster.cc \
                        jabberoo.cc \
                        jabberoox-filter.cc \
                        jabberoox-agent.cc \
                        sha.c
libjabberoo_a_LIBADD = ../libjudo/src/libjudo.la 
libjabberoo_a_LIBS = $(sigc_a_libs)
INCLUDES = -I$(top_srcdir)/libjudo/src/expat \
	   -I$(top_srcdir)/libjudo/src \
           -I$(top_srcdir)/include $(sigc_cflags)

jabberoo_sources = jutil.cc \
	jabberoo-packet.cc \
	jabberoo-session.cc \
	jabberoo-message.cc \
	jabberoo-presence.cc \
	jabberoo-presencedb.cc \
	jabberoo-roster.cc \
	jabberoo.cc \
	jabberoox-filter.cc \
	jabberoox-agent.cc \
	sha.c
