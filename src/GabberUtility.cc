/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 *  Gabber
 *  Copyright (C) 1999-2002 Dave Smith & Julian Missig
 */

#include "GabberConfig.hh" // for _()

#include "GabberUtility.hh"

#include "ConfigManager.hh"
#include "GabberWin.hh"
#include "StatusInterface.hh"

#include <fstream>
#include <libgnome/gnome-i18n.h>
#include <gnome--/dialog.h>
#include <gal/widgets/e-unicode.h>

using namespace jabberoo;
using namespace GabberUtil;

void GabberUtil::main_dialog(Gnome::Dialog* d)
{
     // If the main window is hidden,
     // child dialogs will appear off screen,
     // so we have to have all possible parents here
     if (G_Win && G_Win->getBaseWindow()->is_visible())
	  d->set_parent(* (G_Win->getBaseWindow()) );
     else if (StatusIndicator::get_dock_window())
	  d->set_parent(* (StatusIndicator::get_dock_window()) );
}

gint GabberUtil::update_progress_bar(Gtk::ProgressBar* bar)
{
     // Grab the current %, increment
     float pvalue = bar->get_current_percentage();
     pvalue += 0.01;

     // If it's going to hit an end, reverse it, so it goes back and forth
     if (pvalue <= 0.01)
     {
	  bar->set_orientation(GTK_PROGRESS_LEFT_TO_RIGHT);
	  pvalue = 0.01;
     }
     else if (pvalue >= 0.99)
     {
	  bar->set_orientation(GTK_PROGRESS_RIGHT_TO_LEFT);
	  pvalue = 0.01;
     }

     // Actually set the percentage
     bar->set_percentage(pvalue);

     return TRUE;
}

void GabberUtil::toggle_visible(Gtk::ToggleButton* t, Gtk::Widget* w)
{
     if (t->get_active())
	  w->show();
     else
	  w->hide();
}

gint GabberUtil::strcasecmp_clist_items(GtkCList* c, const void* lhs, const void* rhs)
{
     const char* lhs_txt = GTK_CELL_PIXTEXT(((GtkCListRow*)lhs)->cell[c->sort_column])->text;
     const char* rhs_txt = GTK_CELL_PIXTEXT(((GtkCListRow*)rhs)->cell[c->sort_column])->text;
     return  strcasecmp(lhs_txt, rhs_txt);
}

gint GabberUtil::strcmp_clist_items(GtkCList* c, const void* lhs, const void* rhs)
{
     string lhs_txt = GTK_CELL_PIXTEXT(((GtkCListRow*)lhs)->cell[c->sort_column])->text;
     string rhs_txt = GTK_CELL_PIXTEXT(((GtkCListRow*)rhs)->cell[c->sort_column])->text;
     return lhs_txt.compare(rhs_txt);
}

string GabberUtil::substitute(const string& full_string, const string& var1)
{
     gchar* subs_gchr = g_strdup_printf(full_string.c_str(), var1.c_str());
     string subs_str = subs_gchr;
     g_free(subs_gchr);
     return subs_str;
}

string GabberUtil::substitute(const string& full_string, const string& var1, const string& var2)
{
     gchar* subs_gchr = g_strdup_printf(full_string.c_str(), var1.c_str(), var2.c_str());
     string subs_str = subs_gchr;
     g_free(subs_gchr);
     return subs_str;
}

void GabberUtil::change_pixmap(Gtk::PixmapMenuItem* pitem, const string& filename)
{
     // The number of incomplete widgets in gnome-libs is starting to scare me
     // This function claws inside the guts of the widget
     // I found something like this inside bonobo
     Gtk::Pixmap* pitem_pixmap = manage(new Gtk::Pixmap(filename));
     GtkPixmapMenuItem* gack = pitem->gtkobj();
     if (gack->pixmap)
     {
	  gtk_widget_destroy(gack->pixmap);
	  gack->pixmap = NULL;
     }
     pitem_pixmap->show();
     pitem->set_pixmap(*pitem_pixmap);
}

string GabberUtil::toUTF8(const string& text)
{
     if (text.empty())
	  return "";
     else
     {
	 gchar*  value = e_utf8_from_locale_string(text.c_str());
	 string result = value;
	 g_free(value);
	 return result;
     }
}

string GabberUtil::toUTF8(Gtk::Widget* w, const string& text)
{
     return toUTF8(text);
}

string GabberUtil::toUTF8(GtkWidget* w, const string& text)
{
     return toUTF8(text);
}

string GabberUtil::fromUTF8(const string& text)
{
     if (text.empty())
	  return "";
     else
     {
	 gchar* value = e_utf8_to_locale_string(text.c_str());
	 string result = value;
	 g_free(value);
	 return result;
     }
}

string GabberUtil::fromUTF8(Gtk::Widget* w, const string& text)
{
     return fromUTF8(text);
}

string GabberUtil::fromUTF8(GtkWidget* w, const string& text)
{
     return fromUTF8(text);
}


string GabberUtil::getShowName(Presence::Show type)
{
     switch (type)
     {
     case Presence::stInvalid:
	  return _("Invalid");
     case Presence::stOffline:
	  return _("Offline");
     case Presence::stOnline:
	  return _("Available");
     case Presence::stChat:
	  return _("Free to Chat");
     case Presence::stAway:
	  return _("Away");
     case Presence::stXA:
	  return _("Extended Away");
     case Presence::stDND:
	  return _("Busy");
     }
     return "";
}

string GabberUtil::getShowFilename(Presence::Show type)
{
     switch(type)
     {
     case Presence::stInvalid:
	  return string("invalid");
     case Presence::stOnline:
	  return string("online");
     case Presence::stOffline:
	  return string("offline");
     case Presence::stChat:
	  return string("chat");
     case Presence::stAway:
	  return string("away");
     case Presence::stXA:
	  return string("xa");
     case Presence::stDND:
	  return string("dnd");
     }
     return string("");
}

string GabberUtil::getS10nName(Roster::Subscription type)
{

     switch (type)
     {
     case Roster::rsBoth:
	  return _("both");
     case Roster::rsFrom:
	  return _("from");
     case Roster::rsTo:
	  return _("to");
     case Roster::rsNone:
     default:
	  return _("none");
     }
}

string GabberUtil::getS10nInfo(Roster::Subscription type)
{
     switch (type)
     {
     case Roster::rsBoth:
	  return _("Both of you can see each other's presence.");
     case Roster::rsFrom:
	  return _("You cannot see their presence, but they can see yours.");
     case Roster::rsTo:
	  return _("You can see their presence, but they cannot see yours.");
	  break;
     case Roster::rsNone:
     default:
	  return _("Neither of you can see the other's presence.");
     }
}

// We have these indexShow functions in case the enum changes
Presence::Show GabberUtil::indexShow(int show_index)
{
     switch (show_index)
     {
     case 0:                 // Invalid
	  return Presence::stInvalid;
     case 1:                 // Offline
	  return Presence::stOffline;
     case 2:                 // Online
	  return Presence::stOnline;
     case 3:		     // Chat
	  return Presence::stChat;
     case 4:		     // Away
	  return Presence::stAway;
     case 5:		     // Not available
	  return Presence::stXA;
     case 6:		     // Do not disturb
	  return Presence::stDND;
     default:
	  return Presence::stInvalid;
     }
}

int GabberUtil::indexShow(Presence::Show show_index)
{
     switch (show_index)
     {
     case Presence::stInvalid:
	  return 0;                 // Invalid
     case Presence::stOffline:
	  return 1;                 // Offline
     case Presence::stOnline:
	  return 2;                 // Online
     case Presence::stChat:
	  return 3;		     // Chat
     case Presence::stAway:
	  return 4;		     // Away
     case Presence::stXA:
	  return 5;		     // Not available
     case Presence::stDND:
	  return 6;		     // Do not disturb
     default:
	  return 0;
     }
}

Presence::Type GabberUtil::indexType(int type_index)
{
     switch (type_index)
     {
     case 0:                 // Error
	  return Presence::ptError;
     case 1:                 // Unavailable
	  return Presence::ptUnavailable;
     case 2:                 // Available
	  return Presence::ptAvailable;
     case 3:                 // Invisible
	  return Presence::ptInvisible;
     default:
	  return Presence::ptError;
     }
}

int GabberUtil::indexType(Presence::Type type_index)
{
     switch (type_index)
     {
     case Presence::ptError:
	  return 0;                 // Error
     case Presence::ptUnavailable:
	  return 1;                 // Unavailable
     case Presence::ptAvailable:
	  return 2;                 // Available
     case Presence::ptInvisible:
	  return 3;                 // Invisible
     default:
	  return 0;
     }
}

ConfigManager::Proxy GabberUtil::indexProxy(int proxy_index)
{
     switch (proxy_index)
     {
     case 0:
	  return ConfigManager::proxyNone;
     case 1:
	  return ConfigManager::proxySOCKS4;
     case 2:
	  return ConfigManager::proxySOCKS5;
     case 3:
	  return ConfigManager::proxyHTTP;
     default:
	  return ConfigManager::proxyNone;
     }
}

int GabberUtil::indexProxy(ConfigManager::Proxy proxy_index)
{
     switch (proxy_index)
     {
     case ConfigManager::proxyNone:
	  return 0;
     case ConfigManager::proxySOCKS4:
	  return 1;
     case ConfigManager::proxySOCKS5:
	  return 2;
     case ConfigManager::proxyHTTP:
	  return 3;
     default:	  return 0;
     }
}


// Attach some DnD stuff to Gtk::Text
GabberDnDText::GabberDnDText(Gtk::Text* widget)
    : _textWidget(widget)
{
     // Setup DnD targets that we can receive
     GtkTargetEntry dnd_dest_targets[] = {
//	  {"text/x-jabber-roster-item", 0, 0},
	  {"text/x-jabber-id", 0, 0},
     };
     int dest_num = sizeof(dnd_dest_targets) / sizeof(GtkTargetEntry);
     gtk_drag_dest_unset(GTK_WIDGET(_textWidget->gtkobj()));
     gtk_drag_dest_set(GTK_WIDGET(_textWidget->gtkobj()), (GtkDestDefaults) (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP), dnd_dest_targets, dest_num, (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE));

     _textWidget->drag_data_received.connect(slot(this, &GabberDnDText::on_drag_data_received));
}

void GabberDnDText::on_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time)
{
     if (data->target == gdk_atom_intern("text/x-jabber-id", FALSE))
     {
	  string jids;
	  for (char *line = strtok((char *) data->data, "\n"); line; line = strtok(NULL, "\n"))
	  {
	       char *p = strchr((char *) line, ':');
	       if (!p)
	       {
		    cerr << "Got invalid DnD data" << endl;
		    return;
	       }
	       *p++ = '\0';
	       if (g_strcasecmp((gchar *) line, "jabber") != 0)
	       {
		    cerr << "Got invalid DnD data" << endl;
		    return;
	       }
	       string jid(p);

	       if (!jids.empty())
		    jids += ", ";
	       jids += "jabber:" + jid;
	  }
	  _textWidget->insert(fromUTF8(_textWidget, jids) + " ");
     }
}


// Menu Builder - easy way to create an option menu with signals
/*
 *  The code in this class is
 *  Copyright (c) 2000 ERDI Gergo
 *  Also under the GPL
 */

MenuBuilder::MenuBuilder()
     : menu(manage(new Gtk::Menu()))
{
}

void MenuBuilder::add_tearoff()
{
     Gtk::TearoffMenuItem* tmi = manage(new Gtk::TearoffMenuItem());
     menu->add(*tmi);
     tmi->show();
}

void MenuBuilder::add_separator()
{
     Gtk::MenuItem* item = manage(new Gtk::MenuItem());

     // Add to menu
     menu->add(*item);
     item->show();
}

Gtk::MenuItem* MenuBuilder::add_item(const string& label, int num, bool submenu)
{
     // Ugh. Ok, so here's the breakdown:
     // label - the label to appear on the menu item
     // num - the number to send through the selected signal, usually this is the placement in the menu
     // submenu - whether or not this item will have a submenu; nasty things happen if you connect activate then

     // Create the menuitem, which has a nice constructor
     Gtk::MenuItem* item = manage(new Gtk::MenuItem(label));
     if (!submenu) // If this is not an item which will have a submenu
	  item->activate.connect(SigC::bind(selected.slot(), num));

     // Add to menu
     menu->add(*item);
     item->show();

     return item;
}

Gtk::PixmapMenuItem* MenuBuilder::add_item(const string& pixmap, const string& label, int num, bool submenu)
{
     // Ugh. Ok, so here's the breakdown:
     // pixmap - the filename of the pixmap we want to appear
     // label - the label to appear on the menu item
     // num - the number to send through the selected signal, usually this is the placement in the menu
     // submenu - whether or not this item will have a submenu; nasty things happen if you connect activate then

     // Create a pixmapmenuitem, pixmap, and label
     Gtk::PixmapMenuItem* pitem = manage(new Gtk::PixmapMenuItem());
     Gtk::Label* pitem_label = manage(new Gtk::Label(label, 0.0, 0.5));
     Gtk::Pixmap* pitem_pixmap = manage(new Gtk::Pixmap(pixmap));

     // Set the pixmap
     pitem->set_pixmap(*pitem_pixmap);
     pitem_pixmap->show();
     // Add the label
     pitem->add(*pitem_label);
     pitem_label->show();

     // Connect it
     if (!submenu) // If this is not an item which will have a submenu
	  pitem->activate.connect(SigC::bind(selected.slot(), num));

     // Add to menu
     menu->add(*pitem);
     pitem->show();

     return pitem;
}

void MenuBuilder::finish_items()
{
     menu->show();
     menu->show_all();
}

PresenceMenuBuilder::PresenceMenuBuilder()
     : _menu(_main.get_menu())
{
     // Get the path to the pixmaps
     string pix_path = ConfigManager::get_PIXPATH();

     // Add the menu items
//     _main.add_tearoff();
     Gtk::PixmapMenuItem* pi;
     pi = _main.add_item(pix_path + "online.xpm", getShowName(Presence::stOnline), indexShow(Presence::stOnline), false);
     pi = _main.add_item(pix_path + "chat.xpm", getShowName(Presence::stChat), indexShow(Presence::stChat), false);
     pi = _main.add_item(pix_path + "away.xpm", getShowName(Presence::stAway), indexShow(Presence::stAway), false);
     pi = _main.add_item(pix_path + "xa.xpm", getShowName(Presence::stXA), indexShow(Presence::stXA), false);
     pi = _main.add_item(pix_path + "dnd.xpm", getShowName(Presence::stDND), indexShow(Presence::stDND), false);
     pi = _main.add_item(pix_path + "invisible.xpm", _("Invisible"), -2, false); // -2 because this is not a show type
//     pi = _main.add_item(_("Set Status..."), -1, false) // -1 because this is not a show type

     _main.finish_items();
     _main.selected.connect(main_selected.slot());
}

// File Selection and saving.
// This assumes you have something you want to save to some file

FileSel::FileSel(const string& filename, const string& title, const string& log)
     : _log(log)
{
     // Create and connect
     _fileSel = manage(new Gtk::FileSelection(title));
     _fileSel->get_ok_button()->clicked.connect(slot(this, &FileSel::on_ok_clicked));
     _fileSel->get_cancel_button()->clicked.connect(slot(this, &FileSel::on_cancel_clicked));
     
     // Get the JID and replace the @ w/ an underscore
//     string basejid = JID::getUserHost(jid);
//     basejid = basejid.replace(basejid.find("@"), 1, "_");
//     basejid += ".log";
     
     // Setup
     _fileSel->set_filename(filename);
     
     // Display
     _fileSel->show();
}

void FileSel::on_ok_clicked()
{
      string filename = _fileSel->get_filename();
     
      // Save the file
      ofstream file(filename.c_str());
      if (!file)
      {
	   Gnome::Dialog* d = manage(Gnome::Dialogs::warning(_("Unable to save file. Please try another name or location.")));
	   d->set_modal(true);
      }
      else
      {
	   file << _log.c_str() << endl;
	   // Get rid of file selection dialog
	   _fileSel->hide();
	   _fileSel->destroy();
      }
}

void FileSel::on_cancel_clicked()
{
     // Get rid of file selection dialog
     _fileSel->hide();
     _fileSel->destroy();
}

