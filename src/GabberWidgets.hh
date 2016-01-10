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

#ifndef INCL_GABBER_WIDGETS_HH
#define INCL_GABBER_WIDGETS_HH

#include "jabberoo.hh"

#include <gdk/gdk.h>
#include <gtk/gtkselection.h>
#include <gtk--/box.h>
#include <gtk--/combo.h>
#include <gtk--/eventbox.h>
#include <gtk--/label.h>
#include <gtk--/tooltips.h>
#include <gnome--/pixmap.h>

class PrettyJID
     : public Gtk::EventBox
{
public:
     enum DisplayType 
     { dtNick, dtNickRes, dtJID, dtJIDRes, dtNickJID, dtNickJIDRes };

     PrettyJID(const string& jid, const string& nickname = "", DisplayType dt = dtJID, string::size_type display_limit = 0, bool select_resource = false, bool select_jid = false);

     void set_display_type(DisplayType dt, string::size_type display_limit = 0);
     void show_label(bool show_label);
     void show_pixmap(bool show_pixmap);
     void hide_resource_select();
     const string get_nickname() const { return _nickname; }
     const string get_full_jid() const { return _default_jid; }
     const string get_jid() const { return _jid; }
     bool is_on_roster() const;
protected:
     bool is_displaying_jid() const;
     void on_presence(const jabberoo::Presence& p, const jabberoo::Presence::Type prev);
     void on_entJID_changed();
     void on_entResource_changed();
     void on_evtShow_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time);
     void on_evtShow_drag_data_get(GdkDragContext* drag_ctx, GtkSelectionData* data, guint info, guint time);
     void on_evtShow_drag_begin(GdkDragContext* drag_ctx);
private:
     string _jid;
     string _nickname;
     string _resource;
     string _default_jid;
     bool   _select_resource;
     bool   _select_jid;
     string _pix_path;
     bool   _onRoster;

     Gtk::HBox*     _hboxPJ;
     Gtk::EventBox* _evtMusic;
     Gnome::Pixmap* _pixMusic;
     Gtk::Entry*    _entJID;
     Gtk::Combo*    _cboResource;
     Gtk::Label*    _lblResource;
     Gtk::Label*    _lblPJ;
     Gnome::Pixmap* _pixPJ;
     Gtk::Tooltips  _tips;

     DisplayType _display_type;
public:
     SigC::Signal0<void> changed;
};

#endif
