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

#include "FilterInterface.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"

#include <libgnome/gnome-i18n.h>
#include <gtk--/separator.h>

using namespace jabberoo;
using namespace GabberUtil;

void add_show(Gtk::HBox* h, Gtk::Widget* w, bool expand=true, bool fill=true, int spacing = 4)
{
     w->show();
     h->pack_start(*w, expand, fill, spacing);
}

// ---------------------------------------------------------
//
// Action view
//
// ---------------------------------------------------------
class ActionView
     : public Gtk::HBox
{
public:
     ActionView(Filter::ActionList& alist, Filter::ActionList::iterator it);
     virtual ~ActionView() {}
protected:
     void on_ParamChange();
     void on_ActionChange(int index);
     void on_Remove();
private:
     MenuBuilder                  _menu;
     Filter::ActionList&          _alist;
     Filter::ActionList::iterator _action;
     Gtk::Entry*                  _pentry;
     Gtk::Label*                  _slabel;
};

ActionView::ActionView(Filter::ActionList& alist, Filter::ActionList::iterator it)
     : Gtk::HBox(), _alist(alist), _action(it)
{
     // Add base widgets into this container
     add_show(this, manage(new Gtk::Label(_("Then"))), false, 4);
     
     // Create action menu
     _menu.add_item(_("change message type to..."), Filter::Action::SetType);
     _menu.add_item(_("forward message to..."), Filter::Action::ForwardTo);
     _menu.add_item(_("reply with..."), Filter::Action::ReplyWith);
     _menu.add_item(_("store this message offline."), Filter::Action::StoreOffline);
     _menu.add_item(_("continue processing rules."), Filter::Action::Continue);
     _menu.selected.connect(slot(this, &ActionView::on_ActionChange));
     _menu.finish_items();

     // Create option menu widget to hold action menu
     Gtk::OptionMenu* optmnu = manage(new Gtk::OptionMenu());
     optmnu->set_menu(_menu.get_menu());
     optmnu->set_history(_action->value());
     optmnu->show();
     pack_start(*optmnu, false);

     // Add a spacing label to take up the expanded space when pentry is gone
     _slabel = manage(new Gtk::Label(" "));
     add_show(this, _slabel, true, true, 4);

     // Create parameters entry -- 256 chars max
     _pentry = manage(new Gtk::Entry(256));
     _pentry->changed.connect(slot(this, &ActionView::on_ParamChange));
     _pentry->set_text(fromUTF8(_action->param()));
     _pentry->set_usize(120, 0);
     add_show(this, _pentry, true, true, 4);
     if (it->requires_param())
     {
	  _pentry->show();
	  _slabel->hide();
     }
     else
     {
	  _pentry->hide();
	  _slabel->show();
     }

     // Create seperator
     Gtk::VSeparator* v = manage(new Gtk::VSeparator());
     add_show(this, v, false, false, 4);

     // Create remove button
     Gtk::Button* b = manage(new Gtk::Button(_("Remove")));
     b->clicked.connect(slot(this, &ActionView::on_Remove));
     b->set_border_width(4);
     add_show(this, b, false, false, 0);

     show();
}

void ActionView::on_ParamChange()
{
     // Update the underlying action
     *_action << toUTF8(_pentry->get_text());
}

void ActionView::on_ActionChange(int index)
{
     // Update the underlying action
     *_action << (Filter::Action::Value)index;
     // Display the parameter entry depending on context
     if (_action->requires_param())
     {
	  _pentry->show();
	  _slabel->hide();
     }
     else
     {
	  _pentry->hide();		    
	  _slabel->show();
     }
}

void ActionView::on_Remove()
{
     // Delete the underlying action
     _alist.erase(_action);

     // Remove this widget from its parent
     Gtk::Container* parent = static_cast<Gtk::Container*>(get_parent());
     parent->remove(*this);
}

// ---------------------------------------------------------
//
// Condition Dialog
//
// ---------------------------------------------------------
class ConditionView
     : public Gtk::HBox
{
public:
     ConditionView(Filter::ConditionList& clist, Filter::ConditionList::iterator it);
     virtual ~ConditionView() {}
protected:
     void on_ParamChange();
     void on_ConditionChange(int index);
     void on_Remove();
private:
     MenuBuilder                     _menu;
     Filter::ConditionList&          _clist;
     Filter::ConditionList::iterator _condition;
     Gtk::Entry*                     _pentry;
     Gtk::Label*                     _slabel;
};

ConditionView::ConditionView(Filter::ConditionList& clist, Filter::ConditionList::iterator it)
     : Gtk::HBox(), _clist(clist), _condition(it)
{
     // Add base widgets into this container
     add_show(this, manage(new Gtk::Label(_("If"))), false, 5);

     // Create condition menu
     _menu.add_item(_("I'm not online."), Filter::Condition::Unavailable);
     _menu.add_item(_("the message is from..."), Filter::Condition::From);
     _menu.add_item(_("the message is sent to..."), Filter::Condition::MyResourceEquals);
     _menu.add_item(_("the subject is..."), Filter::Condition::SubjectEquals);
     _menu.add_item(_("the body is..."), Filter::Condition::BodyEquals);
     _menu.add_item(_("my status is..."), Filter::Condition::ShowEquals);
     _menu.add_item(_("the message type is..."), Filter::Condition::TypeEquals);
     _menu.selected.connect(slot(this, &ConditionView::on_ConditionChange));
     _menu.finish_items();

     // Create option menu to hold condition menu
     Gtk::OptionMenu* optmnu = manage(new Gtk::OptionMenu());
     optmnu->set_menu(_menu.get_menu());
     optmnu->set_history(_condition->value());
     optmnu->show();
     pack_start(*optmnu, false);

     // Add a spacing label to take up the expanded space when pentry is gone
     _slabel = manage(new Gtk::Label(" "));
     add_show(this, _slabel, true, true, 4);

     // Create parameters entry -- 256 chars max
     _pentry = manage(new Gtk::Entry(256));
     _pentry->changed.connect(slot(this, &ConditionView::on_ParamChange));
     _pentry->set_text(fromUTF8(_condition->param()));
     _pentry->set_usize(120,0);
     add_show(this, _pentry, true, true, 4);
     if (it->requires_param())
     {
	  _pentry->show();
	  _slabel->hide();
     }
     else
     {
	  _pentry->hide();
	  _slabel->show();
     }

     // Create seperator
     Gtk::VSeparator* v = manage(new Gtk::VSeparator());
     add_show(this, v, false, false, 4);

     // Create remove button
     Gtk::Button* b = manage(new Gtk::Button(_("Remove")));
     b->clicked.connect(slot(this, &ConditionView::on_Remove));
     b->set_border_width(4);
     add_show(this, b, false, false, 0);

     show();
}

void ConditionView::on_ParamChange()
{
     // Update underlying condition
     *_condition << toUTF8(_pentry->get_text());
}

void ConditionView::on_ConditionChange(int index)
{
     // Update the underlying action
     *_condition << (Filter::Condition::Value)index;
     // Display the parameter entry depending on context
     if (_condition->requires_param())
     {
	  _pentry->show();
	  _slabel->hide();
     }
     else
     {
	  _pentry->hide();		    
	  _slabel->show();
     }
}

void ConditionView::on_Remove()
{
     // Delete the underlying condition
     _clist.erase(_condition);

     // Remove this widget from it's parent
     Gtk::Container* parent = static_cast<Gtk::Container*>(get_parent());
     parent->remove(*this);
}

// ---------------------------------------------------------
//
// Filter List view
//
// ---------------------------------------------------------
FilterListView* FilterListView::_Dialog = NULL;

void FilterListView::execute()
{
     if (_Dialog == NULL)
	  // Send request to the server for a filter
	  G_App->getSession().queryNamespace("jabber:iq:filter", slot(&FilterListView::handleFilterIQ));
}

FilterListView::~FilterListView()
{
     _Dialog = NULL;
}

FilterListView::FilterListView(FilterList fl)
     : BaseGabberDialog("FilterList_dlg"), _flist(fl)
{
     main_dialog(_thisDialog);
     // Get widget pointers
     _lstFilters = getWidget<Gtk::CList>("FilterList_Filters_clist");

     // Setup event handlers
     _lstFilters->select_row.connect(slot(this, &FilterListView::on_FilterSelect));
     getButton("FilterList_OK_btn")->clicked.connect(slot(this, &FilterListView::on_ok_clicked));
     getButton("FilterList_Cancel_btn")->clicked.connect(slot(this, &FilterListView::on_cancel_clicked));
     getButton("FilterList_Edit_btn")->clicked.connect(slot(this, &FilterListView::on_EditFilterClick));
     getButton("FilterList_New_btn")->clicked.connect(slot(this, &FilterListView::on_NewFilterClick));
     getButton("FilterList_Remove_btn")->clicked.connect(slot(this, &FilterListView::on_DelFilterClick));
     getButton("FilterList_Promote_btn")->clicked.connect(slot(this, &FilterListView::on_PromoteFilterClick));
     getButton("FilterList_Demote_btn")->clicked.connect(slot(this, &FilterListView::on_DemoteFilterClick));

     // View the filter list
     refresh();

}

void FilterListView::on_FilterSelect(int row, int col, GdkEvent* e)
{
     // If double-clicked, retrieve the row and display
     // a view of the selected filter
     if (e->type == GDK_2BUTTON_PRESS)
     {
	  // Retrieve the pointer to the filter
	  Filter& f = *static_cast<Filter*>(_lstFilters->get_row_data(row));
	  displayFilter(f);
     }
}

void FilterListView::on_ok_clicked()
{
     // Generate an XML representation of the FilterList
     // and send it to the server
     string s  = "<iq type='set'>" + _flist.toXML() + "</iq>";
     G_App->getSession() << s.c_str();
     close();
}

void FilterListView::on_cancel_clicked()
{
     close();
}

void FilterListView::on_NewFilterClick()
{
     // Insert a filter into the list
     FilterList::iterator it = _flist.insert(_flist.end(), Filter(_("New Rule")));

     displayFilter(*it);
}

void FilterListView::on_EditFilterClick()
{
     if (!_lstFilters->selection().empty())
     {
	  // Get the data pointer from the selection
	  Filter& f = *static_cast<Filter*>(_lstFilters->selection().begin()->get_data());
	  displayFilter(f);
     }
}

void FilterListView::on_DelFilterClick()
{
     if (!_lstFilters->selection().empty())
     {
	  // Get the data pointer from the selection
	  Filter& f = *static_cast<Filter*>(_lstFilters->selection().begin()->get_data());

	  // Find and erase the iterator in the filter list which holds this corresponding
	  FilterList::iterator it = find(_flist.begin(), _flist.end(), f);
	  if (it != _flist.end())
	       _flist.erase(it);

	  // Update the display
	  refresh();
     }
}

void FilterListView::on_PromoteFilterClick()
{
     if (!_lstFilters->selection().empty())
     {
	  // Get the data pointer from the selection
	  Filter& f = *static_cast<Filter*>(_lstFilters->selection().begin()->get_data());

	  // Find the iterator 
	  FilterList::iterator it = find(_flist.begin(), _flist.end(), f);

	  // If a matching filter was found and it's not already the first
	  // filter, swap this filter and the previous one
	  if ((it != _flist.end()) && (it != _flist.begin()))
	  {
	       swap(*it, *(--it));
	  }
	  
	  // Update the display
	  refresh();
     }
}

void FilterListView::on_DemoteFilterClick()
{
     if (!_lstFilters->selection().empty())
     {
	  // Get the data pointer from the selection
	  Filter& f = *static_cast<Filter*>(_lstFilters->selection().begin()->get_data());

	  // Find the iterator 
	  FilterList::iterator it = find(_flist.begin(), _flist.end(), f);

	  // If a matching filter was found and it's not already the last
	  // filter, swap this filter and the next one
	  if ((it != _flist.end()) && (&(f) != &_flist.back()))
	  {
	       swap(*it, *(++it));
	  }
	  
	  // Update the display
	  refresh();
     }
}


void FilterListView::handleFilterIQ(const Element& iq)
{
     // If we get a good query back, create a new FilterList 
     // from the packet and create a view of it
     if (iq.cmpAttrib("type", "result"))
     {
	  // Extract the query 
	  const Element* query = iq.findElement("query");
	  if (query != NULL)
		{
	       // Create a new view, passing a FilterList constructor
	       _Dialog = manage(new FilterListView(FilterList(*query)));
		}
	  else
	       cerr << "ERROR->Unable to extract query from IQ: " << iq.toString() << endl;
     }
     else 
     {
	  cerr << "ERROR->Server does not support filters." << endl;
	  Gnome::Dialog* d = manage(Gnome::Dialogs::warning(_("Sorry, your server does not support Jabber filters.")));
	  d->set_modal(true);
	  main_dialog(d);
     //	  getWidget<Gtk::Frame>("FilterList_frame")->set_sensitive(false);
     }
}

void FilterListView::displayFilter(jabberoo::Filter& f)
{
     // Create a view of the filter
     FilterView* v = manage(new FilterView(f));

     // Setup  so that the list will refresh when the view
     // is destroyed
     v->evtDestroy.connect(slot(this, &FilterListView::refresh));
     v->getBaseDialog()->set_parent(*_thisWindow);
}

void FilterListView::refresh()
{
     _lstFilters->freeze();
     _lstFilters->clear();
     // Load the filter names into _lstFilters
     for (FilterList::iterator it = _flist.begin(); it != _flist.end(); it++)
     {
	  // Insert a new row in the filter list
	  const char* rowdata[1] = { fromUTF8(it->Name()).c_str() };
	  int row = _lstFilters->append_row(rowdata);
	  // Store the corresponding filter address in the last element's data section	  
	  Filter* fp = &(*it);
	  _lstFilters->set_row_data(row, fp);
     }
     _lstFilters->thaw();
}


// ---------------------------------------------------------
//
// Filter Dialog
//
// ---------------------------------------------------------
FilterView::FilterView(Filter& f)
     : BaseGabberDialog("Filter_dlg"), _original_filter(f), _filter(f)
{
     // Note: This view creates a copy of the specified filter _and_ stores the reference
     //       to the original filter; this is to allow the dialog to have a simple undo 
     //       mechanism. All changes made in the view are made to the filter copy. When
     //       the OK button is clicked, the original filter reference is overwritten with
     //       the modified filter. If the Cancel button is clicked, no change is made to the
     //       original reference.

     main_dialog(_thisDialog);
     // Get widget pointers
     _vbxActions    = getWidget<Gtk::VBox>("Filter_Action_vbox");
     _vbxConditions = getWidget<Gtk::VBox>("Filter_Condition_vbox");
     _entFilterName = getEntry("Filter_Name_txt");
     
     // Setup event handlers
     _entFilterName->changed.connect(slot(this, &FilterView::on_FilterNameChange));
     getButton("Filter_CondNew_btn")->clicked.connect(slot(this, &FilterView::on_NewConditionClick));
     getButton("Filter_ActNew_btn")->clicked.connect(slot(this, &FilterView::on_NewActionClick));
     getButton("Filter_OK_btn")->clicked.connect(slot(this, &FilterView::on_OKClick));
     getButton("Filter_Cancel_btn")->clicked.connect(slot(this, &FilterView::on_CancelClick));

     // Load filter name
     _entFilterName->set_text(fromUTF8(f.Name()));

     // Walk the filter's conditions and create a view for each
     for (Filter::ConditionList::iterator it = _filter.Conditions().begin(); it != _filter.Conditions().end(); it++)
     {
	  // Create a condition view
	  ConditionView* v = manage(new ConditionView(_filter.Conditions(), it));
	  // Add the view to the box
	  _vbxConditions->pack_start(*v, false);
     }

     // Walk the filter's action and create a view for each
     for (Filter::ActionList::iterator it = _filter.Actions().begin(); it != _filter.Actions().end(); it++)
     {
	  // Create an action view
	  ActionView* v = manage(new ActionView(_filter.Actions(), it));
	  // Add the view to the box
	  _vbxActions->pack_start(*v, false);
     }

     // Display the view
     show();
}

FilterView::~FilterView()
{
     cerr << "FilterView released" << endl;
}

void FilterView::on_NewActionClick()
{
     // Create a new action
     Filter::ActionList::iterator it = _filter.Actions().insert(_filter.Actions().end(), Filter::Action(Filter::Action::SetType, ""));

     // Create a corresponding view
     ActionView* v = manage(new ActionView(_filter.Actions(), it));
     _vbxActions->pack_start(*v, false);	  
}

void FilterView::on_NewConditionClick()
{
     // Create a new condition
     Filter::ConditionList::iterator it = _filter.Conditions().insert(_filter.Conditions().end(), Filter::Condition(Filter::Condition::Unavailable));

     // Create a correspondin view
     ConditionView* v = manage(new ConditionView(_filter.Conditions(), it));
     _vbxConditions->pack_start(*v, false);
}

void FilterView::on_FilterNameChange()
{
     // Update filter name
     _filter.setName(toUTF8(_entFilterName->get_text()));
}

void FilterView::on_OKClick()
{
     // Overwrite the old filter
     _original_filter = _filter;
     // Close the dialog
     close();
}

void FilterView::on_CancelClick()
{
     // Close
     close();
}


