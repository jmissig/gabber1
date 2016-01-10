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

#include "RosterView.hh"

#include "AddContactDruid.hh"
#include "ContactInfoInterface.hh"
#include "ContactInterface.hh"
#include "FTInterface.hh"
#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "GabberWin.hh"
#include "GroupsInterface.hh"
#include "MessageManager.hh"
#include "MessageViews.hh"
#include "StatusInterface.hh"

#include <libgnome/gnome-triggers.h>
#include <libgnome/gnome-url.h>
#include <libgnome/gnome-util.h>
#include <gtk--/style.h>

using Gtk::CTree;
using namespace jabberoo;
using namespace GabberUtil;

// Temporary/scratch map for composing group nodes -- used by getGroupNode && On_Refresh
NodeMap GROUPS;
NodeMap USERS;

static bool group_pixmap_loaded = false;
static Gdk_Pixmap group_drag_pixmap;
static Gdk_Bitmap group_drag_bitmap;
static bool offline_pixmap_loaded = false;
static Gdk_Pixmap offline_drag_pixmap;
static Gdk_Bitmap offline_drag_bitmap;

RosterView::RosterView(Gtk::CTree* tree, Gtk::ScrolledWindow* tscroll, Session& session)
     : _filter(rvfAll), _session(session), _tree(tree), _tscroll(tscroll),
     _row(-1), _col(-1), _dnd_data(0)
{
     // Trick up tree
     _tree->set_line_style(GTK_CTREE_LINES_NONE);
     _tree->set_expander_style(GTK_CTREE_EXPANDER_TRIANGLE);
     _tree->set_column_visibility(colJID, false);
     if (((_tree->get_style()->get_font().ascent() + _tree->get_style()->get_font().descent() + 1) < 17) &&
	 (G_App->getCfg().colors.icons))
	  _tree->set_row_height(17);                                        // Hack for making sure icons aren't chopped off
     _tree->column_titles_hide();
     _tree->set_compare_func(&GabberUtil::strcasecmp_clist_items);
     _tree->set_column_auto_resize(colNickname, true);
     _tree->set_auto_sort(true);
     _tree->set_indent(0);						    // Set the indent to 0 so that the tree is more compact
     // We set the drag icons ourselves
     _tree->set_use_drag_icons(false);

     // Setup tree events
     _tree->tree_expand.connect(slot(this, &RosterView::on_tree_expand));
     _tree->tree_collapse.connect(slot(this, &RosterView::on_tree_collapse));
     _tree->button_press_event.connect(slot(this, &RosterView::on_button_press));
     _tree->key_press_event.connect(slot(this, &RosterView::on_key_press));

     // Setup session events
     _session.roster().evtRefresh.connect(slot(this, &RosterView::on_update_refresh));
     _session.roster().evtPresence.connect(slot(this, &RosterView::on_presence));

     // Connect tree-move signal
     gtk_signal_connect(GTK_OBJECT(_tree->gtkobj()), "tree-move", GTK_SIGNAL_FUNC(&RosterView::on_tree_move), (gpointer) this);
     // allow the roster to be dragged around
     _tree->set_reorderable(true);
     _tree->set_drag_compare_func(&RosterView::is_drop_ok);
     // Setup DnD targets that the roster can receive
     GtkTargetEntry roster_dest_targets[] = {
       {"gtk-clist-drag-reorder", GTK_TARGET_SAME_WIDGET, 0},
     };
     int dest_num = sizeof(roster_dest_targets) / sizeof(GtkTargetEntry);
     // Detup DnD targets that the roster can send 
     GtkTargetEntry roster_source_targets[] = {
       {"text/x-jabber-roster-item", 0, 0},
       {"text/x-vcard", 0, 0},
       {"text/x-vcard-xml", 0, 0},
       {"text/x-jabber-id", 0, 0},
       {"gtk-clist-drag-reorder", GTK_TARGET_SAME_WIDGET, 0},
     };
     int source_num = sizeof(roster_source_targets) / sizeof(GtkTargetEntry);
     // need to call set_reorderable on the tree first so that the ctree's flag gets
     // set but it sets targets which we have to remove before we can set new targets
     gtk_drag_dest_unset(GTK_WIDGET(_tree->gtkobj()));
     gtk_drag_dest_set(GTK_WIDGET(_tree->gtkobj()), (GtkDestDefaults) (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP), roster_dest_targets, dest_num, (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE));
     gtk_drag_source_set(GTK_WIDGET(_tree->gtkobj()), (GdkModifierType) (GDK_BUTTON1_MASK | GDK_BUTTON3_MASK), roster_source_targets, source_num, (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE));
     // Setup DnD callbacks
     _tree->drag_data_get.connect(slot(this, &RosterView::on_drag_data_get));
     _tree->drag_data_received.connect(slot(this, &RosterView::on_drag_data_received));
     _tree->drag_begin.connect(slot(this, &RosterView::on_drag_begin));

     Gtk::Main::timeout.connect(slot(this, &RosterView::FlashEvents), 500);
}

void RosterView::clear()
{
     _tree->clear();
}

void RosterView::refresh(bool reload_cache)
{
     if (reload_cache)
     {
          G_App->getCfg().initPresenceCache(_tree->get_window());
     }
     // If ther timer is already running, stop it
     if (_refresh_timer.connected())
          _refresh_timer.disconnect();
     // start a new timer to refresh the roster
     _refresh_timer = Gtk::Main::timeout.connect(slot(this, &RosterView::on_refresh), 1000);
}

void RosterView::disable_presence_sounds(int millisecs)
{
     // We only start a new timer if there is no existing one waiting
     if (!_presence_sounds_timer.connected())
     {
	  // Start a timer for the given amount of milliseconds
	  _presence_sounds_timer = Gtk::Main::timeout.connect(slot(this, &RosterView::enable_presence_sounds), millisecs);
     }
}

gint RosterView::enable_presence_sounds()
{
     if (_presence_sounds_timer.connected())
	  _presence_sounds_timer.disconnect();

     return FALSE;
}

void RosterView::save_collapsed_groups()
{
     // Save the list of collapsed groups to the Gabber config file
     ConfigManager& cf = G_App->getCfg();

     cf.roster.collapsed_groups.clear();
     for(set<string>::iterator it = _collapsed_groups.begin(); it != _collapsed_groups.end(); it++)
     {
	  cf.roster.collapsed_groups.push_back(*it);
     }
     _collapsed_groups.clear();
}

void RosterView::load_collapsed_groups()
{
     // Load the list of collapsed groups from the Gabber config file
     ConfigManager& cf = G_App->getCfg();

     for (list<string>::iterator it = cf.roster.collapsed_groups.begin(); it != cf.roster.collapsed_groups.end(); it++)
     {
	  _collapsed_groups.insert(*it);
     }
}

void RosterView::set_view_filter(Filter f)
{
     _filter = f;
     refresh();
}

GtkCTreeNode* RosterView::get_group_node(const string& groupid, bool available)
{
     GtkCTreeNode* result = NULL;

     // Lookup this id
     NodeMap::iterator it = GROUPS.find(groupid);

     // If no such group is found, create one and return it
     if (it == GROUPS.end())
     {
	  // Create the node on the tree
	  char* header[2] = { (char*)fromUTF8(_tree, groupid).c_str(), (char*)groupid.c_str()};
	  result = gtk_ctree_insert_node(_tree->gtkobj(), NULL, NULL, header, 0, 
					 NULL, NULL, NULL, NULL, false, true);

	  // Ensure count of available items is set to 0
	  gtk_ctree_node_set_row_data(_tree->gtkobj(), result, NULL);

	  // Set group foreground/background colours
	  GdkColor bg = get_style_bg_color();
	  gtk_ctree_node_set_background(_tree->gtkobj(), result, &bg);
	  GdkColor fg = get_style_fg_color();
	  gtk_ctree_node_set_foreground(_tree->gtkobj(), result, &fg);

	  // Ensure the group is not selectable
	  gtk_ctree_node_set_selectable(_tree->gtkobj(), result, false);

	  // If this group name does not exist in the collapsed_groups set, we need to 
	  // make sure it's expanded
	  if (_collapsed_groups.find(header[colJID]) == _collapsed_groups.end())
	       gtk_ctree_expand(_tree->gtkobj(), result);
	  else
	       gtk_ctree_collapse(_tree->gtkobj(), result);

	  // Insert the node into the map for future reference
	  GROUPS.insert(make_pair(groupid, result));

     }
     // Otherwise return the node
     else
	  result = it->second;

     // Increment online cnt (stored in node data ptr) if the request
     // was made by an online item
     if (available)
     {
	  gint cnt = GPOINTER_TO_INT(gtk_ctree_node_get_row_data(_tree->gtkobj(), result)) + 1;
	  gtk_ctree_node_set_row_data(_tree->gtkobj(), result, GINT_TO_POINTER(cnt));
     }
     return result;
}

GtkCTreeNode* RosterView::create_item_node(GtkCTreeNode* groupnode, const Roster::Item& r)
{
     // Get ref to ConfigManager
     ConfigManager& cfgm = G_App->getCfg();

     // Create the node on the tree
     string nickname = fromUTF8(_tree, r.getNickname());
     char* header[2] = { (char*)nickname.c_str(), (char*)r.getJID().c_str() };
     GtkCTreeNode* n = gtk_ctree_insert_node(_tree->gtkobj(), groupnode, NULL, header, 0,
					     NULL, NULL, NULL, NULL, true, true);
     USERS.insert(make_pair(JID::getUserHost(r.getJID()), n));

     // Set color/icons appropriately
     bool showIcons = cfgm.colors.icons;
  
     // If this JID is available...
     if (_session.presenceDB().available(r.getJID()))
     {
	  // Lookup presence (since it's guaranteed to be there)
	  const Presence& p = *_session.presenceDB().find(r.getJID());

          string show = p.getShow_str();
	  string status = p.getStatus();
	  // Set text color
	  gtk_ctree_node_set_foreground(_tree->gtkobj(), n, cfgm.getPresenceColor(show));
	  if (showIcons)
	  {
               // Display the appropriate icon
	       gtk_ctree_node_set_pixtext(_tree->gtkobj(), n, colNickname, nickname.c_str(), 5, cfgm.getPresencePixmap(show), cfgm.getPresenceBitmap(show));
	  }
	  else
	  {
	       gtk_ctree_node_set_text(_tree->gtkobj(), n, colNickname, nickname.c_str());	
	  }    
     }
     // Else if the user doesn't really have a subscription with this JID...
     else if (r.getSubsType() == Roster::rsFrom || r.getSubsType() == Roster::rsNone)
     {
	  gtk_ctree_node_set_foreground(_tree->gtkobj(), n, cfgm.getPresenceColor("stalker"));
	  if (showIcons)
	       gtk_ctree_node_set_pixtext(_tree->gtkobj(), n, colNickname, nickname.c_str(), 5, cfgm.getPresencePixmap("stalker"), cfgm.getPresenceBitmap("stalker"));
     }
     // Else if this JID is _not_ available...
     else
     {
	  gtk_ctree_node_set_foreground(_tree->gtkobj(), n, cfgm.getPresenceColor("offline"));
	  // Indent this node since there is no picture beside it...21 is derived from adding the width of the icon(16) + the 5 
	  // pixel text offset
	  if (showIcons)
	       gtk_ctree_node_set_shift(_tree->gtkobj(), n, colNickname, 0, 21);
     }
     return n;
}

void RosterView::render_group_nodes()
{
     for (NodeMap::iterator i = GROUPS.begin(); i != GROUPS.end(); i++)
     {
	  gint onlinecount = GPOINTER_TO_INT(gtk_ctree_node_get_row_data(_tree->gtkobj(), i->second));
	  if (onlinecount > 0)
	  {
	       gchar* grp_title = g_strdup_printf("%s (%d)", fromUTF8(_tree, i->first).c_str(), onlinecount);
	       gtk_ctree_node_set_text(_tree->gtkobj(), i->second, colNickname, grp_title); // Set visible title appropriately
	       g_free(grp_title);
	       _total_online += onlinecount;
	  }
     }
}

gint RosterView::on_refresh()
{
     // Prep the tree
     _tree->freeze();
     float hscroll, vscroll;
     hscroll = _tscroll->get_hadjustment()->get_value(); // Save the horizontal scroll position
     vscroll = _tscroll->get_vadjustment()->get_value(); // Save the vertical scroll position

     // Get the current selected user, if any
     bool selectedUser = false;
     string selectedJID;
     if (!_tree->selection().empty())
     {
	  selectedUser = true;
	  Gtk::CTree::Row r = *_tree->selection().begin();
	  selectedJID = r[colJID].get_text();
     }
     
      _tree->clear();

     // Clear the groups nodemap
     GROUPS.clear();
     // CLear users
     USERS.clear();

     // First render all the temporary groups
     for (list<string>::iterator it = _temp_groups.begin(); it != _temp_groups.end(); it++)
	  get_group_node(*it, false);

     // Render each item in the roster
     for(Roster::iterator i = _session.roster().begin(); i != _session.roster().end(); i++)
     {
	  // Total Online
	  _total_online = 0;

	  // Determine if this person is online
	  bool available = _session.presenceDB().available(i->getJID());

	  // If it is a transport and we're set to not display transports, skip it,
	  // otherwise display it
	  if (G_App->isAgent(i->getJID()))
	  {
	       if (_filter == rvfAllNoAgents || _filter == rvfOnlineOnlyNoAgents)
		    continue;
	  }
	  // If this JID is not online and the filter is set to display online people only,
	  // skip this item
	  else if ( !(available) && 
		    !(i->isPending()) &&
		    (_filter == rvfOnlineOnly || _filter == rvfOnlineOnlyNoAgents) && 
		    (G_App->getMessageManager().getEvent(i->getJID()) == G_App->getMessageManager().getEvents().end()))
	       continue;

	  // Walk the groups contained in the item, creating an entry in each
	  for (Roster::Item::iterator grp_it = i->begin(); grp_it != i->end(); grp_it++)
	  {
	       // Retrieve/create the group node
	       GtkCTreeNode* groupnode = get_group_node(*grp_it, available);
	       // Create the item node on this group
	       create_item_node(groupnode, *i);
	  }

	  // Walk the temporary group node map and render the number of people
	  // online in each group
	  render_group_nodes();
	  // Set the number in the status indicators
	  StatusIndicator::display_online_contacts(_total_online);
     }

     // display users that are not in list but need to be in the roster temporarily because they have sent us a message
     for (list<Roster::Item>::iterator it = _nil_roster.begin(); it != _nil_roster.end(); it++)
     {
          for (Roster::Item::iterator grp_it = it->begin(); grp_it != it->end(); grp_it++)
          {
               GtkCTreeNode *groupnode = get_group_node(*grp_it, false);
               create_item_node(groupnode, *it);
          }
     }

     // Reset the scroll position
     _tscroll->get_hadjustment()->set_value(hscroll); // Set the horizontal scroll position
     _tscroll->get_vadjustment()->set_value(vscroll); // Set the horizontal scroll position


     // Select the user which was selected before the refresh
     if (selectedUser)
     {
	  for (Gtk::CTree_Helpers::RowIterator it = _tree->rows().begin(); it != _tree->rows().end(); it++)
	  {
	       if ((*it)[colJID].get_text() == selectedJID)
	       {
		    it->select();
		    break;
	       }
	       // If we don't find the user, then do nothing...
	  }
     }
     
     // Thaw the tree 
     _tree->thaw();

     _refresh_timer.disconnect();
     // return false to prevent the timer from being called again
     return FALSE;
}


void RosterView::on_presence(const string& jid, bool available, Presence::Type prev_type)
{
     refresh();

     // Grab the nickname
     string nickname;
     try {
	  nickname = fromUTF8(_tree, G_App->getSession().roster()[jid].getNickname());
     } catch (Roster::XCP_InvalidJID& e) {
	  nickname = fromUTF8(_tree, JID::getUser(jid));
     }

     if (available && prev_type == Presence::ptUnavailable)
     {
	  // Play UserAvailable sound
	  if (!_presence_sounds_timer.connected())
	  {
	       gnome_triggers_do(NULL, NULL, "gabber", "UserAvailable", NULL);
	       disable_presence_sounds(1500); // Disable presence sounds for 1.5 seconds
	  }
	  // Update the statusbar and clear in 30 seconds
	  G_Win->push_status_bar_msg(nickname + _(" is now online"), 30000);
     }
     else if (available && prev_type == Presence::ptAvailable)
     {
	  // This would probably cause way too many updates... 
	  // or perhaps we should give specific status changes?

	  // Update the statusbar and clear in 5 seconds
	  //G_Win->push_status_bar_msg(nickname + _(" changed status"), 5000);
	  ;
     }
     else if (!available && prev_type == Presence::ptAvailable)
     {
	  // Play UserOffline sound
	  if (!_presence_sounds_timer.connected())
	  {
	       gnome_triggers_do(NULL, NULL, "gabber", "UserOffline", NULL);
	       disable_presence_sounds(1500); // Disable presence sounds for 1.5 seconds
	  }
	  // Update the statusbar and clear in 30 seconds
	  G_Win->push_status_bar_msg(nickname + _(" is now offline"), 30000);
     }
}

void RosterView::on_tree_expand(Gtk::CTree::Row r)
{
     set<string>::iterator it = _collapsed_groups.find(r[colJID].get_text());
     if (it != _collapsed_groups.end())
	  _collapsed_groups.erase(it);
}

void RosterView::on_tree_collapse(Gtk::CTree::Row r)
{
     _collapsed_groups.insert(r[colJID].get_text());
}


int RosterView::on_button_press(GdkEventButton* e)
{
     if (!_tree->get_selection_info(e->x, e->y, &_row, &_col))
	  return 0;

     if (!_tree->row(_row).get_selectable())
	  return 0;

     _tree->row(_row).select();


     switch (e->type) 
     {
     case GDK_2BUTTON_PRESS: // Double-click
     {
	  // Only open message dialog for left button clicks
	  if (e->button == 1)
	  {
	       // Give the function the JabberID and whether control key is pressed
	       on_roster_row_activate(_tree->get_text(_row, colJID), e->state & GDK_CONTROL_MASK);
	  }
	  break;
     }
     case GDK_BUTTON_PRESS: // Single-click
	  // Check for rt-click
	  if (e->button == 3) 
	  {
	       // If it's a transport
	       if (G_App->isAgent(_tree->get_text(_row, colJID)))
	       {
		    G_Win->get_menuAgent()->show_all();
		    G_Win->get_menuAgent()->popup(e->button, e->time);
	       }
	       else
	       {
		    G_Win->get_menuUser()->show_all();

                    // Hide appropriate items if the user is NIL
                    string defaultJID = _tree->get_text(_row, colJID);
                    bool on_roster = true;
                    try {
                         G_App->getSession().roster()[JID::getUserHost(defaultJID)];
                    } catch (Roster::XCP_InvalidJID& e) {
                         on_roster = false;
                    }
                    if (on_roster)
                         G_Win->toggle_roster_popup(true);
                    else
                         G_Win->toggle_roster_popup(false);

		    G_Win->get_menuUser()->popup(e->button, e->time);
	       }
	  }
	  break;
     default:
	  break;
     }

     return 0;
}

int RosterView::on_key_press(GdkEventKey* e)
{
     if (_tree->selection().begin() == _tree->selection().end() )
	  return 0;

     // If they pressed the Keypad enter, make it act like a normal enter
     if (e->keyval == GDK_KP_Enter)
	  e->keyval = GDK_Return;

     Gtk::CTree::Row r = *_tree->selection().begin();

     // If they pressed enter or space on a group
//     if(!r.is_leaf() && (e->keyval == GDK_Return || e->keyval == GDK_space))
//     {
	  // We need to manually handle the expansion.
	  // Apparently pressing enter doesn't normally open/close a group
//	  r.toggle_expansion();

	  // And we don't need to do anything else
//	  return 0;
//     }

     if (!r.get_selectable())
	  return 0;

     switch (e->keyval) 
     {
     case GDK_Return:

	  // Give the function the JabberID and whether control key is pressed
	  on_roster_row_activate(r[colJID].get_text(), e->state & GDK_CONTROL_MASK);

	  break;
     default:
	  break;
     }

     return 0;
}

void RosterView::on_roster_row_activate(const string& jid, bool control_pressed)
{
     // Lookup default resource for this user
     string defaultJID = jid;
     try 
     {
	  defaultJID = _session.presenceDB().find(defaultJID)->getFrom();
     } 
     catch (PresenceDB::XCP_InvalidJID& e)
     {
     }
     
     MessageManager::EventList::iterator it = G_App->getMessageManager().getEvent(defaultJID);
     if (it != G_App->getMessageManager().getEvents().end())
     {
	  // display using it->first instead of defaultJID because defaultJID may not
	  // contain a resource or may not be the resource the event came from
	  G_App->getMessageManager().display(it->first, it->second);
	  refresh();

	  return;
     } else {
	it = G_App->getMessageManager().getEvent(jid);
	if (it != G_App->getMessageManager().getEvents().end()){
	  G_App->getMessageManager().display(it->first, it->second);
	  refresh();
	  return;
	} 
     }

     // Check for control key..
     if (control_pressed)
     {
	  // If control key is on, do the reverse of the default send method
	  if (G_App->getCfg().msgs.sendmsgs)
	       G_App->getMessageManager().display(defaultJID, MessageManager::translateType("chat"));
	  else
	       G_App->getMessageManager().display(defaultJID, MessageManager::translateType("normal"));
     }
     else
     {
	  // If control key is not on, send using preferred method
	  if (G_App->getCfg().msgs.sendmsgs)
	       G_App->getMessageManager().display(defaultJID, MessageManager::translateType("normal"));
	  else
	       G_App->getMessageManager().display(defaultJID, MessageManager::translateType("chat"));
     }

}

// Right-click menu for users

void RosterView::on_Message_activate()
{
     // Lookup default resource for this user
     string defaultJID = _tree->get_text(_row, colJID);
     try {
	  defaultJID = _session.presenceDB().find(defaultJID)->getFrom();
     } 
     catch (PresenceDB::XCP_InvalidJID& e) {}

     G_App->getMessageManager().display(defaultJID, MessageManager::translateType("normal"));
}

void RosterView::on_OOOChat_activate()
{
     // Lookup default resource for this user
     string defaultJID = _tree->get_text(_row, colJID);
     try {
	  defaultJID = _session.presenceDB().find(defaultJID)->getFrom();
     } 
     catch (PresenceDB::XCP_InvalidJID& e) {}

     G_App->getMessageManager().display(defaultJID, MessageManager::translateType("chat"));
}

void RosterView::on_SendRoster_activate()
{
     manage(new ContactSendDlg(_tree->get_text(_row, colJID)));
}

void RosterView::on_SendFile_activate()
{
    manage(new FTSendDlg(_tree->get_text(_row, colJID)));
}

void RosterView::on_EditUser_activate()
{
     bool on_roster = true;
     string jid = _tree->get_text(_row, colJID);
     try {
          G_App->getSession().roster()[jid];
     } catch (Roster::XCP_InvalidJID& e) {
          on_roster = false;
     }
     if (on_roster)
          ContactInfoDlg::display(jid);
     else
          ContactInfoDlg::display(jid, Roster::rsNone);
}

void RosterView::on_EditGroups_activate()
{
     manage(new EditGroupsDlg(_tree->get_text(_row, colJID)));
}

void RosterView::on_History_activate()
{
     string file = "file://";
     file += string(G_App->getLogFile(string(_tree->get_text(_row, colJID))));
     gnome_url_show(file.c_str());
}

void RosterView::on_S10nRequest_activate()
{
     G_App->getSession() << Presence(_tree->get_text(_row, colJID), Presence::ptSubRequest);  
}


void RosterView::on_DeleteUser_activate()
{
     Gnome::Dialog* d;
     Gtk::CTree::Row r = *_tree->selection().begin();
     string jid = r[colJID].get_text();  // The jid is stored in the Gtk:: CTree in UTF-8,
     string nickname = toUTF8(r[colNickname].get_text()); // while the nickname is in the local encoding.

     string question = substitute(_("Are you sure you want to remove %s from your roster?"), 
				  fromUTF8(_tree, nickname) + " (" + fromUTF8(_tree, jid) + ")");
     d = manage(Gnome::Dialogs::question_modal(question, // handle_RemoveUserDlg expects both string arguments to be in UTF-8.
					       bind(slot(this, &RosterView::handle_RemoveUserDlg), jid, nickname)));
     main_dialog(d);
}

void RosterView::on_AddUser_activate()
{
     string jid = _tree->get_text(_row, colJID);
     AddContactDruid::display(JID::getUserHost(jid));
}

// Right-click menu for transports

void RosterView::on_LoginTransport_activate()
{
     char* cpriority;
     cpriority = g_strdup_printf("%d", G_App->getCfg().get_priority());
     G_App->getSession() << Presence(_tree->get_text(_row, colJID), Presence::ptAvailable, G_App->getCfg().get_show(), G_App->getCfg().get_status(), string(cpriority));
     g_free(cpriority);
}

void RosterView::on_LogoutTransport_activate()
{
     G_App->getSession() << Presence(_tree->get_text(_row, colJID), Presence::ptUnavailable);
}

void RosterView::on_TransInfo_activate()
{
     manage(new AgentInfoDlg(_tree->get_text(_row, colJID)));
}


void RosterView::handle_RemoveUserDlg(int code, string jid, string nickname)
{
     Gnome::Dialog* d;
     string id, question;
     Packet iq("iq");

     switch (code)
     {
     case 0:
	  if (G_App->isAgent(jid))
	  {
	       // First send an unavailable presence so that
	       // all users of that transport go offline
	       G_App->getSession() << Presence(jid, Presence::ptUnavailable);

	       // Send an iq request to get the key and thus unregister the agent
	       id = G_App->getSession().getNextID();
	       iq.setID(id);
	       iq.setTo(jid);
	       iq.getBaseElement().putAttrib("type", "get");

	       Element* query = iq.getBaseElement().addElement("query");
	       query->putAttrib("xmlns", "jabber:iq:register");
	       G_App->getSession() << iq;
	       G_App->getSession().registerIQ(id, slot(this, &RosterView::handle_RemoveAgent_iq));

	       question = substitute(_("Would you also like to remove all contacts associated with %s?"), 
					    fromUTF8(_tree, nickname) + " (" + fromUTF8(_tree, jid) + ")");
	       d = manage(Gnome::Dialogs::question_modal(question,
							 bind(slot(this, &RosterView::handle_RemoveAgentDlg), jid)));
	       main_dialog(d);
	  }
	  else
	  {
	       G_App->getSession().roster().deleteUser(jid);
	  }
	  break;
     case 1:
          // Do nothing
          ;
     }
}

void RosterView::handle_RemoveAgentDlg(int code, string jid)
{
     Roster::iterator it = G_App->getSession().roster().begin();
     string agent = JID::getHost(jid);
     switch (code)
     {
     case 0:
          for ( ; it != G_App->getSession().roster().end(); it++)
          {
               // Check if the jid is from the agent
               if (agent == JID::getHost(it->getJID()))
               {
		    G_App->getSession().roster().deleteUser(it->getJID()); // Remove that user from the roster
               }
          }
	  break;
     case 1:
	  G_App->getSession().roster().deleteUser(jid);
     }
}

void RosterView::handle_RemoveAgent_iq(const judo::Element& iq)
{
     if (iq.cmpAttrib("type", "result"))
     {
	  const Element* query = iq.findElement("query");
	  if (query)
	  {
	       string key = query->getChildCData("key");
	       if (!key.empty())
	       {
		    // Unregister the agent
		    string id = G_App->getSession().getNextID();

		    Packet iq("iq");
		    iq.setID(id);
		    iq.setTo(iq.getBaseElement().getAttrib("from"));
		    iq.getBaseElement().putAttrib("type", "set");

		    Element* query = iq.getBaseElement().addElement("query");
		    query->putAttrib("xmlns", "jabber:iq:register");
		    query->addElement("key", key);
		    query->addElement("remove", "");
		    G_App->getSession() << iq;
	       }
	  }
     }
}

GdkColor RosterView::get_style_bg_color()
{
     GtkStyle* gs = gtk_widget_get_style(GTK_WIDGET(_tree->gtkobj()));
     return gs->bg[GTK_STATE_NORMAL];
}

GdkColor RosterView::get_style_fg_color()
{
     GtkStyle* gs = gtk_widget_get_style(GTK_WIDGET(_tree->gtkobj()));
     return gs->fg[GTK_STATE_NORMAL];
}

void RosterView::addNILItem(Roster::Item& r)
{
     list<Roster::Item>::iterator it = _nil_roster.begin();
     string user = JID::getUserHost(r.getJID());
     while ((it != _nil_roster.end()) && (JID::getUserHost(it->getJID()) != user))
          it++;

     if (it == _nil_roster.end())
     {
          _nil_roster.push_back(r);
          refresh();
     }
}

void RosterView::clear_event(const string& jid)
{
     list<Roster::Item>::iterator it = _nil_roster.begin();
     string user = JID::getUserHost(jid);
     // Find JID in the NIL roster and remove them
     while ((it != _nil_roster.end()) && user != JID::getUserHost(it->getJID()))
	  it++;

     if (it != _nil_roster.end())
     {
	  _nil_roster.erase(it);
     }
     // refresh regardless of whether we removed an NIL item to make sure
     // the icons get reset properly.  
     refresh();
}

gint RosterView::FlashEvents()
{
     ConfigManager& cfgm = G_App->getCfg();
     MessageManager& mm = G_App->getMessageManager();
     bool showIcons = cfgm.colors.icons;

     MessageManager::EventList& events = G_App->getMessageManager().getEvents();
     // Iterate backwards so that older events (ones closer to the front of the list) are the ones displayed
     MessageManager::EventList::reverse_iterator it = events.rbegin();
     for (; it != events.rend(); it++)
     {
          string user = JID::getUserHost(it->first);
          pair<NodeMap::iterator, NodeMap::iterator> p = USERS.equal_range(user);
          if (p.first == p.second)
               continue;

          string nickname;
          try {
               nickname = fromUTF8(_tree, G_App->getSession().roster()[user].getNickname());
          } catch (Roster::XCP_InvalidJID& e)
          {
	       // cerr << "jid " << user << " not found in roster" << endl;
               list<Roster::Item>::iterator lit = _nil_roster.begin();
               while ((lit != _nil_roster.end()) && (JID::getUserHost(lit->getJID()) != user))
                    lit++;
               if (lit == _nil_roster.end())
	       {
		    // cerr << "jid " << user << " not found" << endl;
                    continue;
	       }
               nickname = fromUTF8(_tree, lit->getNickname());
          }

	  for (NodeMap::iterator uit = p.first; uit != p.second; uit++)
	  {
	       GtkCTreeNode* n = uit->second;
	       GtkCTreeNode* parent = GTK_CTREE_ROW(uit->second)->parent;

	       // get rid of the node shift so the text is in the right place
	       gtk_ctree_node_set_shift(_tree->gtkobj(), n, colNickname, 0, 0);
	       if (_event_flash)
	       {
		    // Change the color of the group node if it is collapsed
		    if (!GTK_CTREE_ROW(parent)->expanded)
			 gtk_ctree_node_set_foreground(_tree->gtkobj(), parent, mm.getEventColor(it->second));
		    else
		    {
			 // Otherwise flash the event
			 gtk_ctree_node_set_foreground(_tree->gtkobj(), n, mm.getEventColor(it->second));
			 if (showIcons)
			      gtk_ctree_node_set_pixtext(_tree->gtkobj(), n, colNickname, nickname.c_str(), 5, mm.getEventPixmap(it->second), mm.getEventBitmap(it->second));
		    }
	       }
	       else
	       {
		    GdkColor fg = get_style_fg_color();
		    gtk_ctree_node_set_foreground(_tree->gtkobj(), parent, &fg);

		    string show;
		    try {
			 show = _session.presenceDB().find(it->first)->getShow_str();
		    } catch (PresenceDB::XCP_InvalidJID& e) {
			 // JID isn't in roster so use the online color
			 show = "online";
		    }
		    gtk_ctree_node_set_foreground(_tree->gtkobj(), n, cfgm.getPresenceColor(show));
		    if (showIcons)
			 gtk_ctree_node_set_pixtext(_tree->gtkobj(), n, colNickname, nickname.c_str(), 5, mm.getEventPixmap(MessageManager::error_mtype), mm.getEventBitmap(MessageManager::error_mtype));
	       }
	  }
     }

     _event_flash = !_event_flash;
     return TRUE;
}

void RosterView::on_update_refresh()
{
     // This is called when we first receive the roster from the server (as well as a few other times)
     // We need to make sure all users not in the roster but who have spooled Message events are added
     // to the not in list group of the roster
     MessageManager::EventList& events = G_App->getMessageManager().getEvents();
     for (MessageManager::EventList::iterator it = events.begin(); it != events.end(); it++)
     {
          try {
               G_App->getSession().roster()[JID::getUserHost(it->first)];
          } catch(Roster::XCP_InvalidJID& e) {
               // User isn't in roster, add it to the NIL group
               Roster::Item t(it->first, JID::getUser(it->first));
               t.addToGroup(toUTF8(_tree, _("Not in Roster")));
               addNILItem(t);
          }
     }
     refresh();
}

void RosterView::on_tree_move(GtkCTree* ctree, GtkCTreeNode* node, GtkCTreeNode* new_parent, GtkCTreeNode* new_sibling, gpointer data)
{
     RosterView* roster = (RosterView *) data;
     Gtk::CTree::Row row(ctree, node);
     Gtk::CTree::Row new_parent_row(ctree, new_parent);
     Gtk::CTree::Row old_parent_row(ctree, GTK_CTREE_ROW(node)->parent);

     // Make changes to the roster item for the row being moved
     try {

          Roster::Item item = roster->_session.roster()[row[1].get_text()];
	  // remove from old group
	  item.delFromGroup(old_parent_row[1].get_text());
	  // add item to new group
	  item.addToGroup(new_parent_row[1].get_text());
	  // send off the modified roster item to the server
	  roster->_session.roster() << item;
     } catch(Roster::XCP_InvalidJID& e) {
	  cerr << "item not in roster! Oh horror of horrors, this should never happen!" << endl;
     }
}

void RosterView::on_drag_data_get(GdkDragContext* drag_ctx, GtkSelectionData* data, guint info, guint time)
{
     if (data->target == gdk_atom_intern("text/x-jabber-id", FALSE))
     {
	  // Got this out of the gtkclist drag_data_get function, should work
	  GtkCListCellInfo* info;

	  info = (GtkCListCellInfo*) g_dataset_get_data(drag_ctx, "gtk-clist-drag-source");
	  string dnddata;
	  // Handle groups
	  if (!_tree->row(info->row).is_leaf())
	  {
	       // This code doesn't work right for some reason, not sure why.  Using
#if 0
	       const Gtk::CTree::RowList& rl = _tree->row(info->row).subtree();
	       for (Gtk::CTree::RowList::const_iterator it = rl.begin(); it != rl.end(); it++)
	       {
		    string jid = (*it)[1].get_text();
		    cerr << "adding jid " << jid << endl;
		    dnddata += "jabber:" + jid + "\n";
	       }
#endif
	       // Using just the gtk+ stuff instead
	       NodeMap::iterator it = GROUPS.find(_tree->row(info->row)[colJID].get_text());
	       if (it == GROUPS.end())
	       {
		    // should never happane
		    return;
	       }
	       GtkCTreeRow* parent = GTK_CTREE_ROW(it->second);
	       for (GtkCTreeNode* node = parent->children; node && GTK_CTREE_ROW(node)->is_leaf; node = GTK_CTREE_NODE_NEXT(node))
	       {
		    Gtk::CTree::Row r(_tree->gtkobj(), node);
		    string jid = r[colJID].get_text();
		    dnddata += "jabber:" + jid + "\n";
	       }
	  }
	  else
	  {
	       string jid = _tree->row(info->row)[1].get_text();
	       dnddata = "jabber:" + jid + "\n";
	  }
	  gtk_selection_data_set(data, data->target, 8, (guchar *) dnddata.c_str(), dnddata.size() + 1);
     }
     else if (data->target == gdk_atom_intern("text/x-jabber-roster-item", FALSE))
     {
	  GtkCListCellInfo* info;

	  string dnddata;
	  info = (GtkCListCellInfo*) g_dataset_get_data(drag_ctx, "gtk-clist-drag-source");
	  if (!_tree->row(info->row).is_leaf())
	  {
	       const Gtk::CTree::RowList& rl = _tree->row(info->row).subtree();
               for (Gtk::CTree::RowList::const_iterator it = rl.begin(); it != rl.end(); it++)
               {
                    string jid = (*it)[1].get_text();
		    try {
			 Roster::Item item = G_App->getSession().roster()[jid];

			 // Convert roster item to a tag
			 Element itemt("item");
			 itemt.putAttrib("jid", item.getJID());
			 if (!item.getNickname().empty())
			      itemt.putAttrib("name", item.getNickname());
			 // Do we want to put all the groups in here?
			 for (Roster::Item::iterator it = item.begin(); it != item.end(); it++)
			      itemt.addElement("group", *it);

			 // Set the DnD Data
			 dnddata += itemt.toString();
		    } catch (Roster::XCP_InvalidJID& e) { }
               }
	  }
	  else
	  {
	       string jid = _tree->row(info->row)[1].get_text();
	       try {
		    Roster::Item item = G_App->getSession().roster()[jid];

		    // Convert roster item to a tag
		    Element itemt("item");
		    itemt.putAttrib("jid", item.getJID());
		    if (!item.getNickname().empty())
			 itemt.putAttrib("name", item.getNickname());
		    for (Roster::Item::iterator it = item.begin(); it != item.end(); it++)
			 itemt.addElement("group", *it);

		    // Set the DnD Data
		    dnddata = itemt.toString();
	       } catch (Roster::XCP_InvalidJID& e) { }
	  }
	  gtk_selection_data_set(data, data->target, 8, (guchar *) dnddata.c_str(), dnddata.size() + 1);
     }
     else if (data->target == gdk_atom_intern("text/x-vcard-xml", FALSE))
     {
	  // TODO: HANDLE Groups??
	  if (_dnd_data)
	  {
	       // already waiting for a DnD vCard request, don't start another
	       return;
	  }

	  GtkCListCellInfo* info;

	  info = (GtkCListCellInfo*) g_dataset_get_data(drag_ctx, "gtk-clist-drag-source");
	  string jid = _tree->row(info->row)[1].get_text();

	  // Get next session ID
	  string id = G_App->getSession().getNextID();

	  // create request for user's vcard
	  Packet iq("iq");
	  iq.setID(id);
	  iq.setTo(jid);
	  iq.getBaseElement().putAttrib("type", "get");
	  Element* vCard = iq.getBaseElement().addElement("vCard");
	  vCard->putAttrib("xmlns", "vcard-temp");
	  vCard->putAttrib("version", "2.0");
	  vCard->putAttrib("prodid", "-//HandGen//NONSGML vGen v1.0//EN");

	  _dnd_data = data;

	  // Send the vCard request
	  G_App->getSession() << iq;
	  G_App->getSession().registerIQ(id, slot(this, &RosterView::dnd_get_vCard));
     }
     else if (data->target == gdk_atom_intern("text/x-vcard", FALSE))
     {
	  if (_dnd_data)
	  {
	       // already waiting for a DnD vCard request, don't start another
	       return;
	  }

// FIXME: Need to convert vcard XML to normal vcard
// It supposedly shouldn't be that hard

	  GtkCListCellInfo* info;

	  info = (GtkCListCellInfo*) g_dataset_get_data(drag_ctx, "gtk-clist-drag-source");
	  string jid = _tree->row(info->row)[1].get_text();

	  // Get next session ID
	  string id = G_App->getSession().getNextID();

	  // create request for user's vcard
	  Packet iq("iq");
	  iq.setID(id);
	  iq.setTo(jid);
	  iq.getBaseElement().putAttrib("type", "get");
	  Element* vCard = iq.getBaseElement().addElement("vCard");
	  vCard->putAttrib("xmlns", "vcard-temp");
	  vCard->putAttrib("version", "2.0");
	  vCard->putAttrib("prodid", "-//HandGen//NONSGML vGen v1.0//EN");

	  _dnd_data = data;

	  // Send the vCard request
	  G_App->getSession() << iq;
	  G_App->getSession().registerIQ(id, slot(this, &RosterView::dnd_get_vCard));
     }
}

void RosterView::on_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time)
{
     GtkCListDestInfo *dest_info;
     Gtk::CTree::Row dest_row;

     dest_info = (GtkCListDestInfo *) g_dataset_get_data(drag_ctx, "gtk-clist-drag-dest");
     if (dest_info)
     {
          if (_tree->row(dest_info->cell.row).get_parent() != (gpointer) 0)
               dest_row = _tree->row(dest_info->cell.row).get_parent();
          else
               dest_row = _tree->row(dest_info->cell.row);
          if (dest_row[colJID].get_text() == "Agents" ||
              dest_row[colJID].get_text() == "Not in Roster" ||
	      dest_row[colJID].get_text() == "Unfiled")
	       dest_row = Gtk::CTree::Row();
     }

     if (data->target == gdk_atom_intern("text/x-jabber-id", FALSE))
     {
	  for (char *line = strtok((char *) data->data, "\n"); line; line = strtok(NULL, "\n"))
	  {
	       char *p = strchr(line, ':');
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

	       Roster::Item item(jid, JID::getUser(jid));
	       if (dest_row != (gpointer) 0)
		    item.addToGroup(dest_row[colJID].get_text());
	       else
		    item.addToGroup("Unfiled");
	       _session.roster() << item;
	  }
	  refresh();
     }
     // Check for vcard, xml vcard, etc, and do appropriate stuff
}

gboolean RosterView::is_drop_ok(GtkCTree* ctree, GtkCTreeNode* node, GtkCTreeNode* new_parent, GtkCTreeNode* new_sibling)
{
     // Prevent roster items from being moved to the toplevel of the ctree
     if (!new_parent)
	  return FALSE;
     // Prevent groups from being moved around
     if (!GTK_CTREE_ROW(node)->parent)
	  return FALSE;
     // Setup CTree rows to make getting text easier
     Gtk::CTree::Row new_parent_row(ctree, new_parent);
     Gtk::CTree::Row old_parent_row(ctree, GTK_CTREE_ROW(node)->parent);
  
     if (new_parent_row[colJID].get_text() == "Agents" || new_parent_row[colJID].get_text() == "Not in Roster" || 
	 new_parent_row[colJID].get_text() == "Pending" || new_parent_row[colJID].get_text().empty())
          return FALSE;
     if (old_parent_row[colJID].get_text() == "Agents" || old_parent_row[colJID].get_text() == "Not in Roster" ||
	 old_parent_row[colJID].get_text() == "Pending")
          return FALSE;

     return TRUE;
}

void RosterView::dnd_get_vCard(const Element& iq)
{
     const Element* vCard = iq.findElement("vCard");

     if (vCard)
     {
	  string dnddata = vCard->toString();
          gtk_selection_data_set(_dnd_data, _dnd_data->target, 8, (guchar *) dnddata.c_str(), dnddata.size() + 1);
	  _dnd_data = NULL;
     }
}

void RosterView::on_drag_begin(GdkDragContext* drag_ctx)
{
     GtkCListCellInfo* info;
     gint row;

     info = (GtkCListCellInfo*) g_dataset_get_data(drag_ctx, "gtk-clist-drag-source");
     if (info)
          row = info->row;
     else
          row = GTK_CLIST(_tree->gtkobj())->click_cell.row;
     string jid = _tree->row(row)[colJID].get_text();
     if (GROUPS.find(jid) != GROUPS.end())
     {
          // The node is a group so set the group icon
	  if (!group_pixmap_loaded)
	  {
	       // Build a table of possible image locations
	       const char* pixpath_tbl[4] = { ConfigManager::get_GLADEDIR(), "./", "./pixmaps/", "../pixmaps/" };
	       // Look for and load the pixmap for the specified status...if it exists
	       for (int i = 0; i < 4; i++)
	       {
		    string filename = string(pixpath_tbl[i]) + "glade-group.xpm";
		    // If we find a pixmap by this name, load it and return
		    if (g_file_exists(filename.c_str()))
		    {
			 group_drag_pixmap.create_from_xpm(_tree->get_window(), group_drag_bitmap, Gdk_Color("white"), filename);
			 group_pixmap_loaded = true;
		    }
	       }
	  }
          gtk_drag_set_icon_pixmap(drag_ctx, _tree->get_colormap(), group_drag_pixmap, group_drag_bitmap, -2, -2);
     }
     else
     {
          if (!G_App->getSession().presenceDB().available(jid))
          {
	       if (!offline_pixmap_loaded)
	       {
		    // Build a table of possible image locations
		    const char* pixpath_tbl[4] = { ConfigManager::get_PIXPATH(), "./", "./pixmaps/", "../pixmaps/" };
		    // Look for and load the pixmap for the specified status...if it exists
		    for (int i = 0; i < 4; i++)
		    {
			 string filename = string(pixpath_tbl[i]) + "offline.xpm";
			 // If we find a pixmap by this name, load it and return
			 if (g_file_exists(filename.c_str()))
			 {
			      offline_drag_pixmap.create_from_xpm(_tree->get_window(), offline_drag_bitmap, Gdk_Color("white"), filename);
			      offline_pixmap_loaded = true;
			      break;
			 }
		    }
	       }
               gtk_drag_set_icon_pixmap(drag_ctx, _tree->get_colormap(), offline_drag_pixmap, offline_drag_bitmap, -2, -2);
          }
	  else
	  {
	       // set icon to the row icon since it should be set properly
	       GtkCTreeNode *node;

	       node = GTK_CTREE_NODE(g_list_nth(GTK_CLIST(_tree->gtkobj())->row_list, row));
	       if (node)
               {
		    gtk_drag_set_icon_pixmap(drag_ctx, gtk_widget_get_colormap(GTK_WIDGET(_tree->gtkobj())),
					     GTK_CELL_PIXTEXT(GTK_CTREE_ROW(node)->row.cell[_tree->gtkobj()->tree_column])->pixmap,
					     GTK_CELL_PIXTEXT(GTK_CTREE_ROW(node)->row.cell[_tree->gtkobj()->tree_column])->mask,
					     -2, -2);
		    return;
	       }
	  }
     }
}

void RosterView::add_temp_group(string& groupname)
{
     for (list<string>::iterator it = _temp_groups.begin(); it != _temp_groups.end(); it++)
	  if (*it == groupname)
	       return;

     _temp_groups.push_back(groupname);
}

///////////////////////////////////////////
// Stuff for SimpleRosterView
///////////////////////////////////////////

SimpleRosterView::SimpleRosterView(Gtk::CTree* tree, Gtk::ScrolledWindow* tscroll, Roster::ItemMap& roster, bool groups, string dgrp)
     : _tree(tree), _tscroll(tscroll), _roster(roster), _defaultgroup(dgrp), _usegroups(groups), _usenick(true)
{
     _colNickname = 0;
     _colJID = 1;

     // Trick up tree
     _tree->set_line_style(GTK_CTREE_LINES_NONE);
     _tree->set_expander_style(GTK_CTREE_EXPANDER_TRIANGLE);
     _tree->set_column_visibility(_colJID, false);
     if (((_tree->get_style()->get_font().ascent() + _tree->get_style()->get_font().descent() + 1) < 17) &&
	 (G_App->getCfg().colors.icons))
	  _tree->set_row_height(17);                                        // Hack for making sure icons aren't chopped off
     _tree->column_titles_hide();
     _tree->set_compare_func(&GabberUtil::strcasecmp_clist_items);
     _tree->set_column_auto_resize(_colNickname, true);
     _tree->set_auto_sort(true);
     _tree->set_indent(0);    // Set the indent to 0 so that the tree is more compact

     // Do we want this in SimpleRoster??
     // _tree->button_press_event.connect(slot(this, &RosterView::on_button_press));

     // Connect tree-move signal
     gtk_signal_connect(GTK_OBJECT(_tree->gtkobj()), "tree-move", GTK_SIGNAL_FUNC(&SimpleRosterView::on_tree_move), (gpointer) this);
     // allow the roster to be dragged around
     // _tree->set_reorderable(true);
     // _tree->set_drag_compare_func(&SimpleRosterView::is_drop_ok);
     // Setup DnD targets that the roster can receive
     GtkTargetEntry roster_dest_targets[] = {
      {"text/x-jabber-id", 0, 0},
/*      {"gtk-clist-drag-reorder", GTK_TARGET_SAME_WIDGET, 0}, */
     };
     int dest_num = sizeof(roster_dest_targets) / sizeof(GtkTargetEntry);
     // Detup DnD targets that the roster can send 
     GtkTargetEntry roster_source_targets[] = {
      {"text/x-jabber-roster-item", 0, 0},
      {"text/x-vcard", 0, 0},
      {"text/x-vcard-xml", 0, 0},
      {"text/x-jabber-id", 0, 0},
      {"gtk-clist-drag-reorder", GTK_TARGET_SAME_WIDGET, 0},
     };
     int source_num = sizeof(roster_source_targets) / sizeof(GtkTargetEntry);
     // need to call set_reorderable on the tree first so that the ctree's flag gets
     // set but it sets targets which we have to remove before we can set new targets
     gtk_drag_dest_unset(GTK_WIDGET(_tree->gtkobj()));

     gtk_drag_dest_set(GTK_WIDGET(_tree->gtkobj()), GTK_DEST_DEFAULT_ALL, roster_dest_targets, dest_num, (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE));
     gtk_drag_source_set(GTK_WIDGET(_tree->gtkobj()), (GdkModifierType) (GDK_BUTTON1_MASK | GDK_BUTTON3_MASK), roster_source_targets, source_num, (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE));
     // Setup DnD callbacks
     _tree->drag_data_get.connect(slot(this, &SimpleRosterView::on_drag_data_get));
     _tree->drag_data_received.connect(slot(this, &SimpleRosterView::on_drag_data_received));
     _tree->drag_motion.connect(slot(this, &SimpleRosterView::on_drag_data_motion));
     _tree->drag_begin.connect(slot(this, &SimpleRosterView::on_drag_begin));
}

SimpleRosterView::~SimpleRosterView()
{
}

void SimpleRosterView::push_drag_data(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time)
{
     // Public way to simply push drag data... 
     // possibly dragged to some other widget, but we want to push it here
     on_drag_data_received(drag_ctx, x, y, data, info, time);
}

void SimpleRosterView::add_jid(const string& jid, const string& nickname)
{
     Roster::Item item(jid, nickname);
     item.addToGroup(_defaultgroup);
     _roster.insert(make_pair(JID::getUserHost(jid), item));
     refresh();
}

void SimpleRosterView::refresh()
{
     // Prep the tree
     _tree->freeze();
     float hscroll, vscroll;
     hscroll = _tscroll->get_hadjustment()->get_value(); // Save the horizontal scroll position
     vscroll = _tscroll->get_vadjustment()->get_value(); // Save the vertical scroll position
     _tree->clear();

     // Clear out the group map
     _GROUPS.clear();

     // Render each item in the roster
     for(Roster::ItemMap::iterator i = _roster.begin(); i != _roster.end(); i++)
     {
	  if (_usegroups)
	  {
	       for (Roster::Item::iterator grp_it = i->second.begin(); grp_it != i->second.end(); grp_it++)
	       {
		    // Retrieve/create the group node
		    GtkCTreeNode* groupnode = get_group_node(*grp_it);
		    // Create the item node on this group
		    create_item_node(groupnode, i->second);
	       }
	  }
	  else
	  {
	       create_item_node(NULL, i->second);
	  }
     }

     // Reset the scroll position
     _tscroll->get_hadjustment()->set_value(hscroll); // Set the horizontal scroll position
     _tscroll->get_vadjustment()->set_value(vscroll); // Set the horizontal scroll position

     _tree->thaw();
}

GtkCTreeNode* SimpleRosterView::get_group_node(const string& groupid)
{
     GtkCTreeNode* result = NULL;

     // Lookup this id
     NodeMap::iterator it = _GROUPS.find(groupid);

     // If no such group is found, create one and return it
     if (it == _GROUPS.end())
     {
          // Create the node on the tree
	  if (_showjid)
	  {
	       char* header[3] = { (char*)fromUTF8(_tree, groupid).c_str(), (char*)"", (char*)groupid.c_str() };
	       result = gtk_ctree_insert_node(_tree->gtkobj(), NULL, NULL, header, 0,
					      NULL, NULL, NULL, NULL, false, true);
	  }
	  else
	  {
	       char* header[2] = { (char*)fromUTF8(_tree, groupid).c_str(), (char*)""};
	       result = gtk_ctree_insert_node(_tree->gtkobj(), NULL, NULL, header, 0,
					      NULL, NULL, NULL, NULL, false, true);
	  }

          // Set group foreground/background colours
          GdkColor bg = get_style_bg_color();
          gtk_ctree_node_set_background(_tree->gtkobj(), result, &bg);
          GdkColor fg = get_style_fg_color();
          gtk_ctree_node_set_foreground(_tree->gtkobj(), result, &fg);

          // Ensure the group is not selectable
          gtk_ctree_node_set_selectable(_tree->gtkobj(), result, false);

          // Insert the node into the map for future reference
          _GROUPS.insert(make_pair(groupid, result));
     }
     // Otherwise return the node
     else
          result = it->second;

     return result;
}

GtkCTreeNode* SimpleRosterView::create_item_node(GtkCTreeNode* groupnode, const Roster::Item& r)
{
     // Create the node on the tree
     string nickname;
     if (_usenick)
	  nickname = fromUTF8(_tree, r.getNickname());
     else
	  nickname = fromUTF8(_tree, r.getJID());
     GtkCTreeNode* n;
     if (_showjid)
     {
	  string jid = fromUTF8(_tree, JID::getUserHost(r.getJID()));
	  char* header[3] = { (char*)nickname.c_str(), (char*)jid.c_str(), (char*)r.getJID().c_str() };
	  n = gtk_ctree_insert_node(_tree->gtkobj(), groupnode, NULL, header, 0,
				    NULL, NULL, NULL, NULL, true, true);
     }
     else
     {
	  char* header[2] = { (char*)nickname.c_str(), (char*)r.getJID().c_str() };
	  n = gtk_ctree_insert_node(_tree->gtkobj(), groupnode, NULL, header, 0,
                                    NULL, NULL, NULL, NULL, true, true);
     }
     return n;
}

void SimpleRosterView::clear()
{
     _tree->clear();
}

GdkColor SimpleRosterView::get_style_bg_color()
{
     GtkStyle* gs = gtk_widget_get_style(GTK_WIDGET(_tree->gtkobj()));
     return gs->bg[GTK_STATE_NORMAL];
}

GdkColor SimpleRosterView::get_style_fg_color()
{
     GtkStyle* gs = gtk_widget_get_style(GTK_WIDGET(_tree->gtkobj()));
     return gs->fg[GTK_STATE_NORMAL];
}

void SimpleRosterView::on_tree_move(GtkCTree *ctree, GtkCTreeNode *node, 
				    GtkCTreeNode *new_parent, 
				    GtkCTreeNode *new_sibling, gpointer data)
{
     SimpleRosterView* roster = (SimpleRosterView *) data;
     Gtk::CTree::Row row(ctree, node);
     Gtk::CTree::Row new_parent_row(ctree, new_parent);
     Gtk::CTree::Row old_parent_row(ctree, GTK_CTREE_ROW(node)->parent);

     // Make changes to the roster item for the row being moved
     Roster::ItemMap::iterator it = roster->_roster.find(row[roster->_colJID].get_text());
     if (it != roster->_roster.end())
     {
          // remove from old group
          it->second.delFromGroup(old_parent_row[roster->_colJID].get_text());
          // add item to new group
          it->second.addToGroup(new_parent_row[roster->_colJID].get_text());
     }
     else
     {
          cerr << "item not in roster! Oh horror of horrors, this should never happen!" << endl;
     }
}

void SimpleRosterView::on_drag_data_get(GdkDragContext* drag_ctx,
					GtkSelectionData* data,
					guint info, guint time)
{
     if (data->target == gdk_atom_intern("text/x-jabber-id", FALSE))
     {
	  // Got this out of the gtkclist drag_data_get function, should work
	  GtkCListCellInfo* info;

          string dnddata;
	  info = (GtkCListCellInfo*) g_dataset_get_data(drag_ctx, "gtk-clist-drag-source");
          // Handle groups
          if (!_tree->row(info->row).is_leaf())
          {
#if 0
               const Gtk::CTree::RowList& rl = _tree->row(info->row).subtree();
	       for (Gtk::CTree::RowList::const_iterator it = rl.begin(); it != rl.end(); it++)
               {
                    string jid = (*it)[_colJID].get_text();
                    dnddata += "jabber:" + jid + "\n";
               }
#endif
	       // Using just the gtk+ stuff instead
               NodeMap::iterator it = GROUPS.find(_tree->row(info->row)[_colJID].get_text());
               if (it == GROUPS.end())
               {
                    // should never happen
                    return;
               }
               GtkCTreeRow* parent = GTK_CTREE_ROW(it->second);
               for (GtkCTreeNode* node = parent->children; node && GTK_CTREE_ROW(node)->is_leaf; node = GTK_CTREE_NODE_NEXT(node))
	       {
                    Gtk::CTree::Row r(_tree->gtkobj(), node);
                    string jid = r[_colJID].get_text();
                    dnddata += "jabber:" + jid + "\n";
               }
          }
          else
          {
               string jid = _tree->row(info->row)[_colJID].get_text();
               dnddata = "jabber:" + jid + "\n";
          }
          gtk_selection_data_set(data, data->target, 8, (guchar *) dnddata.c_str(), dnddata.size() + 1);
     }
     else if (data->target == gdk_atom_intern("text/x-jabber-roster-item", FALSE))
     {
          GtkCListCellInfo* info;

          string dnddata;
          info = (GtkCListCellInfo*) g_dataset_get_data(drag_ctx, "gtk-clist-drag-source");
          if (!_tree->row(info->row).is_leaf())
          {
               const Gtk::CTree::RowList& rl = _tree->row(info->row).subtree();
               for (Gtk::CTree::RowList::const_iterator it = rl.begin(); it != rl.end(); it++)
               {
                    string jid = (*it)[1].get_text();
		    Roster::ItemMap::iterator it = _roster.find(jid);
		    if (it == _roster.end())
		    {
			 // This shouldn't happen...
			 continue;
		    }

                    // Convert roster item to a tag
		    Element itemt("item");
                    itemt.putAttrib("jid", it->second.getJID());
                    if (!it->second.getNickname().empty())
                         itemt.putAttrib("name", it->second.getNickname());
                    // Do we want to put all the groups in here?
                    for (Roster::Item::iterator rit = it->second.begin(); rit != it->second.end(); rit++)
                         itemt.addElement("group", *rit);
		    dnddata += itemt.toString();
	       }
	  }
	  else
          {
               string jid = _tree->row(info->row)[1].get_text();
               Roster::ItemMap::iterator it = _roster.find(jid);

               // Convert roster item to a tag
               Element itemt("item");
               itemt.putAttrib("jid", it->second.getJID());
               if (!it->second.getNickname().empty())
                    itemt.putAttrib("name", it->second.getNickname());
               for (Roster::Item::iterator rit = it->second.begin(); rit != it->second.end(); rit++)
                    itemt.addElement("group", *rit);

               // Set the DnD Data
               dnddata = itemt.toString();
          }
     }
     else if (data->target == gdk_atom_intern("text/x-vcard-xml", FALSE))
     {
	  // TODO: dragging groups?
          if (_dnd_data)
          {
               // already waiting for a DnD vCard request, don't start another
               return;
          }

          GtkCListCellInfo* info;

          info = (GtkCListCellInfo*) g_dataset_get_data(drag_ctx, "gtk-clist-drag-source");
          string jid = _tree->row(info->row)[_colJID].get_text();

          // Get next session ID
          string id = G_App->getSession().getNextID();

          // create request for user's vcard
          Packet iq("iq");
          iq.setID(id);
          iq.setTo(jid);
          iq.getBaseElement().putAttrib("type", "get");
          Element* vCard = iq.getBaseElement().addElement("vCard");
          vCard->putAttrib("xmlns", "vcard-temp");
          vCard->putAttrib("version", "2.0");
          vCard->putAttrib("prodid", "-//HandGen//NONSGML vGen v1.0//EN");

          _dnd_data = data;

          // Send the vCard request
          G_App->getSession() << iq;
          G_App->getSession().registerIQ(id, slot(this, &SimpleRosterView::dnd_get_vCard));
     }
     else if (data->target == gdk_atom_intern("text/x-vcard", FALSE))
     {
          if (_dnd_data)
          {
	       // already waiting for a DnD vCard request, don't start another
	       return;
          }

         // FIXME: Need to convert vcard XML to normal vcard
         // It supposedly shouldn't be that hard

         GtkCListCellInfo* info;

         info = (GtkCListCellInfo*) g_dataset_get_data(drag_ctx, "gtk-clist-drag-source");
         string jid = _tree->row(info->row)[_colJID].get_text();

         // Get next session ID
         string id = G_App->getSession().getNextID();

         // create request for user's vcard
         Packet iq("iq");
         iq.setID(id);
         iq.setTo(jid);
         iq.getBaseElement().putAttrib("type", "get");
	 Element* vCard = iq.getBaseElement().addElement("vCard");
	 vCard->putAttrib("xmlns", "vcard-temp");
	 vCard->putAttrib("version", "2.0");
	 vCard->putAttrib("prodid", "-//HandGen//NONSGML vGen v1.0//EN");

         _dnd_data = data;

         // Send the vCard request
         G_App->getSession() << iq;
         G_App->getSession().registerIQ(id, slot(this, &SimpleRosterView::dnd_get_vCard));
     }
}

void SimpleRosterView::on_drag_data_received(GdkDragContext* drag_ctx,
					     gint x, gint y,
					     GtkSelectionData* data,
					     guint info, guint time)
{
     GtkCListDestInfo *dest_info;
     Gtk::CTree::Row dest_row;

     dest_info = (GtkCListDestInfo *) g_dataset_get_data(drag_ctx, "gtk-clist-drag-dest");
     if (dest_info)
     {
          if (_tree->row(dest_info->cell.row).get_parent() != (gpointer) 0)
	       dest_row = _tree->row(dest_info->cell.row).get_parent();
	  else
	       dest_row = _tree->row(dest_info->cell.row);
	  if (dest_row[_colJID].get_text() == "Agents" || 
	      dest_row[_colJID].get_text() == "Not in Roster" ||
	      dest_row[_colJID].get_text() == "Pending")
	       dest_row = Gtk::CTree::Row();
     }

     if (data->target == gdk_atom_intern("text/x-jabber-id", FALSE))
     {
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

	       // Get the nickname
	       string nickname = JID::getUser(jid);
	       try {
		    nickname = G_App->getSession().roster()[JID::getUserHost(jid)].getNickname();
	       } catch (Roster::XCP_InvalidJID& e) {
		    // Special handling for a groupchat ID -- use the resource as the nickname
		    if (G_App->isGroupChatID(jid))
			 nickname = JID::getResource(jid);
	       }

	       Roster::Item item(jid, nickname);
	       if (dest_row != (gpointer) 0)
		    item.addToGroup(dest_row[_colJID].get_text());
	       else
		    item.addToGroup(_defaultgroup);
	       _roster.insert(make_pair(JID::getUserHost(jid), item));
	       refresh();
	  }
     }
}

void SimpleRosterView::dnd_get_vCard(const Element& iq)
{
     const Element* vCard = iq.findElement("vCard");

     if (vCard)
     {
          string dnddata = vCard->toString();
          gtk_selection_data_set(_dnd_data, _dnd_data->target, 8, (guchar *) dnddata.c_str(), dnddata.size() + 1);
          _dnd_data = NULL;
     }
}

void SimpleRosterView::show_jid()
{
     _showjid = true;
     _tree->column_titles_show();
     _colJID++;
     _tree->set_column_visibility(1, true);
     _tree->set_column_visibility(_colJID, false);
     refresh();
}

void SimpleRosterView::ignore_nick()
{
     _usenick = false;
     refresh();
}

gboolean SimpleRosterView::on_drag_data_motion(GdkDragContext* drag_ctx, gint x, gint y, guint time)
{
     // what we return doesn't matter because we're using the default motion functions to do
     // the readl work.  
     if (gtk_drag_get_source_widget(drag_ctx) == GTK_WIDGET(_tree->gtkobj()))
     {
	  gdk_drag_status(drag_ctx, (GdkDragAction) 0, time);
	  return TRUE;
     }
     return TRUE;
}

void SimpleRosterView::on_drag_begin(GdkDragContext* drag_ctx)
{
     GtkCListCellInfo* info;
     gint row;

     info = (GtkCListCellInfo*) g_dataset_get_data(drag_ctx, "gtk-clist-drag-source");
     if (info)
	  row = info->row;
     else
	  row = GTK_CLIST(_tree->gtkobj())->click_cell.row;
     string jid = _tree->row(row)[_colJID].get_text();
     if (_GROUPS.find(jid) != _GROUPS.end())
     {
	  // The node is a group so set the group icon
	  // gtk_drag_set_icon_pixmap(drag_ctx, 
     }
     else
     {
	  if (!offline_pixmap_loaded)
	  {
	       // Build a table of possible image locations
	       const char* pixpath_tbl[4] = { ConfigManager::get_PIXPATH(), "./", "./pixmaps/", "../pixmaps/" };
	       // Look for and load the pixmap for the specified status...if it exists
	       for (int i = 0; i < 4; i++)
	       {
		    string filename = string(pixpath_tbl[i]) + "offline.xpm";
		    // If we find a pixmap by this name, load it and return
		    if (g_file_exists(filename.c_str()))
		    {
			 offline_drag_pixmap.create_from_xpm(_tree->get_window(), offline_drag_bitmap, Gdk_Color("white"), filename);
			 offline_pixmap_loaded = true;
		    }
	       }
               ConfigManager& cf = G_App->getCfg();
               gtk_drag_set_icon_pixmap(drag_ctx, _tree->get_colormap(), offline_drag_pixmap, offline_drag_bitmap, -2, -2);
	  }
     }
}
