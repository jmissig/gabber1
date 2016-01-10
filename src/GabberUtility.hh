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
 *  Copyright (C) 1999-2001 Dave Smith & Julian Missig
 */

#ifndef INCL_GABBER_UTILITY_HH
#define INCL_GABBER_UTILITY_HH

#include "jabberoo.hh"

#include "ConfigManager.hh"

#include <gdk/gdk.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkselection.h>
#include <sigc++/object_slot.h>
#include <sigc++/signal_system.h>
#include <gtk--/fileselection.h>
#include <gtk--/progressbar.h>
#include <gtk--/togglebutton.h>
#include <gtk--/menu.h>
#include <gtk--/text.h>
#include <gnome--/dialog.h>
#include <gnome--/pixmapmenuitem.h>

namespace GabberUtil
{
     void main_dialog(Gnome::Dialog* d);
     gint update_progress_bar(Gtk::ProgressBar* bar);
     void toggle_visible(Gtk::ToggleButton* t, Gtk::Widget* w);
     gint strcasecmp_clist_items(GtkCList* c, const void* lhs, const void* rhs);
     gint strcmp_clist_items(GtkCList* c, const void* lhs, const void* rhs);
     string substitute(const string& full_string, const string& var1);
     string substitute(const string& full_string, const string& var1, const string& var2);
     void change_pixmap(Gtk::PixmapMenuItem* pitem, const string& filename);
     // We still have the widget parameter because we may move back to needing that at some point
     string toUTF8(const string& text);
     string toUTF8(Gtk::Widget* w, const string& text);
     string toUTF8(GtkWidget* w, const string& text);
     string fromUTF8(const string& text);
     string fromUTF8(Gtk::Widget* w, const string& text);
     string fromUTF8(GtkWidget* w, const string& text);
     string getShowName(jabberoo::Presence::Show type);
     string getShowFilename(jabberoo::Presence::Show type);
     string getS10nName(jabberoo::Roster::Subscription type);
     string getS10nInfo(jabberoo::Roster::Subscription type);
     jabberoo::Presence::Show indexShow(int show_index);
     int    indexShow(jabberoo::Presence::Show show_index);
     jabberoo::Presence::Type indexType(int type_index);
     int    indexType(jabberoo::Presence::Type type_index);
     ConfigManager::Proxy indexProxy(int proxy_index);
     int    indexProxy(ConfigManager::Proxy proxy_index);
};

class GabberDnDText
     : public SigC::Object
{
public:
     GabberDnDText(Gtk::Text* widget);
protected:
     void on_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time);
private:
     Gtk::Text* _textWidget;
};

class MenuBuilder
     : public SigC::Object
{
public:
     MenuBuilder();
     Gtk::Menu* get_menu() { return menu; }
     void add_tearoff();
     void add_separator();
     Gtk::MenuItem* add_item(const string& label, int num, bool submenu = false);
     Gtk::PixmapMenuItem* add_item(const string& pixmap, const string& label, int num, bool submenu = false);
     void add_presence_items();
     void finish_items();
private:
    Gtk::Menu* menu;
public:
     SigC::Signal1<void, int> selected;
};

class PresenceMenuBuilder
     : public SigC::Object
{
public:
     PresenceMenuBuilder();
     Gtk::Menu* get_menu() { return _menu; }
private:
     MenuBuilder _main;
     Gtk::Menu* _menu;
     MenuBuilder _online;
     MenuBuilder _chat;
     MenuBuilder _away;
     MenuBuilder _xa;
     MenuBuilder _dnd;
public:
     SigC::Signal1<void, int> main_selected;
//     SigC::Signal2<void, int, jabberoo::Presence::Show> presence_selected;
};

class FileSel
     : public SigC::Object
{
public:
     FileSel::FileSel(const string& filename, const string& title, const string& log);
protected:
     void FileSel::on_ok_clicked();
     void FileSel::on_cancel_clicked();
private:
     const string& _log;
     Gtk::FileSelection* _fileSel;
};

#endif
