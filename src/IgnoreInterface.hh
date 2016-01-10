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
 * Author Brandon Lees <brandon@sci.brooklyn.cuny.edu>
 */

#ifndef INCL_IGNORE_INTERFACE_HH
#define INCL_IGNORE_INTERFACE_HH

#include "BaseGabberWindow.hh"
#include "RosterView.hh"

class IgnoreDlg
     : public BaseGabberDialog
{
public:
     static void execute();

     IgnoreDlg();
     ~IgnoreDlg();
protected:
     void on_Add_clicked();
     void on_Remove_clicked();
     void on_OK_clicked();
     void on_Apply_clicked();
     void on_Cancel_clicked();
private:
     SimpleRosterView*         _RosterView;
     jabberoo::Roster::ItemMap _ignorelist;
     Gtk::CheckButton*         _chkOutsideContact;
     Gtk::CTree*	       _ctreePeople;
};

class IgnoreAddDlg
     : public BaseGabberDialog
{
public:
     static bool execute(string& jid);

     IgnoreAddDlg();
     ~IgnoreAddDlg();
private:
     Gtk::Entry* _entJID;
};

#endif

