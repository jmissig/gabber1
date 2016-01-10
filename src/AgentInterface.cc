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

#include "AgentInterface.hh"

#include "AddContactDruid.hh"
#include "ContactInfoInterface.hh"
#include "GabberApp.hh"
#include "GabberUtility.hh"

#include <libgnome/gnome-i18n.h>
#include <gtk--/table.h>
#include <gnome--/dialog.h>

using namespace jabberoo;
using namespace GabberUtil;

// ---------------------------------------------------------
//
// Browser
//
// ---------------------------------------------------------
AgentBrowser::AgentBrowser(BaseGabberWindow* base, const string& base_string)
     : _base(base),
       _agent_list(Element("null"), G_App->getSession()),
       _filter(agAll)
{
     // Widget pointers
     _entServer  = _base->getEntry(string(base_string + "_Server_ent").c_str());
     _gentServer = _base->getGEntry(string(base_string + "_Server_gent").c_str());
     _ctreeAgent = _base->getCTree(string(base_string + "_ctree").c_str());

     // Setup handlers
     _base->getButton(string(base_string + "_Browse_btn").c_str())->clicked.connect(slot(this, &AgentBrowser::on_browse_clicked));
     _entServer->key_press_event.connect(slot(this, &AgentBrowser::on_key_press_event));
     
     // Setup the tree
     _ctreeAgent->set_line_style(GTK_CTREE_LINES_NONE);
     _ctreeAgent->set_expander_style(GTK_CTREE_EXPANDER_TRIANGLE);
     _ctreeAgent->set_row_height(17);						    // Hack for making sure icons aren't chopped off
     _ctreeAgent->column_titles_hide();
     _ctreeAgent->set_compare_func(&GabberUtil::strcasecmp_clist_items);
     _ctreeAgent->set_column_auto_resize(0, true);
     _ctreeAgent->set_auto_sort(true);
     _ctreeAgent->set_indent(20);	
     
     // Setup tree events
     _ctreeAgent->tree_expand.connect(slot(this, &AgentBrowser::on_tree_expand));
     _ctreeAgent->tree_select_row.connect(slot(this, &AgentBrowser::on_tree_select));
     _ctreeAgent->tree_unselect_row.connect(slot(this, &AgentBrowser::on_tree_unselect));

     // Grab the history and put it in the gnome entry (a cooler combo box)
     G_App->getCfg().loadEntryHistory(_gentServer);
}

AgentBrowser::~AgentBrowser()
{
}

void AgentBrowser::update_agents(const string& server, const Element& query)
{
     // Load into an agents list
     _agent_list.load(query, G_App->getSession());

     // Set the server entry
     _entServer->set_text(fromUTF8(_entServer, server));

     // Walk the tree list and render
     _ctreeAgent->freeze();
     for(AgentList::iterator it = _agent_list.begin(); it != _agent_list.end(); it++)
	  render_agent(*it, NULL);
     _ctreeAgent->thaw();
}

void AgentBrowser::set_view_filter(Filter f)
{
     _filter = f;
     refresh();
}

void AgentBrowser::clear()
{
     _ctreeAgent->freeze();
     // Clear the tree
     _ctreeAgent->clear();
     _current_agent = NULL;

     agent_selected(NULL);
     cerr << "clear() _agent now NULL" << endl;
     _ctreeAgent->thaw();
}

void AgentBrowser::browse()
{
     on_browse_clicked();
}

gint AgentBrowser::on_key_press_event(GdkEventKey* e)
{
     // If Enter is pressed, act like they pressed browse
     switch (e->keyval)
     {
     case GDK_Return:
	  browse();
	  break;
     }

     return FALSE;
}

void AgentBrowser::on_browse_clicked()
{
     // Save the gnome entry history
     G_App->getCfg().saveEntryHistory(_gentServer);
     G_App->getCfg().loadEntryHistory(_gentServer);

     clear();

     // Send request to the server for the update
     G_App->getSession().queryNamespace("jabber:iq:agents", slot(this, &AgentBrowser::handle_agents_IQ), toUTF8(_entServer, _entServer->get_text()));
     // Save the gnome entry history
     G_App->getCfg().saveEntryHistory(_gentServer);
     G_App->getCfg().loadEntryHistory(_gentServer);
}

void AgentBrowser::on_tree_expand(Gtk::CTree::Row r)
{
     Agent* a = static_cast<Agent*>(r.get_data());

     // Fetch any subagents
     if ((a != NULL) && a->hasAgents())
	  a->fetch();
}

void AgentBrowser::on_tree_select(Gtk::CTree::Row r, int col)
{
     _current_agent = static_cast<Agent*>(r.get_data());

     if (_current_agent != NULL)
     {
	  agent_selected(_current_agent);
	  cerr << "tree_select() _agent now " << _current_agent->JID() << endl;
     }
}

void AgentBrowser::on_tree_unselect(Gtk::CTree::Row r, int col)
{
     _current_agent = NULL;
     agent_selected(NULL);
     cerr << "tree_unselect() _agent now NULL" << endl;
}

void AgentBrowser::on_agent_fetch_complete(bool successful, GtkCTreeNode* agentnode, GtkCTreeNode* dummynode)
{
     _ctreeAgent->freeze();

     // Get the agent stored in the specified node
     void* nodedata = gtk_ctree_node_get_row_data(_ctreeAgent->gtkobj(), agentnode);
     Agent& a = *static_cast<Agent*>(nodedata);

     if (successful)
     {
	  // Remove dummy node
	  if (dummynode)
	       gtk_ctree_remove_node(_ctreeAgent->gtkobj(), dummynode);
	  
	  // Walk the agent's sublist and render individual nodes
	  for (Agent::iterator it = a.begin(); it != a.end(); it++)
	       render_agent(*it, agentnode);
     }
     else
     {
	  // Update dummy node text
	  gtk_ctree_node_set_text(_ctreeAgent->gtkobj(), dummynode, 0, _("Error occurred querying this agent. Please try again later."));
     }

     // Make sure this agentnode is expanded
     gtk_ctree_expand(_ctreeAgent->gtkobj(), agentnode);

     _ctreeAgent->thaw();
}

// Normal handler -- updates existing window
void AgentBrowser::handle_agents_IQ(const Element& iq)
{
     Gnome::Dialog* d;

     // If we get a good query back, clear the existing agentlist and tree
     // and generate a new agent list
     if (iq.cmpAttrib("type", "result"))
     {
	  // Extract the query 
	  const Element* query = iq.findElement("query");
	  if (query != NULL)
	  {      
	       // Re load the agent list
	       _agent_list.load(*query, G_App->getSession());
	       // Refresh the view
	       refresh();
	       // Update the server entry
	       _entServer->set_text(fromUTF8(_entServer, iq.getAttrib("from")));
	  }
	  else
	  {
	       d = manage(Gnome::Dialogs::warning(_("Error receiving XML for Agent Browser, see standard output.")));
	       d->set_modal(true); 
	       // d is a child window of this window:
	       d->set_parent(*(_base->getBaseWindow()));
	       cerr << "ERROR->Unable to extract query for AgentBrowser: " << iq.toString() << endl;
	  }
     }
     else
     {
	  d = manage(Gnome::Dialogs::warning(_("Error, this server does not support agents.")));
	  // d is a child window of this window:
	  d->set_parent(*(_base->getBaseWindow()));
	  d->set_modal(true);
     }
}

void AgentBrowser::refresh()
{
     clear();

     _ctreeAgent->freeze();
     // Render the agent list
     for(AgentList::iterator it = _agent_list.begin(); it != _agent_list.end(); it++)
	  render_agent(*it, NULL);
     _ctreeAgent->thaw();
}

void AgentBrowser::render_agent(Agent& a, GtkCTreeNode* parent)
{
     // Only render this agent if it matches the filter
     // this is here instead of in refresh() because subagents need to match too
     if (matches_filter(a))
     {
	  char* header[1] = { (char*)fromUTF8(_ctreeAgent, a.name()).c_str()};
	  // Create the node
	  GtkCTreeNode* node = gtk_ctree_insert_node(_ctreeAgent->gtkobj(), parent, NULL, header, 0,
						     NULL, NULL, NULL, NULL, !a.hasAgents(), false);
	  if (a.hasAgents())
	  {
	       // Insert a dummy row so the expander shows up
	       char* dummy_header[1] = { "Loading..." };
	       GtkCTreeNode* dummynode = gtk_ctree_insert_node(_ctreeAgent->gtkobj(), node, NULL, dummy_header, 0, 
							       NULL, NULL, NULL, NULL, true, false);
	       // Hook up to the agent's refresh event
	       a.evtFetchComplete.connect(bind(slot(this, &AgentBrowser::on_agent_fetch_complete), node, dummynode));
	  }
	  
	  // Set row data to be a ref to this agent
	  gtk_ctree_node_set_row_data(_ctreeAgent->gtkobj(), node, &a);
     }
}

bool AgentBrowser::matches_filter(Agent& a)
{
     switch (_filter)
     {
     case agAll:
	  return true;
     case agRegisterable:
	  return a.isRegisterable();
     case agSearchable:
	  return a.isSearchable();
     case agGCCapable:
	  return a.isGCCapable();
     case agAgents:
	  return a.hasAgents();
     case agTransport:
	  return !a.transport().empty();
     }
     return false;
}

// ---------------------------------------------------------
//
// Agent Browser Dialog
//
// ---------------------------------------------------------
AgentBrowserDlg::AgentBrowserDlg(const string& server, const Element& query)
     : BaseGabberDialog("AgentBrowser_dlg")
{
     // Store widget pointers
     _infoBtn     = getButton("AgentBrowser_Info_btn");
     _registerBtn = getButton("AgentBrowser_Register_btn");
     _searchBtn   = getButton("AgentBrowser_Search_btn");
     _browser     = manage(new AgentBrowser(this, "AgentBrowser"));
     _browser     ->update_agents(server, query);

     // Setup handlers
     _infoBtn->clicked.connect(slot(this, &AgentBrowserDlg::on_info_clicked));
     _registerBtn->clicked.connect(slot(this, &AgentBrowserDlg::on_register_clicked));
     _searchBtn->clicked.connect(slot(this, &AgentBrowserDlg::on_search_clicked));
     getButton("AgentBrowser_Close_btn")->clicked.connect(slot(this, &AgentBrowserDlg::on_close_clicked));
     _browser->agent_selected.connect(slot(this, &AgentBrowserDlg::on_agent_selected));
     _thisWindow->key_press_event.connect(slot(this, &AgentBrowserDlg::on_key_pressed));

     show();
}

void AgentBrowserDlg::on_info_clicked()
{
     if (_agent != NULL)
     {
	  manage(new AgentInfoDlg(*_agent));
     }
}

void AgentBrowserDlg::on_register_clicked()
{
     if (_agent != NULL)
     {
	  manage(new AgentRegisterDruid(*_agent));
     }
}

void AgentBrowserDlg::on_search_clicked()
{
     if (_agent != NULL)
     {
          AddContactDruid::display(*_agent);
     }
}

void AgentBrowserDlg::on_close_clicked()
{
     close();
}

void AgentBrowserDlg::on_agent_selected(jabberoo::Agent* agent)
{
     // Set the sensitivity of the appropriate buttons
    if (agent)
    {
        _infoBtn->set_sensitive(true);
        _registerBtn->set_sensitive(agent->isRegisterable());
        _searchBtn->set_sensitive(agent->isSearchable());
    }
    else
    {
        _infoBtn->set_sensitive(false);
        _registerBtn->set_sensitive(false);
        _searchBtn->set_sensitive(false);
    }
    _agent = agent;
}

gint AgentBrowserDlg::on_key_pressed(GdkEventKey* e)
{
     switch (e->keyval)
     {
     case GDK_Escape:
	  on_close_clicked();
     }
     return FALSE;
}     

void AgentBrowserDlg::execute()
{
     // Send request to the server for a filter
     G_App->getSession().queryNamespace("jabber:iq:agents", slot(&AgentBrowserDlg::handle_agents_IQ_s), G_App->getCfg().get_server());
}

// Static handler -- instantiates a new window
void AgentBrowserDlg::handle_agents_IQ_s(const Element& iq)
{
     Gnome::Dialog* d;

     // If we get a good query back, create a new AgentList
     // from the packet and display it in the browser
     if (iq.cmpAttrib("type", "result"))
     {
	  // Extract the query 
	  const Element* query = iq.findElement("query");
	  if (query != NULL)
		{
	       // Create a new browser, passing a new AgentList
	       manage(new AgentBrowserDlg(iq.getAttrib("from"), *query));
		}
	  else
	  {
	       d = manage(Gnome::Dialogs::warning(_("Error receiving XML for Agent Browser, see standard output.")));
	       d->set_modal(true); 
	       main_dialog(d);
	       cerr << "ERROR->Unable to extract query for AgentBrowser: " << iq.toString() << endl;
	  }
     }
     else
     {
	  d = manage(Gnome::Dialogs::warning(substitute(_("Error, %s does not support agents."), fromUTF8(iq.getAttrib("from")))));
	  d->set_modal(true);
	  main_dialog(d);
     }
}


// ---------------------------------------------------------
//
// AgentRegisterDruid
//
// ---------------------------------------------------------
AgentRegisterDruid::AgentRegisterDruid(Agent& a)
     : BaseGabberWindow("RegDruid_win"), _agent(a), _init_interface(false)
{
     // Get widgets
     _druid = getWidget<Gnome::Druid>("RegDruid_druid");
     _fields_vbox = getWidget<Gtk::VBox>("RegDruid_fields_vbox");
     _instr_lbl = getLabel("RegDruid_fields_loading_lbl");
     _lblRegistered = getLabel("RegDruid_registered_explain_lbl");
     _barStatus = getWidget<Gnome::AppBar>("RegDruid_Status_bar");
     
     // Hookup event
     _druid->cancel.connect(slot(this, &AgentRegisterDruid::on_cancel));

     // Setup pages
     _RDruid_loading     = getWidget<Gnome::DruidPage>("RegDruid_loading");
     _RDruid_fields      = getWidget<Gnome::DruidPage>("RegDruid_fields");
     _RDruid_fields      ->prepare.connect(slot(this, &AgentRegisterDruid::on_Fields_prepare));
     _RDruid_fields      ->next.connect(slot(this, &AgentRegisterDruid::on_Fields_next));
     _RDruid_registering = getWidget<Gnome::DruidPage>("RegDruid_registering");
     _RDruid_registered  = getWidget<Gnome::DruidPage>("RegDruid_registered");
     _RDruid_registered  ->prepare.connect(slot(this, &AgentRegisterDruid::on_Registered_prepare));
     _RDruid_registered  ->back.connect(slot(this, &AgentRegisterDruid::on_Registered_back));
     _RDruid_registered  ->finish.connect(slot(this, &AgentRegisterDruid::on_finish));

     // Set the JID
     getLabel("RegDruid_loading_jid_lbl")    ->set_text(_agent.JID());
     getLabel("RegDruid_fields_jid_lbl")     ->set_text(_agent.JID());
     getLabel("RegDruid_registering_jid_lbl")->set_text(_agent.JID());
     getLabel("RegDruid_registered_jid_lbl") ->set_text(_agent.JID());

     // Set the agent info
     Gtk::Label* l = getLabel("RegDruid_loading_Service_lbl");
     l->set_text(fromUTF8(l, _agent.service()));
     l = getLabel("RegDruid_loading_Description_lbl");
     l->set_text(fromUTF8(l, _agent.description()));

     // Set the page to the loading page, and disallow back and next
     _druid->set_page(*_RDruid_loading);
     _druid->set_buttons_sensitive(0,0,1);

     // Display
     show();

     set_progress(_("Loading registration information..."), true);

     // Send query to the agent
     _agent.requestRegister(slot(this, &AgentRegisterDruid::handle_request_reply));

}

void AgentRegisterDruid::on_cancel()
{
     close();
}

void AgentRegisterDruid::set_progress(const string& status, bool active)
{
     _barStatus->pop();
     if (!status.empty())
	  _barStatus->push(status);
     _barStatus->get_progress()->set_activity_mode(active);
     if (active)
     {
	  if (_refresh_timer.connected())
	       _refresh_timer.disconnect();
	  _refresh_timer = Gtk::Main::timeout.connect(slot(this, &AgentRegisterDruid::on_refresh), 50);
     }
     else if (_refresh_timer.connected())
     {
	  _refresh_timer.disconnect();
	  _barStatus->get_progress()->set_percentage(0.00);
     }
}

gint AgentRegisterDruid::on_refresh()
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

void AgentRegisterDruid::handle_request_reply(const Element& iq)
{
     // No longer loading
     set_progress("", false);

     if (iq.cmpAttrib("type", "result"))
     {
          // Extract the query 
          const Element* query = iq.findElement("query");

          if (_init_interface) {
               _key = query->getChildCData("key");
	  } else {
               Element::const_iterator it = query->begin();

	       // Create fields table
	       Gtk::Table* fld_tbl = manage(new Gtk::Table(query->size()-1, 2));
	       fld_tbl->set_row_spacings(4);
	       fld_tbl->set_col_spacings(4);
	       fld_tbl->set_border_width(0);
	       // Add the table to the vbox
	       _fields_vbox->pack_start(*fld_tbl, true, true, 8);
	       int row = 0;

               for (; it != query->end(); it++)
               {
                    // Cast the child element into a tag
		    if ((*it)->getType() != Node::ntElement)
			 continue;
                    Element& t = *static_cast<Element*>(*it);

                    if (t.getName() == "instructions") {
                         _instr_lbl->set_text(fromUTF8(_instr_lbl, t.getCDATA()));
                    } else if (t.getName() == "registered") {
                         // we are already registered with the agent
                    } else if (t.getName() == "key") {
                         _key = t.getCDATA();
	            } else {
			 // Create a label
			 Gtk::Label* lbl = manage(new Gtk::Label());
			 lbl->set_text(fromUTF8(lbl, t.getName() + ":"));
			 lbl->set_justify(GTK_JUSTIFY_RIGHT);
			 lbl->set_alignment(1.0, 0.0);
			 fld_tbl->attach(*lbl, 0, 1, row, row+1,GTK_FILL,GTK_FILL);

			 // Create entry
			 Gtk::Entry* entry = manage(new Gtk::Entry());
			 // If it's a password, mask it.
			 if (t.getName() == "password")
			      entry->set_visibility(false);
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
	       // Show everything and indicate that we have stuff
	       _fields_vbox->show_all();
	       _init_interface = true;
          }
	  _barStatus->pop();
	  _barStatus->get_progress()->set_activity_mode(false);
	  // Send them to the fields page
          _druid->set_page(*_RDruid_fields);
     }
     else
     {
          Gnome::Dialog* d = manage(Gnome::Dialogs::warning(substitute(_("Error attempting to register with %s.\n\nThe agent is either down, unreachable, or misconfigured."), fromUTF8(iq.getAttrib("from")))));
	  // d is a child window of this window:
	  d->set_parent(*_thisWindow);
          d->set_modal(true);
	  close();
     }
}

void AgentRegisterDruid::on_Fields_prepare()
{
     // They can't go back
     _druid->set_buttons_sensitive(0, 1, 1);
}

bool AgentRegisterDruid::on_Fields_next() 
{
     // They can't do anything except cancel while waiting for the reply
     _druid->set_buttons_sensitive(0, 0, 1);

     EntryList::const_iterator eit = _entrys.begin();

     // Get next session ID
     string id = G_App->getSession().getNextID();

     Packet iq("iq");
     iq.setID(id);
     iq.setTo(_agent.JID());
     iq.getBaseElement().putAttrib("type", "set");

     Element* query = iq.getBaseElement().addElement("query");
     query->putAttrib("xmlns", "jabber:iq:register");
     query->addElement("key", _key);

     for (; eit != _entrys.end(); eit++)
     {
	     Gtk::Entry* entry = static_cast<Gtk::Entry*>(*eit);
             string& field = *static_cast<string *>(entry->get_data("fieldname"));

             if (!entry->get_text().empty()) {
		  // Remove spaces from username if such a field is present
		  if (field == "username")
		  {
		       string username = toUTF8(entry, entry->get_text());
		       for (string::size_type i = 0; i < username.length(); i++)
		       {
			    if (username[i] == ' ')   // If character at i is a space
				 username.erase(i, 1); // Erase the character at i
		       }
		       query->addElement(field, username);
		  }
		  else
		  {
		       query->addElement(field, toUTF8(entry, entry->get_text()));
		  }
             }
     }
     G_App->getSession() << iq;
     G_App->getSession().registerIQ(id, slot(this, &AgentRegisterDruid::on_register_reply));

     if (!getEntry("RegDruid_AgentReplace_ent")->get_text().empty())
	  agent_replace(getEntry("RegDruid_AgentReplace_ent")->get_text(), _agent.JID());

     set_progress(_("Sending registration information..."), true);

     _druid->set_page(*_RDruid_registering);

     return true;
}

void AgentRegisterDruid::on_register_reply(const Element& iq)
{
     if (!iq.cmpAttrib("type", "result"))
     {
          const Element* error = iq.findElement("error");
	  string message;
	  _RDruid_registered->set_title(_("Registration Failed"));
	  getFrame("RegDruid_Success_frame")->hide();
	  if (error)
               message = substitute(_("Registration Failed:\n%s"), fromUTF8(_druid, error->getCDATA()));
	  else
	       message = _("Registration Failed");
	  _lblRegistered->set_text(message);
     }

     set_progress("", false);

     _druid->set_page(*_RDruid_registered);
}

void AgentRegisterDruid::on_Registered_prepare()
{
     // Show the finish button
     _druid->set_buttons_sensitive(0, 1, 1);
     _druid->set_show_finish(true);
}

bool AgentRegisterDruid::on_Registered_back()
{
     _agent.requestRegister(slot(this, &AgentRegisterDruid::handle_request_reply));

     return false;
}

void AgentRegisterDruid::on_finish()
{
     close();
}

void AgentRegisterDruid::agent_replace(const string& old_agent, const string& new_agent)
{
	// This function is sort of hackish, but I feel the feature outweighs the hackish-ness of it
     if (!getCheckButton("RegDruid_Unregister_chk")->get_active())
	  return;

     // Walk the roster
     Roster::iterator it = G_App->getSession().roster().begin();
     string old_jid, new_jid;
     for ( ; it != G_App->getSession().roster().end(); it++)
     {
	  old_jid = it->getJID();
	  if ((JID::getHost(old_jid) == old_agent) && (!G_App->isAgent(old_jid)))
	  {
	       // This user was set up for the old agent

	       // Create the new JID from the old one - basically, replace the host
	       string new_jid = JID::getUser(old_jid) + "@" + new_agent;
	       if (!JID::getResource(old_jid).empty())
		    new_jid += "/" + JID::getResource(old_jid);

	       // Copy the info
	       Roster::Item new_item(new_jid, it->getNickname());
	       // Add all groups in this item
	       for (Roster::Item::iterator i = it->begin(); i != it->end(); i++)
		    new_item.addToGroup(*i);

	       // Add the new user to the roster
	       G_App->getSession().roster() << new_item;

	       // Subscribe to this new user
	       G_App->getSession() << Presence(new_jid, Presence::ptSubRequest, Presence::stInvalid, toUTF8(_("Automatic subscription request.")));

	       // Delete the old user
	       G_App->getSession().roster().deleteUser(it->getJID());
	  }
     }

     // Send query to the agent
     G_App->getSession().queryNamespace("jabber:iq:register", slot(this, &AgentRegisterDruid::handle_unregister_reply), old_agent);
}

void AgentRegisterDruid::handle_unregister_reply(const Element& iq)
{
     if (iq.cmpAttrib("type", "result"))
     {
          // Extract the query 
          const Element* query = iq.findElement("query");
	  string key;
	  if (query)
	       key = query->getChildCData("key");

	  string id = G_App->getSession().getNextID();

	  Packet niq("iq");
	  niq.setID(id);
	  niq.setTo(iq.getAttrib("from"));
	  niq.getBaseElement().putAttrib("type", "set");

	  Element* newQuery = niq.getBaseElement().addElement("query");
	  newQuery->putAttrib("xmlns", "jabber:iq:register");
	  newQuery->addElement("key", key);
	  newQuery->addElement("remove");
	  G_App->getSession() << niq;
     }
}
