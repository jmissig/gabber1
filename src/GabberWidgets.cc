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

#include "GabberWidgets.hh"

#include "ContactInterface.hh"
#include "GabberApp.hh"
#include "GabberUtility.hh"

#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>

using namespace jabberoo;
using namespace GabberUtil;

// PrettyJID - nice display of JabberID - draggable and all
PrettyJID::PrettyJID(const string& jid, const string& nickname, DisplayType dt, string::size_type display_limit, bool select_resource, bool select_jid)
     : _jid(jid), _nickname(nickname), _resource(JID::getResource(_jid)), _default_jid(_jid), _select_resource(select_resource), _select_jid(select_jid)
{
     // We're only *really* selecting a resource
     // - if this is not a group chat id
     if (_select_resource && G_App->isGroupChatID(_jid))
     {
	  _select_resource = false;
     }

     // If we're selecting a resource
     if (_select_resource)
     {
	  // _jid should only contain user@host
	  _jid = JID::getUserHost(_jid);
     }

     // Figure out if we need to lookup the nickname
     if (_nickname.empty())
     {
	  // Lookup nickname - default is the username
	  _nickname = JID::getUser(_jid);
	  try {
	       _nickname = G_App->getSession().roster()[JID::getUserHost(_jid)].getNickname();
	  } catch (Roster::XCP_InvalidJID& e) {
	       // Special handling for a groupchat ID -- use the resource as the nickname
	       if (G_App->isGroupChatID(_jid) && !_resource.empty())
		    _nickname = _resource;
	  }
     }

     // If this jid has no resource, we need to get the default resource
     if (_resource.empty())
     {
	  try {
	       // _default_jid contains the JID and the default resource
	       _default_jid = G_App->getSession().presenceDB().find(_jid)->getFrom();
	  } catch (PresenceDB::XCP_InvalidJID& e) {}
     }

     // Create the HBox
     _hboxPJ = manage(new Gtk::HBox(false, 4));
     add(*_hboxPJ);

     // Locate the pixmap
     // Build a table of possible image locations
     const char* pixpath_tbl[4] = { ConfigManager::get_PIXPATH(), "./", "./pixmaps/", "../pixmaps/" };
     // Look for and load the pixmap for the specified status...if it exists
     string filename = "offline.xpm";
     for (int i = 0; i < 4; i++)
     {
	  _pix_path = string(pixpath_tbl[i]);
	  filename = _pix_path + "offline.xpm";
	  // If we find a pixmap by this name, load it and return
	  if (g_file_exists(filename.c_str()))
	  {
	       break;
	  }
     }

     // Create the event box for XMMS music
     _evtMusic = manage(new Gtk::EventBox());
     _hboxPJ   ->pack_end(*_evtMusic, false, true, 0);
     _evtMusic ->hide();

     // Create the XMMS music info pixmap
     _pixMusic = manage(new Gnome::Pixmap(_pix_path + "xmms.xpm"));
     _evtMusic ->add(*_pixMusic);
     _pixMusic ->show();

     // Create the resource selector if needed
     if (_select_resource)
     {
	  // The resource selector combo
	  _cboResource = manage(new Gtk::Combo());
	  _cboResource ->set_usize(100, 0);
	  _hboxPJ      ->pack_end(*_cboResource, true, true, 0);
	  _cboResource ->show();

	  // The label between the jid and the resource selector
	  _lblResource  = manage(new Gtk::Label("/", 0.0, 0.5));
	  _hboxPJ       ->pack_end(*_lblResource, false, true, 0);
	  _lblResource  ->show();

	  // The list of resources
	  list<string> resources;
	  // If we have a resource, add it - it may not be among the list in presencedb
	  if (!_resource.empty())
	  {
	       resources.push_back(fromUTF8(_cboResource, _resource));
	       _cboResource->get_entry()->set_text(fromUTF8(_cboResource, _resource));
	  }
	  try {
	       // Walk the list of resources and add them to the combo box
	       PresenceDB::range r = G_App->getSession().presenceDB().equal_range(_jid);
	       for (PresenceDB::const_iterator it = r.first; it != r.second; it++)
	       {
		    const Presence& p = *it;

		    // If this presence is a NA presence, then skip it
		    if (p.getType() == Presence::ptUnavailable)
			 continue;

		    // Extract the resource
		    const string& res = JID::getResource(p.getFrom());
		    // Avoid resource duplication
		    if (res != _resource)
			 resources.push_back(fromUTF8(_cboResource, res));
	       }
	  } catch (PresenceDB::XCP_InvalidJID& e) {
	       // No presences from any resources
	  }

	  // Attach the list of resources to the combo
	  if (!resources.empty())
	       _cboResource->set_popdown_strings(resources);

	  // Hook up the changed event for the resource entry
	  // We do this *after* setting the resource because otherwise
	  // it would try to grab a presence and call on_presence etc
	  // before we've finished setting up the other widgets - 
	  // including the pixmap for status
	  // Plus, we're going to grab the presence at the end this ctor anyway
	  _cboResource->get_entry()->changed.connect(slot(this, &PrettyJID::on_entResource_changed));
     }

     // If we're selecting a JID
     if (_select_jid)
     {
	  // Create the entry for the jid
	  _entJID = manage(new Gtk::Entry());
	  _hboxPJ ->pack_end(*_entJID, true, true, 0);
	  _entJID ->set_text(fromUTF8(_entJID, _jid));
	  _entJID ->changed.connect(slot(this, &PrettyJID::on_entJID_changed));
	  _entJID ->show();
     }
     else
     {
	  // Create the label
	  _lblPJ  = manage(new Gtk::Label("", 0.0, 0.5));
	  _hboxPJ ->pack_end(*_lblPJ, !_select_resource, true, 0);
	  _lblPJ  ->show();
     }

     // Create the pixmap
     _pixPJ  = manage(new Gnome::Pixmap(filename));
     _hboxPJ ->pack_end(*_pixPJ, false, true, 0);
     _pixPJ  ->show();

     _hboxPJ ->show();

     // Set the default tooltip
     string tooltip = fromUTF8(this, _jid);
     _tips.set_tip(*this, tooltip);

     // Setup DnD targets that we can receive
     GtkTargetEntry dnd_dest_targets[] = {
//	  {"text/x-jabber-roster-item", 0, 0},
	  {"text/x-jabber-id", 0, 0},
     };
     int dest_num = sizeof(dnd_dest_targets) / sizeof(GtkTargetEntry);
     gtk_drag_dest_unset(GTK_WIDGET(this->gtkobj()));
     gtk_drag_dest_set(GTK_WIDGET(this->gtkobj()), (GtkDestDefaults) (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP), dnd_dest_targets, dest_num, (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE));

     // Setup DnD targets that we can send 
     GtkTargetEntry dnd_source_targets[] = {
       {"text/x-jabber-roster-item", 0, 0},
//       {"text/x-vcard", 0, 0},
//       {"text/x-vcard-xml", 0, 0},
       {"text/x-jabber-id", 0, 0},
     };
     int source_num = sizeof(dnd_source_targets) / sizeof(GtkTargetEntry);
     gtk_drag_source_set(GTK_WIDGET(this->gtkobj()), (GdkModifierType) (GDK_BUTTON1_MASK | GDK_BUTTON3_MASK), dnd_source_targets, source_num, (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE));

     drag_data_received.connect(slot(this, &PrettyJID::on_evtShow_drag_data_received));
     drag_data_get.connect(slot(this, &PrettyJID::on_evtShow_drag_data_get));    
     drag_begin.connect(slot(this, &PrettyJID::on_evtShow_drag_begin));

     // Hook up the presence event
     G_App->getSession().evtPresence.connect(slot(this, &PrettyJID::on_presence));

     // Set the default display
     set_display_type(dt, display_limit);

     // And grab any existing presence...
     try {
     	  const Presence& p = G_App->getSession().presenceDB().findExact(_default_jid);
	  on_presence(p, p.getType());
     } catch (PresenceDB::XCP_InvalidJID& e) {
     }
}

void PrettyJID::set_display_type(DisplayType dt, string::size_type display_limit)
{
     // save the display type
     _display_type = dt;

     string display_text;
     // If we want to display the nickname of a groupchat user
     // and we have the proper nickname
     if (!is_displaying_jid() && G_App->isGroupChatID(_jid) && !_resource.empty())
     {
	  display_text = substitute(_("%s from groupchat %s"), fromUTF8(_nickname), JID::getUser(_jid));
     }
     else
     {
	  switch (dt)
	  {
	  case dtNickRes:
	       if (!_select_resource && !_resource.empty())
	       {
		    display_text = fromUTF8(_nickname) + "/" + fromUTF8(_resource);
		    break;
	       }
	       // else it'll continue on and display just the nickname
	  case dtNick:
	       display_text = fromUTF8(_nickname);
	       break;
	  case dtJID:
	       if (!_select_resource && !_resource.empty())
	       {
		    display_text = fromUTF8(JID::getUserHost(_jid));
		    break;
	       }
	       // else it'll just display the contents of _jid
	  case dtJIDRes:
	       display_text = fromUTF8(_jid);
	       break;
	  case dtNickJIDRes:
	       if (!_resource.empty())
	       {
		    display_text = fromUTF8(_nickname) + "/" + fromUTF8(_resource);
		    display_text += " (" + fromUTF8(JID::getUserHost(_jid)) + ")";
		    break;
	       }
	       // else it'll continue on and display just the nickname
	  case dtNickJID:
	       display_text = fromUTF8(_nickname);
	       display_text += " (" + fromUTF8(JID::getUserHost(_jid)) + ")";
	       break;
	  }
     }

     // Ensure it's not too long
     if (display_limit != 0 && display_text.length() > display_limit)
	  display_text = substitute(_("%s..."), display_text.substr(0, display_limit));

     // Finally, set the text
     if (!_select_jid)
	  _lblPJ->set_text(display_text);
}

void PrettyJID::show_label(bool show_label)
{
     if (!_select_jid)
     {
	  if (show_label)
	       _lblPJ->show();
	  else
	       _lblPJ->hide();
     }
}

void PrettyJID::show_pixmap(bool show_pixmap)
{
     if (show_pixmap)
	  _pixPJ->show();
     else
	  _pixPJ->hide();
}

void PrettyJID::hide_resource_select()
{
     g_assert( _cboResource != NULL );

     // No longer selecting resource
     _select_resource = false;
     // Allow the JabberID label to expand
//FIXME: Why is this an undefined reference to Gtk::Box_Helpers::Child::set_options(bool, bool, unsigned) ?
//FIXME: Even gtk--/box.h is included, it compiles fine but doesn't link (?)
//     (*_hboxPJ->children().find(*_lblPJ))->set_options(true, true, 0);
     // Remove the resource selection
     _hboxPJ->children().remove(*_cboResource);
     _hboxPJ->children().remove(*_cboResource);
}

bool PrettyJID::is_on_roster() const
{
     return G_App->getSession().roster().containsJID(_jid);
}

bool PrettyJID::is_displaying_jid() const
{
     switch (_display_type)
     {
     case dtNickRes:
     case dtNick:
	  return false;
     case dtJID:
     case dtJIDRes:
     case dtNickJIDRes:
     case dtNickJID:
	  return true;
     }
     return true;
}

void PrettyJID::on_presence(const Presence& p, const Presence::Type prev)
{
     // Display a notification message if this presence packet
     // is from the JID associated with this widget
     if (p.getFrom() != _default_jid)
	  return;

     // Set the labels
     string filename = _pix_path + p.getShow_str() + ".xpm";
     _pixPJ->load(filename);

     // Set the tooltip
     string tooltip;
     // if they can't see the JID, show it in the tooltip
     if (!is_displaying_jid())
	  tooltip += fromUTF8(this, _default_jid) + "\n";
     tooltip += getShowName(p.getShow()) + ": " + fromUTF8(this, p.getStatus());
     _tips.set_tip(*this, tooltip);

     Element* x = p.findX("gabber:x:music:info");
     if (!x)
     {
	  x = p.findX("jabber:x:music:info");
     }
     if (x)
     {
	  string song_title = x->getChildCData("title");
	  string state = x->getChildCData("state");
	  if (state == "stopped")
	  {
	       song_title += _(" [stopped]");
	       _pixMusic->load(_pix_path + "xmms_stopped.xpm");
	  }
	  else if (state == "paused")
	  {
	       song_title += _(" [paused]");
	       _pixMusic->load(_pix_path + "xmms_paused.xpm");
	  }
	  else
	  {
	       _pixMusic->load(_pix_path + "xmms.xpm");
	  }
	  _tips.set_tip(*_evtMusic, fromUTF8(this, song_title));
	  _evtMusic->show();
     }
     else
     {
	  _evtMusic->hide();
     }

     // Presence can fundamentally change
     // some things (gpg), so say so
     changed();
}

void PrettyJID::on_entJID_changed()
{
     g_assert( _entJID != NULL );

     // I don't really like the duplication of code in this function
     // but this is such a rare case that I didn't want to redesign
     // PrettyJID to accomplish this without code duplication...

     _jid = toUTF8(_entJID, _entJID->get_text());
     if (!_jid.empty())
     {
	  // Set the new full jabberid
	  if (_resource.empty())
	  {
	       _default_jid = _jid;
	  }
	  else
	  {
	       _default_jid = _jid + "/" + _resource;
	  }

	  // Grab an existing presence for this particular resource (maybe)
	  // and process it
	  try {
	       const Presence& p = G_App->getSession().presenceDB().findExact(_default_jid);
	       on_presence(p, p.getType());
	  } catch (PresenceDB::XCP_InvalidJID& e) {
	       // Construct an offline presence
	       Presence p("", Presence::ptUnavailable, Presence::stOffline, _("No presence has been received."));
	       p.setFrom(_default_jid);
	       on_presence(p, p.getType());
	  }
     }

     // Lookup nickname - default is the username
     _nickname = JID::getUser(_jid);
     try {
	  _nickname = G_App->getSession().roster()[JID::getUserHost(_jid)].getNickname();
     } catch (Roster::XCP_InvalidJID& e) {
	  // Special handling for a groupchat ID -- use the resource as the nickname
	  if (G_App->isGroupChatID(_jid) && !_resource.empty())
	       _nickname = _resource;
     }

     // The list of resources
     list<string> resources;
     // If we have a resource, add it - it may not be among the list in presencedb
     try {
	  // Walk the list of resources and add them to the combo box
	  PresenceDB::range r = G_App->getSession().presenceDB().equal_range(_jid);
	  for (PresenceDB::const_iterator it = r.first; it != r.second; it++)
	  {
	       const Presence& p = *it;
	       
	       // If this presence is a NA presence, then skip it
	       if (p.getType() == Presence::ptUnavailable)
		    continue;
	       
	       // Extract the resource
	       const string& res = JID::getResource(p.getFrom());
	       resources.push_back(fromUTF8(_cboResource, res));
	  }
     } catch (PresenceDB::XCP_InvalidJID& e) {
	  // No presences from any resources
     }
     
     // Attach the list of resources to the combo
     if (!resources.empty())
	  _cboResource->set_popdown_strings(resources);

     // Fire the signal
     changed();
}

void PrettyJID::on_entResource_changed()
{
     g_assert( _cboResource != NULL );
     g_assert( _cboResource->get_entry() != NULL);

     _resource = toUTF8(_cboResource->get_entry(), _cboResource->get_entry()->get_text());
     if (!_resource.empty())
     {
	  // Set the new full jabberid
	  _default_jid = _jid + "/" + _resource;

	  // Grab an existing presence for this particular resource (maybe)
	  // and process it
	  try {
	       const Presence& p = G_App->getSession().presenceDB().findExact(_default_jid);
	       on_presence(p, p.getType());
	  } catch (PresenceDB::XCP_InvalidJID& e) {
	       // Construct an offline presence
	       Presence p("", Presence::ptUnavailable, Presence::stOffline, _("No presence has been received."));
	       p.setFrom(_default_jid);
	       on_presence(p, p.getType());
	  }
     }

     // Fire the signal
     changed();
}

void PrettyJID::on_evtShow_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time)
{
     ContactSendDlg* csd = manage(new ContactSendDlg(_jid));
     csd->push_drag_data(drag_ctx, x, y, data, info, time);
}

void PrettyJID::on_evtShow_drag_data_get(GdkDragContext* drag_ctx, GtkSelectionData* data, guint info, guint time)
{
     if (data->target == gdk_atom_intern("text/x-jabber-id", FALSE))
     {
	  string dnddata;
	  dnddata = "jabber:" + JID::getUserHost(_jid) + "\n";
	  gtk_selection_data_set(data, data->target, 8, (guchar *) dnddata.c_str(), dnddata.size() + 1);
     }
     else if (data->target == gdk_atom_intern("text/x-jabber-roster-item", FALSE))
     {
	  string dnddata;
	  try {
	       Roster::Item item = G_App->getSession().roster()[JID::getUserHost(_jid)];

	       // Convert roster item to a tag
	       Element itemt("item");
	       itemt.putAttrib("jid", item.getJID());
	       if (!item.getNickname().empty())
		    itemt.putAttrib("name", item.getNickname());
	       for (Roster::Item::iterator it = item.begin(); it != item.end(); it++)
		    itemt.addElement("group", *it);

	       // Set the DnD Data
	       dnddata = itemt.toString();
	  } catch (Roster::XCP_InvalidJID& e) {}

	  gtk_selection_data_set(data, data->target, 8, (guchar *) dnddata.c_str(), dnddata.size() + 1);
     }
}

void PrettyJID::on_evtShow_drag_begin(GdkDragContext* drag_ctx)
{
     // I'm so evil. 
     // Gtk::Pixmap doesn't wanna work right... and Gnome::Pixmap doesn't have a nice accessor...
     gtk_drag_set_icon_pixmap(drag_ctx, _pixPJ->get_colormap(), _pixPJ->gtkobj()->pixmap, _pixPJ->gtkobj()->mask, -2, -2);
}
