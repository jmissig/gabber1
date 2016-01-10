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

#ifndef INCL_GROUPS_INTERFACE_HH
#define INCL_GROUPS_INTERFACE_HH

#include "jabberoo.hh"

#include "BaseGabberWindow.hh"

class GroupsEditor
     : public SigC::Object
{
public:
     GroupsEditor(BaseGabberWindow* base, const string& base_string, jabberoo::Roster::Item* item);
     void refresh(jabberoo::Roster::Item* item);
     // Accessors
     jabberoo::Roster::Item* get_item() { return _item; }
protected:
     void on_available_selected(int row, int col, GdkEvent* e);
     void on_current_selected(int row, int col, GdkEvent* e);
     void on_add_clicked();
     void on_remove_clicked();
private:
     // Members
     BaseGabberWindow* _base;
     Gtk::Entry* _entName;
     Gtk::CList* _clCurrent;
     Gtk::CList* _clAvailable;
     jabberoo::Roster::Item* _item;
};

class EditGroupsDlg : 
     public BaseGabberDialog
{
public:
     EditGroupsDlg(const string& jid);
     ~EditGroupsDlg();
private:
     // Event handlers
     void on_close();
     void on_changed();
     void on_ok_clicked();
     void on_cancel_clicked();
     void on_help_clicked();
     // Members
     GroupsEditor* _editor;
     jabberoo::Roster::Item* _item;
};

#endif
