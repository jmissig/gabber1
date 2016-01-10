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

/*
 * ContactInterface
 * Author Brandon Lees <brandon@sci.brooklyn.cuny.edu>
 */

#ifndef INCL_CONTACTS_INTERFACE_HH
#define INCL_CONTACTS_INTERFACE_HH

#include "jabberoo.hh"

#include "BaseGabberWindow.hh"
#include "RosterView.hh"
#include "MessageManager.hh"
#include "MessageViews.hh"

class PrettyJID;

class ContactRecvView
     : public MessageView, public BaseGabberDialog
{
public:
     ContactRecvView(const jabberoo::Message& m, ViewMap& vm);
     ContactRecvView(const string& jid, ViewMap& vm);
     ~ContactRecvView();
     void render(const jabberoo::Message& m);

     static void init_view(MessageManager& m);
     static MessageView* new_view_msg(const jabberoo::Message& m, ViewMap& vm);
     static MessageView* new_view_jid(const string& jid, ViewMap& vm);
protected:
     void on_Remove_clicked();
     void on_ViewInfo_clicked();
     void on_Add_clicked();
     void on_Cancel_clicked();

     void init(string jid);
private:
     Gtk::CTree*	       _ctreePeople;
     Gtk::Label*               _lblRecvFrom;
     SimpleRosterView*         _RosterView;
     jabberoo::Roster::ItemMap _roster;
};

class ContactSendDlg
     : public BaseGabberDialog
{
public:
     ContactSendDlg(const string& jid);
     ~ContactSendDlg();
     void push_drag_data(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time);
protected:
     void on_ViewInfo_clicked();
     void on_Remove_clicked();
     void on_Send_clicked();
     void on_Cancel_clicked();
private:
     Gtk::CTree*	       _ctreePeople;
     Gtk::Label*               _lblSendTo;
     SimpleRosterView*         _RosterView;
     PrettyJID*                _pjid;
     jabberoo::Roster::ItemMap _roster;
     string		       _jid;
};

#endif

