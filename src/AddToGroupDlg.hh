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

#ifndef INCL_ADDTOGROUP_DLG
#define INCL_ADDTOGROUP_DLG

#include "BaseGabberWindow.hh"

class AddToGroupDlg
     : public BaseGabberDialog
{
public:
     typedef Slot1<void, const string&> CallbackFunc;
     AddToGroupDlg(CallbackFunc s);
     virtual ~AddToGroupDlg() {}
private:
     Gtk::Entry* _entName;
     Gtk::CList* _clGroups;
     CallbackFunc _cbNotify;
     void on_AddToGroup_clicked();
     void on_Cancel_clicked();
     void on_Groups_select_row(int row, int col, GdkEvent* e);
};

#endif
