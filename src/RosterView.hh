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

#ifndef INCL_ROSTER_VIEW_HH
#define INCL_ROSTER_VIEW_HH

#include "jabberoo.hh"

#include <sigc++/object_slot.h>
#include <gtk--/ctree.h>
#include <gtk--/scrolledwindow.h>
#include <gtk--/tooltips.h>

typedef multimap<string, GtkCTreeNode*, jabberoo::JID::Compare> NodeMap;

class RosterView
     : public SigC::Object
{
public:
     enum Filter
     {
	  rvfAll, rvfAllNoAgents, rvfOnlineOnly, rvfOnlineOnlyNoAgents
     };

     RosterView(Gtk::CTree* tree, Gtk::ScrolledWindow* tscroll, jabberoo::Session& session);
     void clear();
     void refresh(bool reload_cache = false);
     void disable_presence_sounds(int millisecs);
     gint enable_presence_sounds();
     void save_collapsed_groups();
     void load_collapsed_groups();
     // Menu handlers
     void on_Message_activate();
     void on_OOOChat_activate();
     void on_SendRoster_activate();
     void on_SendFile_activate();
     void on_S10nRequest_activate();
     void on_EditUser_activate();
     void on_EditGroups_activate();
     void on_History_activate();
     void on_DeleteUser_activate();
     void on_LoginTransport_activate();
     void on_LogoutTransport_activate();
     void on_TransInfo_activate();
     void on_AddUser_activate();
     // Filter
     void set_view_filter(Filter f);
     // Not In List Item
     void addNILItem(jabberoo::Roster::Item& r);
     // clear_event will remove stale NIL items
     void clear_event(const string& jid);
     // Temp group
     void add_temp_group(string& groupname);
protected:
     gint on_refresh();
     void on_update_refresh();
     void on_presence(const string& jid, bool available, jabberoo::Presence::Type prev_type);
     // Tree event handlers
     int  on_button_press(GdkEventButton* e);
     int  on_key_press(GdkEventKey* e);
     void on_roster_row_activate(const string& jid, bool control_pressed);
     void on_tree_expand(Gtk::CTree::Row r);
     void on_tree_collapse(Gtk::CTree::Row r);
     // tree_move handler, Gtk-- doesn't support this yet so we do it through gtk directly
     static void on_tree_move(GtkCTree *ctree, GtkCTreeNode *node, GtkCTreeNode *new_parent, GtkCTreeNode *new_sibling, gpointer data);
     // DnD callbacks
     void on_drag_data_get(GdkDragContext* drag_ctx, GtkSelectionData* data, guint info, guint time);
     void on_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time);
     void dnd_get_vCard(const Element& iq);
     static gboolean is_drop_ok(GtkCTree* ctree, GtkCTreeNode* node, GtkCTreeNode* new_parent, GtkCTreeNode* new_sibling);
     void on_drag_begin(GdkDragContext* drag_ctx);
     // Dialog handlers
     void handle_RemoveUserDlg(int code, string jid, string nickname);
     void handle_RemoveAgentDlg(int code, string jid);
     void handle_RemoveAgent_iq(const judo::Element& iq);
     // Rendering helpers
     GtkCTreeNode* get_group_node(const string& groupid, bool available);
     void          render_group_nodes();
     GtkCTreeNode* create_item_node(GtkCTreeNode* groupnode, const jabberoo::Roster::Item& r);
     // Utility
     GdkColor get_style_bg_color();
     GdkColor get_style_fg_color();
     gint FlashEvents();
     Gtk::CTree::Row* findRow(const string& jid);
private:
     set<string>            _collapsed_groups;
     map<string, Gdk_Color> _status_colors;
     list<jabberoo::Roster::Item> _nil_roster;
     list<string>	    _temp_groups;
     bool               _event_flash;
     string             _online, _offline, _pix_path;
     Filter             _filter;
     jabberoo::Session& _session;
     Gtk::CTree*        _tree;
     Gtk::ScrolledWindow* _tscroll;
     int                _row, _col;
     int                _total_online;
     Gtk::Tooltips      _tips;
     // dnd selection info for when we need to request vcard from server
     GtkSelectionData*  _dnd_data;
     // Timer for roster refresh
     SigC::Connection   _refresh_timer;
     // Timer for disabling presence sounds
     SigC::Connection   _presence_sounds_timer;
     // Constants
     static const int colNickname  = 0;
     static const int colJID       = 1;
};

class SimpleRosterView
     : public SigC::Object
{
public:
     SimpleRosterView(Gtk::CTree* tree, Gtk::ScrolledWindow* tscroll, jabberoo::Roster::ItemMap& roster, bool usegroups = true, string defaultgroup = "Unfiled");
     virtual ~SimpleRosterView();

     void refresh();
     void clear();
     void show_jid();
     void ignore_nick();

     Signal1<void, jabberoo::Roster::Item&> evtAddUser;

     // These aren't constants anymore since if jids are being shown then the ctree
     // is 3 columns with the 3rd being the UTF8 jid
     int _colNickname;
     int _colJID;

     // Public way to simply push drag data... 
     // possibly dragged to some other widget, but we want to push it here
     void push_drag_data(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time);
     void add_jid(const string& jid, const string& nickname);
protected:
     GtkCTreeNode* get_group_node(const string& groupid);
     GtkCTreeNode* create_item_node(GtkCTreeNode* groupnode, const jabberoo::Roster::Item& r);
     GdkColor get_style_bg_color(void);
     GdkColor get_style_fg_color(void);

     // tree_move handler, Gtk-- doesn't support this yet so we do it through gtk directly
     static void on_tree_move(GtkCTree *ctree, GtkCTreeNode *node, GtkCTreeNode *new_parent, GtkCTreeNode *new_sibling, gpointer data);
     // DnD callbacks
     void on_drag_data_get(GdkDragContext* drag_ctx, GtkSelectionData* data, guint info, guint time);
     void on_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time);
     void dnd_get_vCard(const Element& iq);
     gboolean on_drag_data_motion(GdkDragContext* drag_ctx, gint x, gint y, guint time);
     void on_drag_begin(GdkDragContext* drag_ctx);

private:
     Gtk::CTree*		   _tree;
     Gtk::ScrolledWindow*          _tscroll;
     jabberoo::Roster::ItemMap&	   _roster;
     // dnd selection info for when we need to request vcard from server
     GtkSelectionData*		   _dnd_data;
     NodeMap			   _GROUPS;
     // Default group items are put in
     string			   _defaultgroup;
     // Whether or not groups should be used
     bool			   _usegroups;
     bool			   _usenick;
     bool			   _showjid;
};

#endif
