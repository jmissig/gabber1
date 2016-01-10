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

#include "GroupsInterface.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "GabberWidgets.hh"

#include <vector>
#include <libgnome/gnome-help.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-window-icon.h>

using namespace jabberoo;
using namespace GabberUtil;

// ---------------------------------------------------------
//
// Groups Editor
//
// ---------------------------------------------------------
GroupsEditor::GroupsEditor(BaseGabberWindow* base, const string& base_string, Roster::Item* item)
     : _base(base),
       _item(item)
{
      // Grab pointers
     _entName        = _base->getEntry(string(base_string + "_NewGroup_ent").c_str());
     _clCurrent      = _base->template getWidget<Gtk::CList>(string(base_string + "_Current_clist").c_str());
     _clAvailable    = _base->template getWidget<Gtk::CList>(string(base_string + "_Available_clist").c_str());

     // Connect buttons
     _base->getButton(string(base_string + "_Add_btn").c_str())->clicked.connect(slot(this, &GroupsEditor::on_add_clicked));
     _base->getButton(string(base_string + "_Remove_btn").c_str())->clicked.connect(slot(this, &GroupsEditor::on_remove_clicked));

     // Connect Available Groups list
     _clAvailable->select_row.connect(slot(this, &GroupsEditor::on_available_selected));
     _clCurrent->select_row.connect(slot(this, &GroupsEditor::on_current_selected));
}

void GroupsEditor::refresh(Roster::Item* item)
{
     _item = item;

     // Clear the lists
     _clCurrent->clear();
     _clAvailable->clear();

     _clAvailable->freeze();
     _clCurrent->freeze();

     // Build a list of available groups
     typedef map<string, set<string> > GMAP;
     const GMAP grps = G_App->getSession().roster().getGroups();
     for (GMAP::const_iterator it = grps.begin(); it != grps.end(); ++it)
     {
	  // Don't display virtual groups
	  if (it->first == "Unfiled" || it->first == "Pending" || it->first == "Agents") continue;
	  vector<Gtk::string> tmpstr;
	  tmpstr.push_back(fromUTF8(_clAvailable, it->first));
	  _clAvailable->rows().push_back(tmpstr);
     }

     // Load group information
     for (Roster::Item::iterator it = _item->begin(); it != _item->end(); ++it)
     {
	  // Don't display virtual groups
	  if (*it == "Unfiled" || *it == "Pending" || *it == "Agents") continue;
	  vector<Gtk::string> tmpstr;
	  tmpstr.push_back(fromUTF8(_clCurrent, *it));
	  _clCurrent->rows().push_back(tmpstr);
	  
	  // Remove this from available
// 	  for (Gtk::CList_Helpers::RowIterator ri = _clAvailable->rows().begin(); ri != _clAvailable->rows().end(); ++ri)
// 	  {
// 	       if (ri 
// 	  }
// 	  Gtk::CList_Helpers::RowIterator ri = _clAvailable->rows().find_data(GCHAR_TO_POINTER(tmpstr.c_str()));
// 	  if (ri != _clAvailable->rows().end())
// 	  {
// 	       _clAvailable->rows().erase(ri);
// 	  }
     }

     _clAvailable->thaw();
     _clCurrent->thaw();
}

void GroupsEditor::on_available_selected(int row, int col, GdkEvent* e)
{
     _entName->set_text(_clAvailable->cell(row, col).get_text());
     // If double-clicked, retrieve the row and 
     // act like they clicked "add"
     if (e->type == GDK_2BUTTON_PRESS)
     {
	  on_add_clicked();
     }
}

void GroupsEditor::on_current_selected(int row, int col, GdkEvent* e)
{
     // If double-clicked, retrieve the row and 
     // act like they clicked "remove"
     if (e->type == GDK_2BUTTON_PRESS)
     {
	  on_remove_clicked();
     }
}

void GroupsEditor::on_add_clicked()
{
     _item->addToGroup(toUTF8(_entName, _entName->get_text()));
     refresh(_item);
}

void GroupsEditor::on_remove_clicked()
{
     // If a group is selected...
     if (!_clCurrent->selection().empty())
     {
	  _item->delFromGroup(toUTF8(_clCurrent, _clCurrent->selection()[0][0].get_text()));
	  refresh(_item);
     }
}

// ---------------------------------------------------------
//
// Edit Groups Dialog
//
// ---------------------------------------------------------

EditGroupsDlg::EditGroupsDlg(const string& jid)
     : BaseGabberDialog("EditGroups_dlg"),
       _item(new Roster::Item(G_App->getSession().roster()[jid]))
{
     string nickname;
     try {
	  nickname = G_App->getSession().roster()[JID::getUserHost(jid)].getNickname();
     } catch (Roster::XCP_InvalidJID& e) {
	  // Typically we'll just use the username
	  nickname = JID::getUser(jid);
     }

     // Widget pointers
     _editor = manage(new GroupsEditor(this, "EditGroups", _item));
     PrettyJID* pj = manage(new PrettyJID(jid, nickname, PrettyJID::dtNickJID));
     pj->show();
     getWidget<Gtk::HBox>("EditGroups_JIDInfo_hbox")->pack_start(*pj, true, true, 0);

     // Connect buttons
     getButton("EditGroups_OK_btn")->clicked.connect(slot(this, &EditGroupsDlg::on_ok_clicked));
     getButton("EditGroups_Cancel_btn")->clicked.connect(slot(this, &EditGroupsDlg::on_cancel_clicked));
     getButton("EditGroups_Help_btn")->clicked.connect(slot(this, &EditGroupsDlg::on_help_clicked));

     // Pixmaps
     string pix_path = ConfigManager::get_PIXPATH();
     string window_icon = pix_path + "gnome-editgroups.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();

     _thisWindow->set_title(substitute(_("Edit Groups for %s"), fromUTF8(nickname)) + _(" - Gabber"));

     _editor->refresh(_item);
     show();
}

EditGroupsDlg::~EditGroupsDlg()
{
     delete _item;
}

void EditGroupsDlg::on_ok_clicked()
{
     // Make sure that the item doesn't actually set any of our virtual groups
     _editor->get_item()->delFromGroup("Unfiled");
     _editor->get_item()->delFromGroup("Pending");
     _editor->get_item()->delFromGroup("Agents");

     // Update the roster
     G_App->getSession().roster() << *_editor->get_item();

     close();
}


void EditGroupsDlg::on_cancel_clicked()
{
     close();
}


void EditGroupsDlg::on_help_clicked()
{
     // Call the manual
     GnomeHelpMenuEntry help_entry = { "gabber", "users.html#USERS-EDITUSER-GROUPS" };
     gnome_help_display (NULL, &help_entry);
}
