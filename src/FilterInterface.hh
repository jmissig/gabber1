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
 
#ifndef INCL_FILTER_INTERFACE_HH
#define INCL_FILTER_INTERFACE_HH

#include "jabberoo.hh"
#include "jabberoox.hh"

#include "BaseGabberWindow.hh"

class FilterListView
     : public BaseGabberDialog
{
public:
     static void execute();
protected:
     // Constructor
     FilterListView(jabberoo::FilterList fl);
     ~FilterListView();
     // Event handlers
     void on_FilterSelect(int row, int col, GdkEvent* e);
     void on_ok_clicked();
     void on_cancel_clicked();
     // Filter mod/edit
     void on_NewFilterClick();
     void on_EditFilterClick();
     void on_DelFilterClick();
     // Filter promotion
     void on_PromoteFilterClick();
     void on_DemoteFilterClick();
     // Filter query handler
     static void handleFilterIQ(const Element& iq);
     // Filter list renderer
     void displayFilter(jabberoo::Filter& f);
     void refresh();
private:
     static FilterListView* _Dialog;
     Gtk::CList* _lstFilters;
     jabberoo::FilterList _flist;
};

class FilterView
     : public BaseGabberDialog
{
public:
     FilterView(jabberoo::Filter& f);
     virtual ~FilterView();
protected:
     void on_NewActionClick();
     void on_NewConditionClick();
     void on_FilterNameChange();
     void on_OKClick();
     void on_CancelClick();
private:
     Gtk::Entry* _entFilterName;
     Gtk::VBox*  _vbxActions;
     Gtk::VBox*  _vbxConditions;
     jabberoo::Filter&     _original_filter;
     jabberoo::Filter      _filter;

};

#endif
