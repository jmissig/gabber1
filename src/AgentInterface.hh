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
#ifndef INCL_AGENT_BROWSER
#define INCL_AGENT_BROWSER

#include "jabberoo.hh"
#include "jabberoox.hh"

#include "BaseGabberWindow.hh"

#include <gnome--/appbar.h>
#include <gnome--/druid.h>

using namespace std;

class AgentBrowser
     : public SigC::Object
{
public:
     enum Filter
     {
	  agAll, agRegisterable, agSearchable, agGCCapable, agAgents, agTransport
     };

     AgentBrowser(BaseGabberWindow* base, const string& base_string);
     ~AgentBrowser();
     void update_agents(const string& server, const Element& query);
     // Filter
     void set_view_filter(Filter f);
     // Actions
     void clear();
     void browse();
     // Accessors
     jabberoo::Agent* get_current_agent() { return _current_agent; }
protected:
     void on_browse_clicked();
     gint on_key_press_event(GdkEventKey* e);
     void on_tree_expand(Gtk::CTree::Row r);
     void on_tree_collapse(Gtk::CTree::Row r);
     int  on_tree_button(GdkEventButton* e);
     void on_tree_select(Gtk::CTree::Row r, int col);
     void on_tree_unselect(Gtk::CTree::Row r, int col);
     void on_agent_fetch_complete(bool successful, GtkCTreeNode* agentnode, GtkCTreeNode* dummynode);
     void handle_agents_IQ(const Element& iq);
     void refresh();
     // Agent render
     void render_agent(jabberoo::Agent& a, GtkCTreeNode* parent = NULL);
     bool matches_filter(jabberoo::Agent& a);
private:
     BaseGabberWindow*   _base;
     jabberoo::AgentList _agent_list;
     jabberoo::Agent*    _current_agent;
     Gtk::CTree*         _ctreeAgent;
     Gtk::Entry*         _entServer;
     Gnome::Entry*       _gentServer;
     Filter              _filter;

public:
     SigC::Signal1<void, jabberoo::Agent*> agent_selected;
};

class AgentBrowserDlg
     : public BaseGabberDialog
{
public:
     static void execute();
protected:
     AgentBrowserDlg(const string& server, const Element& query);
     virtual ~AgentBrowserDlg() {}
     // Event handlers
     void on_info_clicked();
     void on_register_clicked();
     void on_search_clicked();
     void on_close_clicked();
     void on_agent_selected(jabberoo::Agent* agent);
     gint on_key_pressed(GdkEventKey* e);
     // Static IQ Callback
     static void handle_agents_IQ_s(const Element& iq);
private:
     AgentBrowser*       _browser;
     Gtk::Button*        _infoBtn;
     Gtk::Button*        _registerBtn;
     Gtk::Button*        _searchBtn;
     jabberoo::Agent*    _agent;
};

class AgentRegisterDruid
     : public BaseGabberWindow
{
public:
     AgentRegisterDruid(jabberoo::Agent& a);
     virtual ~AgentRegisterDruid() {}
protected:
     void handle_request_reply(const Element& iq);
     void on_register_reply(const Element& iq);
     void on_Fields_prepare();
     bool on_Fields_next();
     void on_Registered_prepare();
     bool on_Registered_back();
     void on_finish();
     void on_cancel();
     void set_progress(const string& status, bool active);
     gint on_refresh();
     void agent_replace(const string& old_agent, const string& new_agent);
     void handle_unregister_reply(const Element& iq);
private:
     typedef list<Gtk::Entry *> EntryList;
     typedef list<string> StringList;

     Gnome::Druid* _druid;
     Gtk::VBox*    _fields_vbox;
     Gtk::Label*   _instr_lbl;

     Gtk::Label*   _lblRegistered;
     Gnome::DruidPage* _RDruid_loading;
     Gnome::DruidPage* _RDruid_fields;
     Gnome::DruidPage* _RDruid_registering;
     Gnome::DruidPage* _RDruid_registered;
     Gnome::AppBar* _barStatus;
     string _key;
     EntryList _entrys;
     StringList _field_names;
     jabberoo::Agent& _agent;
     bool _init_interface;
     SigC::Connection _refresh_timer;
};

#endif
