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

#ifndef INCL_ADD_CONTACT_DRUID_HH
#define INCL_ADD_CONTACT_DRUID_HH

#include "jabberoo.hh"
#include "jabberoox.hh"

#include "AgentInterface.hh"
#include "BaseGabberWindow.hh"
#include "GroupsInterface.hh"

#include <gnome--/appbar.h>
#include <gnome--/druid.h>

using namespace std;

class AddContactDruid :
     public BaseGabberWindow
{
public:
     // Page type
     enum Page {
	  auIntro, auChoice, auNormaladd, auAgents, auSearch, auNickname
     };
     static void display(Page first_page);
     static void display(const string& jid);
     static void display(jabberoo::Agent& a);
     // Destructor
     ~AddContactDruid();
protected:
     // Default constructor
     AddContactDruid(Page first_page, const string& jid);
     AddContactDruid(Page first_page, jabberoo::Agent& agent);
     void init();
     // Non-static manipulators
     Gnome::DruidPage* translate_page(Page given_page);
     void on_Username_changed();
     gint on_Username_key_press(GdkEventKey* e);
     void on_Server_changed();
     void on_JID_changed();
     void on_JID_toggled();
     void on_cancel();
     void set_progress(const string& status, bool active);
     gint on_refresh();
     void on_Choice_prepare();
     bool on_Choice_next();
     void on_Protocol_prepare();
     bool on_Protocol_next();
     bool on_Protocol_back();
     void on_protocol_selected(jabberoo::Agent* agent);
     void handle_Protocol_iq(const Element& iq);
     void on_Normaladd_prepare();
     bool on_Normaladd_next();
     bool on_Normaladd_back();
     void on_Agents_prepare();
     void on_agent_selected(jabberoo::Agent* agent);
     void handle_Agents_iq(const Element& iq);
     bool on_Agents_next();
     bool on_Agents_back();
     void on_Search_prepare();
     void handle_search_request_reply(const Element& iq);
     bool on_Search_next();
     void on_search_reply(const Element& iq);
     void on_SearchResults_select(int row, int col, GdkEvent* e);
     void on_SearchResults_unselect(int row, int col, GdkEvent* e);
     void on_SearchResults_click_column(int column);
     bool on_SearchResults_next();
     void on_Nickname_prepare();
     void get_vCard(const Element& t);
     bool on_Nickname_next();
     bool on_Nickname_back();
     void on_Nickname_changed();
     void on_Groups_prepare();
     bool on_Request_next();
     void on_Final_prepare();
     bool on_Final_next();
     void on_finish();
     void on_AddAgain_toggled();
private:
     static AddContactDruid* _Dialog;
     Gnome::Druid* _druid;
     Gnome::DruidPage* _AUDruid_intro;
     Gnome::DruidPage* _AUDruid_choice;
     Gnome::DruidPage* _AUDruid_protocol;
     Gnome::DruidPage* _AUDruid_normaladd;
     Gnome::DruidPage* _AUDruid_agents;
     Gnome::DruidPage* _AUDruid_nickname;
     Gnome::DruidPage* _AUDruid_groups;
     Gnome::DruidPage* _AUDruid_request;
     Gnome::DruidPage* _AUDruid_final;
     Gnome::DruidPage* _AUDruid_finish;
     Gnome::DruidPage* _lastPage;
     Gnome::AppBar*    _barStatus;
     Gtk::Entry* _entUsername;
     Gtk::Entry* _entServer;
     Gtk::Entry* _entNickname;
     Gtk::Entry* _entJID;
     Gtk::Text*  _txtRequest;
     string _JID;
     string _nickname;
     // Search
     typedef list<Gtk::Entry *> EntryList;
     typedef list<string> StringList;
     Gnome::DruidPage* _AUDruid_search;
     Gnome::DruidPage* _AUDruid_searchresults;
     jabberoo::Agent*  _protocol;
     jabberoo::Agent*  _agent;
     AgentBrowser*     _browserProtocol;
     AgentBrowser*     _browserSearch;
     GroupsEditor*     _groups_editor;
     jabberoo::Roster::Item* _itemRoster;
     Gtk::VBox*        _vboxSearch;
     Gtk::Label*       _lblLoading;
     Gtk::CList*    _results_clist;
     Gtk::Frame*    _results_frame;
     Gtk::ScrolledWindow* _results_scroll;
     SigC::Connection _refresh_timer;
     EntryList _entrys;
     StringList _field_names;
     string _key;
     bool _waiting;
};

#endif
