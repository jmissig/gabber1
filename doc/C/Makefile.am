# To use this template:
#     1) Define: figs, docname, lang, omffile, entities although figs, 
#        omffile, and entities may be empty in your Makefile.am which 
#        will "include" this one 
#     2) Figures must go under figures/ and be in PNG format
#     3) You should only have one document per directory 
#
#        Note that this makefile forces the directory name under
#        $prefix/share/gnome/help/ to be the same as the XML filename
#        of the document.  This is required by GNOME. eg:
#        $prefix/share/gnome/help/fish_applet/C/fish_applet.xml
#                                 ^^^^^^^^^^^   ^^^^^^^^^^^
# Definitions:
#   figs         A list of screenshots which will be included in EXTRA_DIST
#                Note that these should reside in figures/ and should be .png
#                files, or you will have to make modifications below.
#   docname      This is the name of the XML file: <docname>.xml
#   lang         This is the document locale
#   omffile      This is the name of the OMF file.  Convention is to name
#                it <docname>-<locale>.omf.
#   entities     This is a list of XML entities which must be installed 
#                with the main XML file and included in EXTRA_DIST. 
# eg:
#   figs = \
#          figures/fig1.png            \
#          figures/fig2.png
#   docname = scrollkeeper-manual
#   lang = C
#   omffile=scrollkeeper-manual-C.omf
#   entities = fdl.xml
#   include $(top_srcdir)/help/xmldocs.make
#   dist-hook: app-dist-hook
#

figs = \
	figures/first.png \
	figures/login.png \
	figures/main.png \
	figures/msg-normal.png \
	figures/msg-ooo.png

docname = gabber

lang = C

omffile=gabber-C.omf

entities = \
	chapters/agents.xml \
	chapters/file.xml \
	chapters/first.xml \
	chapters/intro.xml \
	chapters/login.xml \
	chapters/main.xml \
	chapters/msg.xml \
	chapters/panel.xml \
	chapters/pref.xml \
	chapters/userinfo.xml \
	chapters/users.xml

include $(top_srcdir)/doc/xmldocs.make

dist-hook: app-dist-hook