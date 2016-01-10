
shared_pixmaps = away.xpm		\
		chat.xpm		\
		dnd.xpm 		\
		invisible.xpm		\
		offline.xpm		\
		online.xpm		\
		stalker.xpm		\
		xa.xpm 			\
		gnome-gabber.xpm 	\
		gnome-message.xpm 	\
		gnome-ooochat.xpm	\
		gnome-groupchat.xpm	\
		gnome-s10n.xpm 		\
		gnome-editgroups.xpm	\
		gnome-userinfo.xpm	\
		gnome-gpg.xpm		\
		gabber-about.png	\
		xmms.xpm		\
		xmms_paused.xpm		\
		xmms_stopped.xpm

glade_pixmaps = glade-gabber-logo.png 	\
		glade-message.xpm 	\
		glade-ooochat.xpm 	\
		glade-blank.xpm		\
		glade-s10n.xpm 		\
		glade-new-user.xpm 	\
		glade-new-user-menu.xpm	\
		glade-add-group-menu.xpm \
		glade-edit-user.xpm	\
		glade-edit-user-menu.xpm \
		glade-edit-group-menu.xpm \
		glade-delete-user.xpm	\
		glade-delete-user-menu.xpm \
		glade-groupchat.xpm	\
		glade-groupchat-menu.xpm \
		glade-users.xpm		\
		glade-sendasooo.xpm	\
		glade-sendasnormal.xpm  \
		glade-noclose.xpm	\
		glade-gabber-sideways.png \
		gpg-encrypted.xpm	\
		gpg-unencrypted.xpm	\
		gpg-signed.xpm		\
		gpg-badsigned.xpm	\
		gpg-unsigned.xpm	\
		glade-alert.xpm		\
		glade-unlock-key.xpm	\
		glade-send-contact-menu.xpm \
		glade-send-file-menu.xpm \
		glade-send-file.xpm	\
		glade-receive-file.xpm 	\
		glade-send-gci.xpm	\
		glade-contact.xpm	\
		glade-file.xpm		\
		glade-gci.xpm		\
		glade-group.xpm		\
		offline.xpm		\
		connected.xpm		\
		connected-ssl.xpm	\
		disconnected.xpm

EXTRA_DIST = gabber.png 		\
	$(shared_pixmaps)		\
	$(glade_pixmaps)

logodir = $(datadir)/pixmaps
logo_DATA = gabber.png

pixmapdir = $(datadir)/pixmaps/gabber
pixmap_DATA = $(shared_pixmaps)

gladedir = $(datadir)/gabber
glade_DATA = $(glade_pixmaps)

install-exec-hook: 
	(cd $(DESTDIR)$(datadir)/gabber;	\
	ln -sf ../pixmaps/gnome-info.png gnome-info.png;		\
	ln -sf ../pixmaps/gnome-lockscreen.png gnome-lockscreen.png;	\
	ln -sf ../pixmaps/gnome-globe.png gnome-globe.png;		\
	ln -sf ../pixmaps/gnome-modem.png gnome-modem.png;		\
	ln -sf ../pixmaps/gnome-question.png gnome-question.png)
