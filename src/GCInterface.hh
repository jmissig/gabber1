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
 * GCInterface
 * Author Brandon Lees <brandon@sci.brooklyn.cuny.edu>
 */

#ifndef INCL_GC_INTERFACE_HH
#define INCL_GC_INTERFACE_HH

#include "jabberoo.hh"

#include "BaseGabberWindow.hh"
#include "RosterView.hh"
#include "MessageViews.hh"

class GCIDlg
     : public BaseGabberDialog
{
public:
     static void execute(const string& roomjid, const string& subject, bool join_self = false);

     GCIDlg(const string& roomjid, const string& subject, bool join_self = false);
     ~GCIDlg();
     void push_drag_data(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time);
     void add_jid(const string& jid, const string& nickname);
protected:
     void on_Remove_clicked();
     void on_Invite_clicked();
     void on_Cancel_clicked();
private:
     string		       _roomjid;
     bool                      _join_self;
     SimpleRosterView*         _RosterView;
     jabberoo::Roster::ItemMap _roster;
     Gtk::Entry*               _entSubject;
     Gtk::CTree*	       _ctreePeople;
};

class GCIRecvView
     : public MessageView, public BaseGabberDialog
{
public:
     GCIRecvView(const jabberoo::Message& m, ViewMap& vm);
     GCIRecvView(const string& jid, ViewMap& vm);
     ~GCIRecvView();
     void render(const jabberoo::Message& m);

     static void init_view(MessageManager& mm);
     static MessageView* new_view_msg(const jabberoo::Message& m, ViewMap& vm);
     static MessageView* new_view_jid(const string& jid, ViewMap& vm);
protected:
     void on_JoinRoom_clicked();
     void on_Cancel_clicked();
     void init(const string& jid);
private:
     Gtk::Label* _lblSubject;
     Gtk::Label* _lblRoom;
     Gtk::Label* _lblServer;
     Gtk::Entry* _entNick;
     Gnome::Entry* _gentNick;
     string      _roomjid;
};

#endif

