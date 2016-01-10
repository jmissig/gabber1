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
 *  Copyright (C) 1999-2001 Dave Smith & Julian Missig
 */

/*
 * IgnoreInterface
 * Author Brandon Lees <brandon@aspect.net>
 */

#include "IgnoreInterface.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"

using namespace jabberoo;
using namespace GabberUtil;

// ---------------------------------------------------------
//
// Ignore Dialog
//
// ---------------------------------------------------------

void IgnoreDlg::execute()
{
     manage(new IgnoreDlg());
}

IgnoreDlg::IgnoreDlg()
     : BaseGabberDialog("Ignore_dlg")
{
     ConfigManager& cf = G_App->getCfg();

     _thisWindow->realize();

     getButton("Ignore_Add_btn")->clicked.connect(slot(this, &IgnoreDlg::on_Add_clicked));
     getButton("Ignore_Remove_btn")->clicked.connect(slot(this, &IgnoreDlg::on_Remove_clicked));
     getButton("Ignore_OK_btn")->clicked.connect(slot(this, &IgnoreDlg::on_OK_clicked));
     getButton("Ignore_Cancel_btn")->clicked.connect(slot(this, &IgnoreDlg::on_Cancel_clicked));

     _chkOutsideContact = getCheckButton("Ignore_OutsideContact_chk");
     _chkOutsideContact->set_active(cf.ignorelist.outsidecontact);

     _ctreePeople = getWidget<Gtk::CTree>("Ignore_People_ctree");
     Gtk::ScrolledWindow* tsw = getWidget<Gtk::ScrolledWindow>("Ignore_People_scroll");
     _RosterView = new SimpleRosterView(_ctreePeople, tsw, _ignorelist, false);
     _RosterView->ignore_nick();

     for (list<string>::iterator it = cf.ignorelist.ignore.begin(); it != cf.ignorelist.ignore.end() ; it++)
     {
	  Roster::Item item(*it, JID::getUser(*it));
          _ignorelist.insert(make_pair(*it, item));
     }
     _RosterView->refresh();

     _thisWindow->show();

     // Display
     show();
}

IgnoreDlg::~IgnoreDlg()
{
     delete _RosterView;
}

void IgnoreDlg::on_Add_clicked()
{
     string jid;
     if (IgnoreAddDlg::execute(jid))
     {
	  Roster::Item item(JID::getUserHost(jid), JID::getUserHost(jid));
	  _ignorelist.insert(make_pair(jid, item));
	  _RosterView->refresh();
     }
}

void IgnoreDlg::on_Remove_clicked()
{
     if (_ctreePeople->selection().begin() != _ctreePeople->selection().end())
     {
          Gtk::CTree::Row& r = *_ctreePeople->selection().begin();
          Roster::ItemMap::iterator it = _ignorelist.find(r[_RosterView->_colJID].get_text());
          if (it != _ignorelist.end())
          {
               _ignorelist.erase(it);
               _RosterView->refresh();
          }
     }
}

void IgnoreDlg::on_OK_clicked()
{
     ConfigManager& cf = G_App->getCfg();
     cf.ignorelist.outsidecontact = _chkOutsideContact->get_active();
     cf.ignorelist.ignore.clear();
     for (Roster::ItemMap::iterator it = _ignorelist.begin(); it != _ignorelist.end(); it++)
	    cf.ignorelist.ignore.push_back(it->first);

     close();
}

void IgnoreDlg::on_Cancel_clicked()
{
     close();
}

////////////////////////////////////////////////////
// IgnoreAddDlg
////////////////////////////////////////////////////

bool IgnoreAddDlg::execute(string& jid)
{
     IgnoreAddDlg* dialog = manage(new IgnoreAddDlg);
     if (dialog->_thisDialog->run() != 0)
     {
	  dialog->close();
	  return false;
     }
     jid = dialog->_entJID->get_text();
     dialog->close();
     return true;
}

IgnoreAddDlg::IgnoreAddDlg()
     : BaseGabberDialog("IgnoreAdd_dlg")
{
     _entJID = getEntry("Ignore_JabberID_ent");
}

IgnoreAddDlg::~IgnoreAddDlg()
{
}
