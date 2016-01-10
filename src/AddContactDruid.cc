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

#include "AddContactDruid.hh"

#include "ContactInfoInterface.hh"
#include "GabberApp.hh"
#include "GabberUtility.hh"

#include <libgnome/gnome-i18n.h>
#include <gtk--/table.h>
#include <gnome--/dialog.h>
#include <gnome--/href.h>

using namespace jabberoo;
using namespace GabberUtil;

AddContactDruid* AddContactDruid::_Dialog = NULL;

void AddContactDruid::display(Page first_page)
{
     _Dialog = manage(new AddContactDruid(first_page, ""));
}

void AddContactDruid::display(const string& jid)
{
     if (G_App->getSession().roster().containsJID(jid))
     {
	  ContactInfoDlg::display(jid);
	  return;
     }
     else if (G_App->isAgent(jid))
     {
	  // Do nothing, let the GabberApp code handle it.
	  return;
     }
     else
     {
	  _Dialog = manage(new AddContactDruid(auNickname, jid));
     }
}

void AddContactDruid::display(Agent& agent)
{
     _Dialog = manage(new AddContactDruid(auSearch, agent));
}

AddContactDruid::AddContactDruid(Page first_page, const string& jid)
     : BaseGabberWindow("AUDruid_win"),
       _protocol(0),
       _agent(0)
{
     // Set JID if we have it already
     if (!jid.empty())
	  _JID = jid;

     init();
     _druid->set_page(*translate_page(first_page));
     _druid->set_buttons_sensitive(false, true, true);
     // Display
     show();
}

AddContactDruid::AddContactDruid(Page first_page, Agent& agent)
     : BaseGabberWindow("AUDruid_win"),
       _protocol(0),
       _agent(manage(new jabberoo::Agent(agent)))
{
     init();
     _druid->set_page(*translate_page(first_page));
     // Display
     show();
}

AddContactDruid::~AddContactDruid()
{
     delete _itemRoster;
}

void AddContactDruid::init()
{
     // Grab some widgets
     _barStatus         = getWidget<Gnome::AppBar>("AUDruid_Status_bar");
     _entUsername       = getEntry("AUDruid_normaladd_username_txt");
     _entUsername       ->changed.connect(slot(this, &AddContactDruid::on_Username_changed));
     _entUsername       ->key_press_event.connect(slot(this, &AddContactDruid::on_Username_key_press));
     _entServer         = getEntry("AUDruid_normaladd_server_txt");
     _entServer         ->changed.connect(slot(this, &AddContactDruid::on_Server_changed));
     _entJID            = getEntry("AUDruid_JabberID_ent");
     _entJID            ->changed.connect(slot(this, &AddContactDruid::on_JID_changed));
     getCheckButton("AUDruid_jabberid_rdo")->toggled.connect(slot(this, &AddContactDruid::on_JID_toggled));
     _waiting           = false;

     // Setup pointers for all of the pages
     _druid = getWidget<Gnome::Druid>("AUDruid_druid");
     _druid->cancel.connect(slot(this, &AddContactDruid::on_cancel));
     // Intro page
     _AUDruid_intro     = getWidget<Gnome::DruidPage>("AUDruid_intro");
     // * Choice
     // --------------------------------------------------------------------------------
     _AUDruid_choice    = getWidget<Gnome::DruidPage>("AUDruid_choice");
     _AUDruid_choice    ->prepare.connect(slot(this, &AddContactDruid::on_Choice_prepare)); 
     _AUDruid_choice    ->next.connect(slot(this, &AddContactDruid::on_Choice_next));
     // * Protocol
     // --------------------------------------------------------------------------------
     _AUDruid_protocol  = getWidget<Gnome::DruidPage>("AUDruid_protocol");
     _AUDruid_protocol  ->prepare.connect(slot(this, &AddContactDruid::on_Protocol_prepare));
     _AUDruid_protocol  ->next.connect(slot(this, &AddContactDruid::on_Protocol_next));
     _AUDruid_protocol  ->back.connect(slot(this, &AddContactDruid::on_Protocol_back));
     // Initialize the browser
     _browserProtocol   = manage(new AgentBrowser(this, "AUDruid_protocol"));
     // Hook up the browser
     _browserProtocol   ->agent_selected.connect(slot(this, &AddContactDruid::on_protocol_selected));
     _browserProtocol   ->set_view_filter(AgentBrowser::agTransport);
     // * Normaladd
     // --------------------------------------------------------------------------------
     _AUDruid_normaladd = getWidget<Gnome::DruidPage>("AUDruid_normaladd");
     _AUDruid_normaladd ->prepare.connect(slot(this, &AddContactDruid::on_Normaladd_prepare));
     _AUDruid_normaladd ->next.connect(slot(this, &AddContactDruid::on_Normaladd_next));
     _AUDruid_normaladd ->back.connect(slot(this, &AddContactDruid::on_Normaladd_back));
     getWidget<Gnome::Entry>("AUDruid_normaladd_server_gent")->prepend_history(0, G_App->getCfg().get_server());
     // * Agents
     // --------------------------------------------------------------------------------
     _AUDruid_agents    = getWidget<Gnome::DruidPage>("AUDruid_agents");
     _AUDruid_agents    ->prepare.connect(slot(this, &AddContactDruid::on_Agents_prepare));
     _AUDruid_agents    ->next.connect(slot(this, &AddContactDruid::on_Agents_next));
     _AUDruid_agents    ->back.connect(slot(this, &AddContactDruid::on_Agents_back));
     // Initialize the browser
     _browserSearch     = manage(new AgentBrowser(this, "AUDruid_agents"));
     // Hook up the browser
     _browserSearch     ->agent_selected.connect(slot(this, &AddContactDruid::on_agent_selected));
     _browserSearch     ->set_view_filter(AgentBrowser::agSearchable);
     // * Search
     // --------------------------------------------------------------------------------
     _AUDruid_search    = getWidget<Gnome::DruidPage>("AUDruid_search");
     _AUDruid_search    ->prepare.connect(slot(this, &AddContactDruid::on_Search_prepare));
     _AUDruid_search    ->next.connect(slot(this, &AddContactDruid::on_Search_next));
     _results_clist     = 0;
     _vboxSearch        = getWidget<Gtk::VBox>("AUDruid_search_vbox");
     _lblLoading        = getLabel("AUDruid_search_loading_lbl");
     // * Search Results
     // --------------------------------------------------------------------------------
     _AUDruid_searchresults = getWidget<Gnome::DruidPage>("AUDruid_searchresults");
     _AUDruid_searchresults ->next.connect(slot(this, &AddContactDruid::on_SearchResults_next));
     _results_scroll    = getWidget<Gtk::ScrolledWindow>("AUDruid_searchresults_results_scroll");
     // * Nickname
     // --------------------------------------------------------------------------------
     _AUDruid_nickname  = getWidget<Gnome::DruidPage>("AUDruid_nickname");
     _AUDruid_nickname  ->prepare.connect(slot(this, &AddContactDruid::on_Nickname_prepare));
     _AUDruid_nickname  ->next.connect(slot(this, &AddContactDruid::on_Nickname_next));
     _AUDruid_nickname  ->back.connect(slot(this, &AddContactDruid::on_Nickname_back));
     _entNickname       = getEntry("AUDruid_nickname_Nickname_txt");
     _entNickname       ->changed.connect(slot(this, &AddContactDruid::on_Nickname_changed));
     // * Groups
     // --------------------------------------------------------------------------------
     _AUDruid_groups    = getWidget<Gnome::DruidPage>("AUDruid_groups");
     _AUDruid_groups    ->prepare.connect(slot(this, &AddContactDruid::on_Groups_prepare));
     _itemRoster        = new Roster::Item("null", "null@null");
     _groups_editor     = manage(new GroupsEditor(this, "AUDruid_groups", _itemRoster));
     // * Request
     // --------------------------------------------------------------------------------
     _AUDruid_request   = getWidget<Gnome::DruidPage>("AUDruid_request");
     _AUDruid_request   ->next.connect(slot(this, &AddContactDruid::on_Request_next));
     // * Final
     // --------------------------------------------------------------------------------
     _AUDruid_final     = getWidget<Gnome::DruidPage>("AUDruid_final");
     _AUDruid_final     ->prepare.connect(slot(this, &AddContactDruid::on_Final_prepare));
     _AUDruid_final     ->next.connect(slot(this, &AddContactDruid::on_Final_next));
     _AUDruid_final     ->finish.connect(slot(this, &AddContactDruid::on_finish));
     _txtRequest        = getWidget<Gtk::Text>("AUDruid_s10nrequest_txt");
     getCheckButton("AUDruid_confirm_Again_chk")->toggled.connect(slot(this, &AddContactDruid::on_AddAgain_toggled));
     // * Finish
     // --------------------------------------------------------------------------------
     _AUDruid_finish    = getWidget<Gnome::DruidPage>("AUDruid_finish");
     _AUDruid_finish    ->finish.connect(slot(this, &AddContactDruid::on_finish));

     _lastPage          = NULL;

     // Grab the history and put it in the gnome entries (a cooler combo box)
     G_App->getCfg().loadEntryHistory(getWidget<Gnome::Entry>("AUDruid_normaladd_server_gent"));
}

Gnome::DruidPage* AddContactDruid::translate_page(Page given_page)
{
     switch(given_page)
     {
     case auIntro:
	  return _AUDruid_intro;
     case auChoice:
	  return _AUDruid_choice;
     case auNormaladd:
	  return _AUDruid_normaladd;
     case auAgents:
	  return _AUDruid_agents;
     case auSearch:
	  return _AUDruid_search;
     case auNickname:
	  return _AUDruid_nickname;
     }
     return NULL;
}

void AddContactDruid::on_Username_changed()
{
     // If server and username are filled in
     if (!_entServer->get_text().empty() && !_entUsername->get_text().empty())
	  _druid->set_buttons_sensitive(true, true, true);
     else
	  _druid->set_buttons_sensitive(true, false, true);
}

gint AddContactDruid::on_Username_key_press(GdkEventKey* e)
{
	/* Disable this for now since this prevents foo@hotmail.com for the username. sigh.
 *     if (e->keyval == GDK_at)
 *     {
 *	  // HACK --  to stop the signal from continuing, and the @ from being put into the Username field
 *	  gtk_signal_emit_stop_by_name(GTK_OBJECT(_entUsername->gtkobj()), "key_press_event");
 *	  _entServer->grab_focus();
 *     }
 */
     return 0;
}

void AddContactDruid::on_Server_changed()
{
     // If server and username are filled in
     if (!_entServer->get_text().empty() && !_entUsername->get_text().empty())
	  _druid->set_buttons_sensitive(true, true, true);
     else
	  _druid->set_buttons_sensitive(true, false, true);
}

void AddContactDruid::on_JID_changed()
{
     if (!_entJID->get_text().empty())
	  _druid->set_buttons_sensitive(false, true, true);
}

void AddContactDruid::on_JID_toggled()
{
     if (_entJID->is_sensitive())
	  _entJID->set_sensitive(false);
     else
	  _entJID->set_sensitive(true);
}

void AddContactDruid::on_cancel()
{
     close();
}

void AddContactDruid::set_progress(const string& status, bool active)
{
     _barStatus->pop();
     if (!status.empty())
	  _barStatus->push(status);
     _barStatus->get_progress()->set_activity_mode(active);
     if (active)
     {
	  if (_refresh_timer.connected())
	       _refresh_timer.disconnect();
	  _refresh_timer = Gtk::Main::timeout.connect(slot(this, &AddContactDruid::on_refresh), 50);
     }
     else if (_refresh_timer.connected())
     {
	  _refresh_timer.disconnect();
	  _barStatus->get_progress()->set_percentage(0.00);
     }
}

gint AddContactDruid::on_refresh()
{
     // Refresh the progressbar
     float pvalue = _barStatus->get_progress()->get_current_percentage();
     pvalue += 0.01;

     if (pvalue <= 0.01)
     {
	  _barStatus->get_progress()->set_orientation(GTK_PROGRESS_LEFT_TO_RIGHT);
	  pvalue = 0.01;
     }
     else if (pvalue >= 0.99)
     {
	  _barStatus->get_progress()->set_orientation(GTK_PROGRESS_RIGHT_TO_LEFT);
	  pvalue = 0.01;
     }

     _barStatus->get_progress()->set_percentage(pvalue);

     return TRUE;
}


// * Choice
// -------------------------------------------------------------------------------------
void AddContactDruid::on_Choice_prepare()
{
     _druid->set_buttons_sensitive(false, true, true);
}
    
bool AddContactDruid::on_Choice_next()
{
     // Depending upon which AUDruid_*_rdo they have selected, redirect
     if(getCheckButton("AUDruid_search_rdo")->get_active())
	  _druid->set_page(*_AUDruid_agents);
     else if (getCheckButton("AUDruid_protocol_rdo")->get_active())
	  _druid->set_page(*_AUDruid_protocol);
     else if (getCheckButton("AUDruid_jabberid_rdo")->get_active())
     {
	  _JID = toUTF8(_entJID, _entJID->get_text());
	  _druid->set_page(*_AUDruid_nickname);
     }

     _lastPage = _AUDruid_choice;

     return true;
}


// * Protocol
// -------------------------------------------------------------------------------------
void AddContactDruid::on_Protocol_prepare()
{
     // Loading...
     set_progress(_("Loading the list of agents..."), true);

     // Clear the browse tree
     _browserProtocol->clear();

     // Toggle the buttons
     _druid->set_buttons_sensitive(true, false, true);

     // Send request to the server for a filter
     G_App->getSession().queryNamespace("jabber:iq:agents", slot(this, &AddContactDruid::handle_Protocol_iq), G_App->getCfg().get_server());
}

void AddContactDruid::on_protocol_selected(jabberoo::Agent* agent)
{
    if (agent)
    {
        // Set the sensitivity of the appropriate buttons
        _druid->set_buttons_sensitive(true, !agent->transport().empty(), true);
        _protocol = agent;
    }
}

void AddContactDruid::handle_Protocol_iq(const Element& iq)
{
     // No longer loading
     set_progress("", false);

     Gnome::Dialog* d;

     // If we get a good query back, create a new AgentList
     // from the packet and display it in the browser
     if (iq.cmpAttrib("type", "result"))
     {
	  // Extract the query 
	  const Element* query = iq.findElement("query");
	  if (query != NULL)
	  {
	       // Update the agents
	       _browserProtocol->update_agents(G_App->getCfg().get_server(), *query);
	  }
	  else
	  {
	       d = manage(Gnome::Dialogs::warning(_("Error receiving XML for Agent Browser, see standard output.")));
			// d is a child window of this window:
			d->set_parent(*_thisWindow);
	       d->set_modal(true); 
	       cerr << "ERROR->Unable to extract query for AgentBrowser: " << iq.toString() << endl;
	  }
     }
     else
     {
	  d = manage(Gnome::Dialogs::warning(_("Error, this server does not support agents.")));
		// d is a child window of this window:
		d->set_parent(*_thisWindow);
	  d->set_modal(true);
     }
}

bool AddContactDruid::on_Protocol_next()
{
     // Make sure the user has registered with the agent
     string agent = _protocol->JID();
     bool onroster = false;
     Gnome::Dialog* d;
     for (Roster::iterator it = G_App->getSession().roster().begin(); it != G_App->getSession().roster().end(); it++)
     {
	  if (JID::compare(JID::getUserHost(it->getJID()), agent) == 0)
	  {
	       onroster = true;
	       break;
	  }
     }
     if (!onroster)
     {
	  manage(new AgentRegisterDruid(*_protocol));
	  d = manage(Gnome::Dialogs::ok(_("You must register with this agent first.")));
	  d->set_modal(true);
		// d is a child window of this window:
		d->set_parent(*_thisWindow);
     }
     
     // Set the server
     _entServer->set_text(fromUTF8(_entServer, agent));
     getWidget<Gnome::Entry>("AUDruid_normaladd_server_gent")->set_sensitive(false);
     
     _druid->set_page(*_AUDruid_normaladd);
     
     _lastPage = _AUDruid_protocol;
     
     return true;
}

bool AddContactDruid::on_Protocol_back()
{
     set_progress("", false);

     _druid->set_page(*_AUDruid_choice);
     return true;
}


// * Normaladd
// -------------------------------------------------------------------------------------
void AddContactDruid::on_Normaladd_prepare()
{
     // They can't click next until they at least enter a username
     _druid->set_buttons_sensitive(true, false, true);
}

bool AddContactDruid::on_Normaladd_next()
{
     string username = toUTF8(_entUsername, _entUsername->get_text());
     string server = toUTF8(_entServer, _entServer->get_text());

     // Remove spaces and @'s
     for (string::size_type i = 0; i < username.length(); i++)
     {
	  if (username[i] == ' ') // If character at i is a space
	       username.erase(i, 1); // Erase the character at i
	  else if (username[i] == '@') // If character at i is @
	       username[i] = '%'; // Replace the character at i
     }

     _JID = username + "@" + server;

     // Save the server gnome entry history
     G_App->getCfg().saveEntryHistory(getWidget<Gnome::Entry>("AUDruid_normaladd_server_gent"));

     // Jump to Edit User Nickname page
     _druid->set_page(*_AUDruid_nickname);

     _lastPage = _AUDruid_normaladd;

     return true;
}

bool AddContactDruid::on_Normaladd_back()
{
     // _lastPage should be set
     g_return_val_if_fail(_lastPage != 0, false);

     // go back to the proper page
     _druid->set_page(*_lastPage);
     return true;
}

// * Agents
// -------------------------------------------------------------------------------------
void AddContactDruid::on_Agents_prepare()
{
     // Loading...
     set_progress(_("Loading the list of agents..."), true);

     // Clear the browse tree
     _browserSearch->clear();

     // Toggle the buttons
     _druid->set_buttons_sensitive(true, false, true);

     // Send request to the server for a filter
     G_App->getSession().queryNamespace("jabber:iq:agents", slot(this, &AddContactDruid::handle_Agents_iq), G_App->getCfg().get_server());
}

void AddContactDruid::on_agent_selected(jabberoo::Agent* agent)
{
    if (agent)
    {
        // Set the sensitivity of the appropriate buttons
        _druid->set_buttons_sensitive(true, agent->isSearchable(), true);
        _agent = agent;
    }
}

void AddContactDruid::handle_Agents_iq(const Element& iq)
{
     // No longer loading
     set_progress("", false);

     Gnome::Dialog* d;

     // If we get a good query back, create a new AgentList
     // from the packet and display it in the browser
     if (iq.cmpAttrib("type", "result"))
     {
	  // Extract the query 
	  const Element* query = iq.findElement("query");
	  if (query != NULL)
	  {
	       // Update the agents
	       _browserSearch->update_agents(G_App->getCfg().get_server(), *query);
	  }
	  else
	  {
	       d = manage(Gnome::Dialogs::warning(_("Error receiving XML for Agent Browser, see standard output.")));
			// d is a child window of this window:
			d->set_parent(*_thisWindow);
	       d->set_modal(true); 
	       cerr << "ERROR->Unable to extract query for AgentBrowser: " << iq.toString() << endl;
	  }
     }
     else
     {
	  d = manage(Gnome::Dialogs::warning(_("Error, this server does not support agents.")));
		// d is a child window of this window:
		d->set_parent(*_thisWindow);
	  d->set_modal(true);
     }
}

bool AddContactDruid::on_Agents_next()
{
     _druid->set_page(*_AUDruid_search);
     return true;
}

bool AddContactDruid::on_Agents_back()
{
     set_progress("", false);
     _druid->set_page(*_AUDruid_choice);
     return true;
}


// * Search
// -------------------------------------------------------------------------------------
void AddContactDruid::on_Search_prepare()
{
     if (!_agent) return;

     // Set the agent labels
     Gtk::Label* l = getLabel("AUDruid_search_agent_lbl");
     l->set_text(fromUTF8(l, _agent->JID()));

     // Clear the vbox and set the loading label
     _vboxSearch->children().clear();
     _lblLoading->set_text(_("Loading..."));

     set_progress(_("Loading information from agent..."), true);

     // Set the search page
     _lastPage = _AUDruid_search;

     // Send query to the agent
     _agent->requestSearch(slot(this, &AddContactDruid::handle_search_request_reply));
     _waiting = true;

     // Toggle the buttons
     _druid->set_buttons_sensitive(true, false, true);
}

void AddContactDruid::handle_search_request_reply(const Element& iq)
{
     _vboxSearch->children().clear();
     _entrys.clear();
     _field_names.clear();
     if (iq.cmpAttrib("type", "result"))
     {
          // Extract the query 
          const Element* query = iq.findElement("query");
	  Element::const_iterator it = query->begin();
	  
	  _field_names.push_back("JID");
	  
	  // Create fields table
	  Gtk::Table* fld_tbl = manage(new Gtk::Table(query->size()-1, 2));
	  fld_tbl->set_row_spacings(4);
	  fld_tbl->set_col_spacings(4);
	  fld_tbl->set_border_width(0);
	  // Add the table to the vbox
	  _vboxSearch->pack_start(*fld_tbl, true, true, 8);
	  int row = 0;
	  
	  for (; it != query->end(); it++)
	  {
	       // Cast the child element into a tag
	       if ((*it)->getType() != Node::ntElement)
		    continue;
	       Element& t = *static_cast<Element*>(*it);
	       
	       if (t.getName() == "instructions")
	       {
		    _lblLoading->set_text(fromUTF8(_lblLoading, t.getCDATA()));
	       }
	       else if (t.getName() == "key")
	       {
		    // Jabber docs say key is not necessary for searches but some servers complain
		    // without it.  
		    _key = t.getCDATA();
	       }
	       else if (t.getName() == "x" && !t.getAttrib("xmlns").empty())
	       {
		    //FIXME: jabber:x:data support (Jabber forms)
		    //if (t.cmpAttrib("xmlns", "jabber:x:data"))

		    //FIXME: jabber:x:oob support (Gnome::HRef links)
		    //if (t.cmpAttrib("xmlns", "jabber:x:oob"))
	       }
	       else
	       {
		    // Create a label
		    Gtk::Label* lbl = manage(new Gtk::Label());
		    lbl->set_text(fromUTF8(lbl, t.getName() + ":"));
		    lbl->set_justify(GTK_JUSTIFY_RIGHT);
		    lbl->set_alignment(1.0, 0.0);
		    fld_tbl->attach(*lbl, 0, 1, row, row+1,GTK_FILL,GTK_FILL);
		    
		    // Create entry
		    Gtk::Entry* entry = manage(new Gtk::Entry());
		    // Set the entry data
		    entry->set_text(fromUTF8(entry, t.getCDATA()));
		    fld_tbl->attach(*entry, 1, 2, row, row+1,GTK_EXPAND|GTK_FILL,0);
		    _field_names.push_back(t.getName());
		    string* fieldname = &(_field_names.back());
		    entry->set_data("fieldname", fieldname);
		    _entrys.push_back(entry);
		    
		    row++;
	       }
	  }
	  _vboxSearch->show_all();
	  // Remove the clist (if it has been added)
	  _results_scroll->remove();
	  // Make a new clist
	  _results_clist = manage(new Gtk::CList(Gtk::SArray(_field_names)));
	  _results_clist->select_row.connect(slot(this, &AddContactDruid::on_SearchResults_select));
	  _results_clist->unselect_row.connect(slot(this, &AddContactDruid::on_SearchResults_unselect));
	  _results_clist->click_column.connect(slot(this, &AddContactDruid::on_SearchResults_click_column));
	  for (unsigned int i = 0; i < _field_names.size(); i++)
	       _results_clist->set_column_auto_resize(i, true);
	  _results_clist->set_selection_mode(GTK_SELECTION_SINGLE);
	  _results_clist->show();
	  _results_scroll->add(*_results_clist);
	  set_progress(_("Information loaded."), false);
	  _waiting = false;
	  // Toggle the buttons
	  _druid->set_buttons_sensitive(true, true, true);
     }
     else
     {
	  Gnome::Dialog* d = manage(Gnome::Dialogs::warning(_("Error attempting to search agent.")));
	  d->set_modal(true);
		// d is a child window of this window:
		d->set_parent(*_thisWindow);
	  close();
     }
}

bool AddContactDruid::on_Search_next()
{
     // Set the agent label
     getLabel("AUDruid_searchresults_agent_lbl")->set_text(_agent->JID());

     EntryList::const_iterator eit = _entrys.begin();

     // Get next session ID
     string id = G_App->getSession().getNextID();

     Packet iq("iq");
     iq.setID(id);
     iq.setTo(_agent->JID());
     iq.getBaseElement().putAttrib("type", "set");

     Element* query = iq.getBaseElement().addElement("query");
     query->putAttrib("xmlns", "jabber:iq:search");
     if (!_key.empty())
	  query->addElement("key", _key);

     for (; eit != _entrys.end(); eit++)
     {
          Gtk::Entry* entry = static_cast<Gtk::Entry*>(*eit);
	  string& field = *static_cast<string *>(entry->get_data("fieldname"));

	  if (!entry->get_text().empty()) {
               query->addElement(field, toUTF8(entry, entry->get_text()));
	  }
     }
     G_App->getSession() << iq;
     G_App->getSession().registerIQ(id, slot(this, &AddContactDruid::on_search_reply));

     // clear clist so old search results are confused with new results
     _results_clist->clear();
     for (unsigned int i = 0; i < _results_clist->columns().size(); i++)
	  _results_clist->set_column_resizeable(i, false);
     set_progress(_("Searching..."), true);

     // Change page
     _druid->set_page(*_AUDruid_searchresults);

     // Toggle the buttons
     _druid->set_buttons_sensitive(true, false, true);
     return true;
}

void AddContactDruid::on_search_reply(const Element& iq)
{
     _results_clist->freeze();
     _results_clist->clear();
     if (iq.cmpAttrib("type", "result"))
     {
	  // We can recv multiple query tags each of which can contain multiple item tags
          Element::const_iterator qit = iq.begin();
	  for (; qit != iq.end(); qit++)
	  {
	       if ((*qit)->getType() != Node::ntElement)
		    continue;
	       Element& query = *static_cast<Element*>(*qit);

	       Element::const_iterator ait = query.begin();
	       
	       for (; ait != query.end(); ait++)
	       {
		    if ((*ait)->getType() != Node::ntElement)
			 continue;
		    Element& item = *static_cast<Element*>(*ait);
		    if (item.getName() == "item")
		    {
                         StringList::iterator sit = _field_names.begin();
		         const char **rowdata;
		         int num_fields = _results_clist->columns().size(), i;

		         rowdata = new const char * [num_fields];
			 rowdata[0] = g_strdup(fromUTF8(_results_clist, item.getAttrib("jid")).c_str());
			 // First field_name is JID which we get from the item tag
		         for (i = 1, sit++; sit != _field_names.end(); sit++, i++)
		         {
                              string* field = &(*sit);
			      
			      rowdata[i] = g_strdup(fromUTF8(_results_clist, item.getChildCData(*field)).c_str());
		         }
		         _results_clist->append_row(rowdata);
			 for (i = 0; i < num_fields; i++)
                              g_free((gpointer)rowdata[i]);
		         delete [] rowdata;
	            }
	       }
	  }
     }
     else if (iq.cmpAttrib("type", "error"))
     {
	  const Element* error = iq.findElement("error");
	  string errormessage = string(_("Error ")) 
	       + string(error->getAttrib("code")) 
	       + string(":\n") 
	       + string(iq.getChildCData("error"));
	  Gnome::Dialog* d = manage(Gnome::Dialogs::warning(errormessage.c_str()));
	  d->set_modal(true);
		// d is a child window of this window:
		d->set_parent(*_thisWindow);
     }
     
     _results_clist->thaw();
     for (unsigned int i = 0; i < _results_clist->columns().size(); i++)
	  _results_clist->set_column_resizeable(i, true);
     _results_scroll->show();
     set_progress(_("Search completed."), false);

     // Toggle the buttons
     _druid->set_buttons_sensitive(true, false, true);
}


// * Search Results
// -------------------------------------------------------------------------------------
void AddContactDruid::on_SearchResults_select(int row, int col, GdkEvent* e)
{
     _druid->set_buttons_sensitive(true, true, true);
}

void AddContactDruid::on_SearchResults_unselect(int row, int col, GdkEvent* e)
{
     _druid->set_buttons_sensitive(true, false, true);
}

void AddContactDruid::on_SearchResults_click_column(int column)
{
     _results_clist->set_sort_column(column);
     _results_clist->sort();
}

bool AddContactDruid::on_SearchResults_next()
{
     if (_results_clist->selection().begin() != _results_clist->selection().end()) 
     {
          _results_clist->get_text(_results_clist->selection().begin()->get_row_num(), 0, _JID);
	  _lastPage = _AUDruid_searchresults;
	  _druid->set_page(*_AUDruid_nickname);
	  return true;
     }
     else
     {
	  _druid->set_page(*_AUDruid_searchresults);
	  return true;
     }
}


// * Nickname
// -------------------------------------------------------------------------------------
void AddContactDruid::on_Nickname_prepare()
{
     // Set the JID label
     Gtk::Label* l = getLabel("AUDruid_nickname_JID");
     l->set_text(fromUTF8(l, _JID));

     // - Grab vCard
     // Get next session ID
     string id = G_App->getSession().getNextID();

     // Construct vCard request
     Packet iq("iq");
     iq.setID(id);
     iq.setTo(_JID);
     iq.getBaseElement().putAttrib("type", "get");
     Element* vCard = iq.getBaseElement().addElement("vCard");
     vCard->putAttrib("xmlns", "vcard-temp");
     vCard->putAttrib("version", "2.0");
     vCard->putAttrib("prodid", "-//HandGen//NONSGML vGen v1.0//EN");

     // Send the vCard request
     G_App->getSession() << iq;
     G_App->getSession().registerIQ(id, slot(this, &AddContactDruid::get_vCard));

     _entNickname->set_text(fromUTF8(_entNickname, JID::getUser(_JID)));
}

void AddContactDruid::get_vCard(const Element& t)
{
     if (!t.empty())
     {
	  string curtext;
	  const Element* vCard = t.findElement("vCard");
	  if (vCard != NULL)
	  {
	       curtext = vCard->getChildCData("NICKNAME");
	       if (_entNickname->get_text().empty() ||
		   toUTF8(_entNickname, _entNickname->get_text()) == JID::getUser(_JID))
		    _entNickname->set_text(fromUTF8(_entNickname, curtext));
	       curtext = vCard->getChildCData("FN");
	       Gtk::Label* l = getLabel("AUDruid_nickname_FullName_lbl");
	       l->set_text(fromUTF8(l, curtext));
	       curtext = vCard->getChildCData("EMAIL");
	       l = getLabel("AUDruid_nickname_eMail_lbl");
	       l->set_text(fromUTF8(l, curtext));
	       curtext = vCard->getChildCData("URL");
	       Gnome::HRef* hrefWeb = getWidget<Gnome::HRef>("AUDruid_nickname_WebSite_href");
	       hrefWeb->set_url(fromUTF8(hrefWeb, curtext));
	       hrefWeb->set_label(fromUTF8(hrefWeb, curtext));
	       const Element* ADR = vCard->findElement("ADR");
	       if (ADR != NULL)
	       {
		    curtext = ADR->getChildCData("COUNTRY");
		    l = getLabel("AUDruid_nickname_Country_lbl");
		    l->set_text(fromUTF8(l, curtext));
	       }
	  }
     }
     else
	  // Set the nickname to the username, if they haven't already set the nickname
	  if (_entNickname->get_text().empty())
	       _entNickname->set_text(_entUsername->get_text());
}

bool AddContactDruid::on_Nickname_next()
{
     // Set the nickname
     _nickname = toUTF8(_entNickname, _entNickname->get_text());

     // Enable back, since they can go back here
     _druid->set_page(*_AUDruid_groups);
     _druid->set_buttons_sensitive(true, true, true);

     return true;
}

bool AddContactDruid::on_Nickname_back()
{
     // _lastPage should be set
     g_return_val_if_fail(_lastPage != 0, false);

     // go back to the proper page
     _druid->set_page(*_lastPage);
     return true;
}

void AddContactDruid::on_Nickname_changed()
{
     // If nickname is filled in, allow them to go forward
     if (!_entNickname->get_text().empty())
	  _druid->set_buttons_sensitive((_lastPage != NULL), true, true);
     else
	  _druid->set_buttons_sensitive((_lastPage != NULL), false, true);
}


// * Groups
// -------------------------------------------------------------------------------------
void AddContactDruid::on_Groups_prepare()
{
     // Prepare the roster item
     delete _itemRoster;
     _itemRoster = new Roster::Item(_JID, _nickname);

     // Refresh the groups list
     _groups_editor->refresh(_itemRoster);
}


// * Request
// -------------------------------------------------------------------------------------
bool AddContactDruid::on_Request_next()
{
     _druid->set_page(*_AUDruid_final);
     _druid->set_buttons_sensitive(true, true, true);

     return true;
}


// * Final
// -------------------------------------------------------------------------------------
void AddContactDruid::on_Final_prepare()
{
     getCheckButton("AUDruid_confirm_Again_chk")->set_active(false);
     on_AddAgain_toggled();
}

bool AddContactDruid::on_Final_next()
{
     // Detect whether we should start again
     Gtk::CheckButton* check = getCheckButton("AUDruid_confirm_Again_chk");
     if (check->get_active())
     {
	  on_finish();
	  _entUsername->set_text("");
	  _entNickname->set_text("");
	  _entServer->set_text("");
	  getWidget<Gnome::Entry>("AUDruid_normaladd_server_gent")->set_sensitive(true);
	  _druid->set_page(*_AUDruid_choice);
     }
     else
	  _druid->set_page(*_AUDruid_finish);

     return true;
}

void AddContactDruid::on_finish()
{
     if (_nickname != "" && _JID != "")
     {
	  // Add the user to the roster
	  G_App->getSession().roster() << *_itemRoster;
	  if (_txtRequest->get_chars(0, -1) != "")
	       G_App->getSession() << Presence(_JID, Presence::ptSubRequest, Presence::stInvalid, toUTF8(_txtRequest, _txtRequest->get_chars(0, -1)));
	  else
	       G_App->getSession() << Presence(_JID, Presence::ptSubRequest);

	  // Display group management
	  if (getCheckButton("AUDruid_confirm_Groups_chk")->get_active())
	       G_App->manage_groups = true;

	  // Close the druid
	  if (!getCheckButton("AUDruid_confirm_Again_chk")->get_active())
	       close();
     }
}

void AddContactDruid::on_AddAgain_toggled()
{
     if (getCheckButton("AUDruid_confirm_Again_chk")->get_active())
	  _druid->set_show_finish(false);
     else
	  _druid->set_show_finish(true);
}

