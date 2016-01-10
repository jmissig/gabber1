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

#include "ContactInterface.hh"

#include "GabberConfig.hh" // for _()

#include "ContactInfoInterface.hh"
#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "GabberWidgets.hh"

#include <libgnome/gnome-i18n.h>

using namespace jabberoo;
using namespace GabberUtil;

// Init function to register view with MessageManager
void ContactRecvView::init_view(MessageManager& mm)
{
     mm.register_view_type("jabber:x:roster", MessageManager::MessageViewInfo::vfExtension,
                           new_view_msg, new_view_jid, "glade-contact.xpm", "#999966");
}
 
MessageView* ContactRecvView::new_view_msg(const Message& m, ViewMap& vm)
{
     MessageView* mv = manage(new ContactRecvView(m, vm));
     return mv;
}
 
MessageView* ContactRecvView::new_view_jid(const string& jid, ViewMap& vm)
{
     MessageView* mv = manage(new ContactRecvView(jid, vm));
     return mv;
}

ContactRecvView::ContactRecvView(const jabberoo::Message& m, ViewMap& vm)
     : MessageView(m, vm), BaseGabberDialog("ContactRecv_dlg") 
{
     init(m.getFrom());

     render(m);
}

ContactRecvView::ContactRecvView(const string& jid, ViewMap& vm)
     : MessageView(jid, vm), BaseGabberDialog("ContactRecv_dlg")
{
     init(jid);
}

void ContactRecvView::init(string jid)
{
     _ctreePeople = getWidget<Gtk::CTree>("ContactRecv_People_ctree");
     Gtk::ScrolledWindow* tsw = getWidget<Gtk::ScrolledWindow>("ContactRecv_People_scroll");
     _RosterView = new SimpleRosterView(_ctreePeople, tsw, _roster, false); // Set arg 3 to true when groups are supported
     _RosterView->show_jid();

     getButton("ContactRecv_ViewInfo_btn")->clicked.connect(slot(this, &ContactRecvView::on_ViewInfo_clicked));
     getButton("ContactRecv_Remove_btn")->clicked.connect(slot(this, &ContactRecvView::on_Remove_clicked));
     getButton("ContactRecv_Add_btn")->clicked.connect(slot(this, &ContactRecvView::on_Add_clicked));
     getButton("ContactRecv_Cancel_btn")->clicked.connect(slot(this, &ContactRecvView::on_Cancel_clicked));

     PrettyJID* pj = manage(new PrettyJID(_jid, "", PrettyJID::dtNickRes));
     pj->show();
     getWidget<Gtk::HBox>("ContactRecv_JIDInfo_hbox")->pack_start(*pj, true, true, 0);

     _thisWindow->set_title(substitute(_("Received Contacts(s) from %s"), fromUTF8(pj->get_nickname())) + _(" - Gabber"));
}

ContactRecvView::~ContactRecvView()
{
     delete _RosterView;
}

void ContactRecvView::render(const jabberoo::Message& m)
{
     Element* x = m.findX("jabber:x:roster");
     if (!x)
     {
	  cerr << "Got a Message without a roster extension in ContactRecvView::render?!?!" << endl;
	  return;
     }
     Element::const_iterator it = x->begin();
     for (; it != x->end(); it++)
     {
	  if ((*it)->getType() != Node::ntElement)
	       continue;
	  Element& item = *static_cast<Element*>(*it);

	  if (item.getName() != "item")
	       continue;
	  string jid = item.getAttrib("jid");
	  string nick = item.getAttrib("name");
	  cerr << "Got roster item " << jid << " " << nick << endl;
	  Roster::Item ritem(jid, nick);
	  Element::const_iterator git = item.begin();
	  for ( ; git != item.end(); git++)
	  {
	       if ((*it)->getType() != Node::ntElement)
		    continue;
	       Element& group = *static_cast<Element*>(*git);

	       if (group.getName() != "group")
		    continue;

	       ritem.addToGroup(group.getCDATA());
	       cerr << "Got group " << group.getCDATA() << endl;
	  }
	  if (ritem.empty())
	       ritem.addToGroup("Unfiled");
	  _roster.insert(make_pair(jid, ritem));
     }
     _RosterView->refresh();
}

void ContactRecvView::on_Remove_clicked()
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

void ContactRecvView::on_ViewInfo_clicked()
{
     if (_ctreePeople->selection().begin() != _ctreePeople->selection().end())
     {
          Gtk::CTree::Row& r = *_ctreePeople->selection().begin();
          // Check the roster for the jid to make sure it isn't a group
          Roster::ItemMap::iterator it = _roster.find(r[_RosterView->_colJID].get_text());
          if (it != _roster.end())
          {
	       Roster::Subscription type;
	       // Try getting the subscription
	       try {
		    type = G_App->getSession().roster()[JID::getUserHost(it->first)].getSubsType();
	       } catch (Roster::XCP_InvalidJID& e) {
		    type = Roster::rsNone;
	       }
               ContactInfoDlg::display(it->first, type);
          }
     }
}

void ContactRecvView::on_Add_clicked()
{
     for (Roster::ItemMap::iterator it = _roster.begin(); it != _roster.end(); it++)
     {
	  // If the JID is not already on the roster, add it
	  if (!G_App->getSession().roster().containsJID(JID::getUserHost(it->first)))
	  {
	       G_App->getSession().roster() << Roster::Item(JID::getUserHost(it->first), it->second.getNickname());
	       G_App->getSession() << Presence(JID::getUserHost(it->first), Presence::ptSubRequest);
	  }
     }
     close();
}

void ContactRecvView::on_Cancel_clicked()
{
     close();
}

///////////////////////////////////////////////////
// Contact Send Dialog
///////////////////////////////////////////////////

ContactSendDlg::ContactSendDlg(const string& jid)
     : BaseGabberDialog("ContactSend_dlg"),
       _jid(jid)
{
     _ctreePeople = getWidget<Gtk::CTree>("ContactSend_People_ctree");
     Gtk::ScrolledWindow* tsw = getWidget<Gtk::ScrolledWindow>("ContactSend_People_scroll");
     _RosterView = new SimpleRosterView(_ctreePeople, tsw, _roster, false); // Set arg 3 to true when groups are supported
     _RosterView->show_jid();

     _pjid = manage(new PrettyJID(_jid, "", PrettyJID::dtNick, 128, true));
     _pjid->show();
     getWidget<Gtk::HBox>("ContactSend_JIDInfo_hbox")->pack_start(*_pjid, true, true, 0);

     _thisWindow->set_title(substitute(_("Send Contacts(s) to %s"), fromUTF8(_pjid->get_nickname())) + _(" - Gabber"));

     getButton("ContactSend_ViewInfo_btn")->clicked.connect(slot(this, &ContactSendDlg::on_ViewInfo_clicked));
     getButton("ContactSend_Remove_btn")->clicked.connect(slot(this, &ContactSendDlg::on_Remove_clicked));
     getButton("ContactSend_Send_btn")->clicked.connect(slot(this, &ContactSendDlg::on_Send_clicked));
     getButton("ContactSend_Cancel_btn")->clicked.connect(slot(this, &ContactSendDlg::on_Cancel_clicked));
}

ContactSendDlg::~ContactSendDlg()
{
     delete _RosterView;
}

void ContactSendDlg::push_drag_data(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time)
{
     _RosterView->push_drag_data(drag_ctx, x, y, data, info, time);
}

void ContactSendDlg::on_ViewInfo_clicked()
{
     if (_ctreePeople->selection().begin() != _ctreePeople->selection().end())
     {
          Gtk::CTree::Row& r = *_ctreePeople->selection().begin();
	  // Check the roster for the jid to make sure it isn't a group
          Roster::ItemMap::iterator it = _roster.find(r[_RosterView->_colJID].get_text());
          if (it != _roster.end())
          {
	       ContactInfoDlg::display(it->first);
	  }
     }
}

void ContactSendDlg::on_Remove_clicked()
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

void ContactSendDlg::on_Send_clicked()
{
     string body = toUTF8(_("Jabber Contacts attached:"));
     // Walk the list and add the JIDs to the message
     for (Roster::ItemMap::iterator it = _roster.begin(); it != _roster.end(); it++)
     {
	  Roster::Item& ritem = it->second;
	  body += "\n" + ritem.getNickname() + " - " + "jabber:" + ritem.getJID();
     }

     Message m(_pjid->get_full_jid(), body, Message::mtNormal);
     Element* x = m.addX("jabber:x:roster");
     // Walk the list and attach the JIDs
     for (Roster::ItemMap::iterator it = _roster.begin(); it != _roster.end(); it++)
     {
	  Roster::Item& ritem = it->second;

	  Element* item = x->addElement("item");
	  item->putAttrib("jid", ritem.getJID());
	  item->putAttrib("name", ritem.getNickname());
	  for (Roster::Item::iterator git = ritem.begin(); git != ritem.end(); git++)
	       item->addElement("group", *git);
     }
     G_App->getSession() << m;
     close();
}

void ContactSendDlg::on_Cancel_clicked()
{
     close();
}
