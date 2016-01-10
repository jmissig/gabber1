SUBDIRS = po jabberoo src ui pixmaps sounds doc omf-install

EXTRA_DIST = \
	AUTHORS		\
	COPYING		\
	COPYING.exception \
	ChangeLog	\
	INSTALL		\
	NEWS		\
	README		\
	README.FreeBSD 	\
	README.proxy	\
	TODO		\
	gabber.1	\
	gabber.spec.in	\
	gabber.desktop.in 	\
	gabber.soundlist.in \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

dist-hook: gabber.spec
	cp gabber.spec $(distdir)

Applicationsdir = $(datadir)/gnome/apps/Internet
Applications_in_files = gabber.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
Soundlistdir = $(sysconfdir)/sound/events
Soundlist_in_files = gabber.soundlist.in
Soundlist_DATA = $(Soundlist_in_files:.soundlist.in=.soundlist)
@INTLTOOL_SOUNDLIST_RULE@
man_MANS = gabber.1
