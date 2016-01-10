
bin_PROGRAMS = gabber

if BUILD_SSL
ssl_sources = SSLAdapter.hh SSLAdapter.cc
else
ssl_sources =
endif

# Keep foreign files first, then gabber files in alphabetical order

gabber_SOURCES = main.cc \
	eggtrayicon.h eggtrayicon.c \
	gtk2-funcs.h gtk2-funcs.c \
	gtkplugxembed.h gtkplugxembed.c \
	gtkspell.h gtkspell.c \
	gtkurl.h gtkurl.c \
	xtext.h xtext.c \
	AddContactDruid.hh AddContactDruid.cc \
	AgentInterface.hh AgentInterface.cc \
	AutoAway.hh AutoAway.cc \
	BaseGabberWindow.hh BaseGabberWindow.cc \
	ChatView.hh ChatView.cc \
	ConfigManager.hh ConfigManager.cc \
	ContactInfoInterface.hh ContactInfoInterface.cc \
	ContactInterface.hh ContactInterface.cc \
	DebugInterface.hh DebugInterface.cc \
	ErrorManager.hh ErrorManager.cc \
	EventManager.hh EventManager.cc \
	FilterInterface.hh FilterInterface.cc \
	FTInterface.hh FTInterface.cc \
	GabberApp.hh GabberApp.cc \
	GabberConfig.hh \
	GabberGPG.hh GabberGPG.cc \
	GabberLogger.hh GabberLogger.cc \
	GabberUtility.hh GabberUtility.cc \
	GabberWidgets.hh GabberWidgets.cc \
	GabberWin.hh GabberWin.cc \
	GCInterface.hh GCInterface.cc \
	GladeHelper.hh \
	GPGInterface.hh GPGInterface.cc \
	GroupsInterface.hh GroupsInterface.cc \
	IgnoreInterface.hh IgnoreInterface.cc \
	MessageManager.hh MessageManager.cc \
	MessageViews.hh MessageViews.cc \
	PrefsInterface.hh PrefsInterface.cc \
	RosterInterface.hh RosterInterface.cc \
	RosterView.hh RosterView.cc \
	S10nInterface.hh S10nInterface.cc \
	StatusInterface.hh StatusInterface.cc \
	TCPTransmitter.hh TCPTransmitter.cc \
	WelcomeDruid.hh WelcomeDruid.cc \
	$(ssl_sources)
          
gabber_LDADD = @EXTRA_GNOME_LIBS@ @XMMS_LIBS@ @SSL_LIBS@ @XSS_LIBS@ @GABBER_STATIC_LIBS@ \
	../jabberoo/src/libjabberoo.a \
	../jabberoo/libjudo/src/libjudo.la \
	-lxml

gabber_LDFLAGS = @GABBER_STATIC@

INCLUDES = -I$(top_srcdir) \
	-I$(top_srcdir)/jabberoo/libjudo/src/expat \
	-I$(top_srcdir)/jabberoo/libjudo/src \
	-I$(top_srcdir)/jabberoo/include \
	@EXTRA_GNOME_CFLAGS@ @XMMS_CFLAGS@ @SSL_CFLAGS@ \
	-DOPENSSL_NO_KRB5 \
	-DG_LOG_DOMAIN=\"Gabber\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DDATADIR=\"$(datadir)\" \
	-I$(top_srcdir)/intl

EXTRA_DIST = ghttp.c xembed.h GladeHelper.hh GabberConfig.hh SSLAdapter.hh SSLAdapter.cc

