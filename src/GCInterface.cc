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
 * Author Brandon Lees <brandon@aspect.net>
 */

#include "GabberConfig.hh" // for _()

#include "GCInterface.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "GabberWidgets.hh"
#include "MessageViews.hh"

#include <libgnome/gnome-i18n.h>

using namespace jabberoo;
using namespace GabberUtil;

// ---------------------------------------------------------
//
// Groupchat Invite Dialog
//
// ---------------------------------------------------------

void GCIDlg::execute(const string& roomjid, const string& subject, bool join_self)
{
     manage(new GCIDlg(roomjid, subject, join_self));
}

GCIDlg::GCIDlg(const string& roomjid, const string& subject, bool join_self)
     : BaseGabberDialog("GCI_dlg"), _roomjid(roomjid), _join_self(join_self)
{
     // Grab the widgets
     _entSubject = getEntry("GCI_Subject_ent");
     _entSubject->set_text(fromUTF8(_entSubject, subject));
     _ctreePeople = getWidget<Gtk::CTree>("GCI_People_ctree");
     Gtk::ScrolledWindow* tsw = getWidget<Gtk::ScrolledWindow>("GCI_People_scroll");
     _RosterView = new SimpleRosterView(_ctreePeople, tsw, _roster, false);

     // Set the room label
     Gtk::Label* l = getLabel("GCI_Room_lbl");
     string room = JID::getUser(_roomjid);
     // It's a IRC channel
     string::size_type percent = room.find('%');
     if (percent != string::npos)
	  l->set_text(substitute(_("%s on IRC Server %s"), 
				 fromUTF8(l, room.substr(0, percent)), 
				 fromUTF8(l, room.substr(percent+ 1))));
     else
	  l->set_text(fromUTF8(l, room));
     l = getLabel("GCI_Server_lbl");
     l->set_text(fromUTF8(l, JID::getHost(_roomjid)));

     getButton("GCI_Remove_btn")->clicked.connect(slot(this, &GCIDlg::on_Remove_clicked));
     getButton("GCI_Send_btn")->clicked.connect(slot(this, &GCIDlg::on_Invite_clicked));
     getButton("GCI_Cancel_btn")->clicked.connect(slot(this, &GCIDlg::on_Cancel_clicked));

     // Display
     show();
}

GCIDlg::~GCIDlg()
{
     delete _RosterView;
}

void GCIDlg::push_drag_data(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time)
{
     _RosterView->push_drag_data(drag_ctx, x, y, data, info, time);
}

void GCIDlg::add_jid(const string& jid, const string& nickname)
{
     _RosterView->add_jid(jid, nickname);
}

void GCIDlg::on_Remove_clicked()
{
     if (_ctreePeople->selection().begin() != _ctreePeople->selection().end())
     {
          Gtk::CTree::Row& r = *_ctreePeople->selection().begin();
          Roster::ItemMap::iterator it = _roster.find(r[_RosterView->_colJID].get_text());
          if (it != _roster.end())
          {
               _roster.erase(it);
               _RosterView->refresh();
          }
     }
}

void GCIDlg::on_Invite_clicked()
{
     string subject = toUTF8(_entSubject, _entSubject->get_text());
     for (Roster::ItemMap::iterator it = _roster.begin(); it != _roster.end(); it++)
     {
	  Message m(it->first, substitute(toUTF8(_("You have been invited to %s")), _roomjid), Message::mtNormal);
	  if (!subject.empty())
               m.setSubject(subject);
	  Element* x = m.addX("jabber:x:conference");
	  x->putAttrib("jid", _roomjid);
	  G_App->getSession() << m;
     }
     
     // Under some circumstances, we may not already be in that group chat room
     if (_join_self)
     {
	  // so we attempt to join with the default nickname
	  GCMessageView::join(_roomjid + "/" + G_App->getCfg().get_nick());
     }
     close();
}

void GCIDlg::on_Cancel_clicked()
{
     close();
}

/////////////////////////////////////////////////////
// Groupchat Invite Receive View
/////////////////////////////////////////////////////

// Init function to register view with MessageManager
void GCIRecvView::init_view(MessageManager& mm)
{
     mm.register_view_type("jabber:x:conference", MessageManager::MessageViewInfo::vfExtension,
                           new_view_msg, new_view_jid, "glade-gci.xpm", "#669999");
}
 
MessageView* GCIRecvView::new_view_msg(const Message& m, ViewMap& vm)
{
     MessageView* mv = manage(new GCIRecvView(m, vm));
     return mv;
}
 
MessageView* GCIRecvView::new_view_jid(const string& jid, ViewMap& vm)
{
     MessageView* mv = manage(new GCIRecvView(jid, vm));
     return mv;
}

GCIRecvView::GCIRecvView(const jabberoo::Message& m, ViewMap& vm)
     : MessageView(m, vm), BaseGabberDialog("GCIRecv_dlg")
{
     init(m.getFrom());

     render(m);
}

GCIRecvView::GCIRecvView(const string& jid, ViewMap& vm)
     : MessageView(jid, vm), BaseGabberDialog("GCIRecv_dlg")
{
     init(jid);
}

GCIRecvView::~GCIRecvView()
{
}

void GCIRecvView::init(const string& jid)
{
     // Grab the widgets
     _lblSubject  = getLabel("GCIRecv_Subject_lbl");
     _lblRoom     = getLabel("GCIRecv_Room_lbl");
     _lblServer   = getLabel("GCIRecv_Server_lbl");
     _gentNick    = getGEntry("GCIRecv_Nick_gent");
     _entNick     = getEntry("GCIRecv_Nick_ent");

     PrettyJID* pj = manage(new PrettyJID(jid, "", PrettyJID::dtNickRes));
     pj->show();
     getWidget<Gtk::HBox>("GCIRecv_JIDInfo_hbox")->pack_start(*pj, true, true, 0);

     _thisWindow->set_title(_("Group Chat Invitation from ") + fromUTF8(_thisWindow, pj->get_nickname()) + _(" - Gabber"));

     // Load the history for gnome entries
     G_App->getCfg().loadEntryHistory(_gentNick);
     // Set the default nick
     _entNick->set_text(fromUTF8(_entNick, G_App->getCfg().get_nick()));

     getButton("GCIRecv_JoinRoom_btn")->clicked.connect(slot(this, &GCIRecvView::on_JoinRoom_clicked));
     getButton("GCIRecv_Cancel_btn")->clicked.connect(slot(this, &GCIRecvView::on_Cancel_clicked));
}

void GCIRecvView::render(const Message& m)
{
     Element* x = m.findX("jabber:x:conference");
     if (!x)
     {
	  cerr << "Got message with no jabber:x:conference in GCIRecvView?!?!?" << endl;
	  return;
     }
     string jid = x->getAttrib("jid");
     string room = JID::getUser(jid);
     // It's a IRC channel
     string::size_type percent = room.find('%');
     if (percent != string::npos)
	  _lblRoom->set_text(substitute(_("%s on IRC Server %s"),
					fromUTF8(_lblRoom, room.substr(0, percent)),  
					fromUTF8(_lblRoom, room.substr(percent + 1))));
     else
	  _lblRoom->set_text(fromUTF8(_lblRoom, room));

     _lblServer->set_text(fromUTF8(_lblServer, JID::getHost(jid)));
     _lblSubject->set_text(fromUTF8(_lblSubject, m.getSubject()));
     _roomjid = jid;
}

void GCIRecvView::on_JoinRoom_clicked()
{
     string nickname = toUTF8(_entNick, _entNick->get_text());
     string room = _roomjid + "/" + nickname;
     GCMessageView::join(room);
     // Save the gnome entry history
     G_App->getCfg().saveEntryHistory(_gentNick);
     close();
}

void GCIRecvView::on_Cancel_clicked()
{
     close();
}
