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
 *  Copyright (C) 1999-2000 Dave Smith & Julian Missig
 */

#include "AddToGroupDlg.hh"
#include "GabberApp.hh"
#include "GabberUtility.hh"

extern "C" {
#ifdef GABBER_WINICON
#include <libgnomeui/gnome-window-icon.h>
#endif
}

using namespace jabberoo;
using namespace GabberUtil;

AddToGroupDlg::AddToGroupDlg(CallbackFunc s)
     : BaseGabberDialog("AddToGroup_dlg"), _cbNotify(s)
{
     // Setup buttons
     getButton("AddToGroup_AddToGroup_btn")->clicked.connect(slot(this, &AddToGroupDlg::on_AddToGroup_clicked));
     getButton("AddToGroup_Cancel_btn")->clicked.connect(slot(this, &AddToGroupDlg::on_Cancel_clicked));

     _entName = getEntry("AddToGroup_NewGroup_txt");
     _clGroups = getWidget<Gtk::CList>("AddToGroup_Groups_clist");
     _clGroups->select_row.connect(slot(this, &AddToGroupDlg::on_Groups_select_row));

     // Build a list of available groups
     typedef map<string, set<string> > GMAP;
     const GMAP grps = G_App->getSession().roster().getGroups();
     for (GMAP::const_iterator it = grps.begin(); it != grps.end(); it++)
     {
	  if (it->first == "Unfiled") continue;
	  const char* data[1] = {fromUTF8(_clGroups, it->first).c_str() };
	  _clGroups->append(data);
     }

     // Pixmaps
     string pix_path = ConfigManager::get_PIXPATH();
#ifdef GABBER_WINICON
     string window_icon = pix_path + "gnome-editgroups.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();
#endif

     // Display
     show();
}

void AddToGroupDlg::on_AddToGroup_clicked()
{
     _cbNotify(toUTF8(_entName, _entName->get_text()));
     close();
}

void AddToGroupDlg::on_Cancel_clicked()
{
     close();
}

void AddToGroupDlg::on_Groups_select_row(int row, int col, GdkEvent* e)
{
     _entName->set_text(_clGroups->cell(row, col).get_text());
}

