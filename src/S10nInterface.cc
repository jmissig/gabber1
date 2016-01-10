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

#include "GabberConfig.hh" // for _()

#include "S10nInterface.hh"

#include "AddContactDruid.hh"
#include "ContactInfoInterface.hh"
#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "MessageManager.hh"

#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-window-icon.h>

using namespace jabberoo;
using namespace GabberUtil;

// ---------------------------------------------------------
//
// Subscription Request Received Dialog
//
// ---------------------------------------------------------

S10nReceiveDlg::~S10nReceiveDlg()
{}

S10nReceiveDlg::S10nReceiveDlg(const Presence& p)
     : BaseGabberDialog("SubscriptionRequest_dlg"),
       _Info(p)
{
     main_dialog(_thisDialog);

     // Setup button handlers
     getButton("SubscriptionRequest_UserInfo_btn")->clicked.connect(slot(this, &S10nReceiveDlg::on_UserInfo_clicked));
     getButton("SubscriptionRequest_Yes_btn")->clicked.connect(slot(this, &S10nReceiveDlg::on_Yes_clicked));
     getButton("SubscriptionRequest_No_btn")->clicked.connect(slot(this, &S10nReceiveDlg::on_No_clicked));
     getButton("SubscriptionRequest_MessageUser_btn")->clicked.connect(slot(this, &S10nReceiveDlg::on_message_user));

     // Initialize JabberID field
     Gtk::Label* l = getLabel("SubscriptionRequest_JID_lbl");
     l->set_text(fromUTF8(l, JID::getUserHost(p.getFrom())));

     // Determine nickname (if possible)
     string nickname;
     Gtk::CheckButton* chk = getCheckButton("SubscriptionRequest_AddUser_chk");
     try {
	  nickname = G_App->getSession().roster()[JID::getUserHost(p.getFrom())].getNickname();
	  chk->hide();
	  chk->set_active(false);
     } catch (Roster::XCP_InvalidJID& e) {
	  nickname = JID::getUser(p.getFrom());
	  chk->show();
     }
     l = getLabel("SubscriptionRequest_Nickname_lbl");
     l->set_text(fromUTF8(l, nickname));

     // Initialize instructions
     getLabel("SubscriptionRequest_Instructions_lbl")->set_text(_("The following user would like to add you to their contact list. Do you wish to allow this?"));

     // Display status area (if necessary)
     string requestmsg = p.getStatus();
     if (!requestmsg.empty())
     {
	  // Display status message
	  Gtk::Editable* mbox = getWidget<Gtk::Editable>("SubscriptionRequest_Message_txt");
	  int i = 0;
	  mbox->insert_text(fromUTF8(mbox, requestmsg).c_str(), fromUTF8(mbox, requestmsg).length(), &i);
	  // Ensure frame is displayed
	  getWidget<Gtk::Frame>("SubscriptionRequest_Request_frm")->show();
     }

     // Pixmaps
     string pix_path = ConfigManager::get_PIXPATH();
     string window_icon = pix_path + "gnome-s10n.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();

     // Display
     show();
}

void S10nReceiveDlg::execute(const Presence& p)
{
     manage(new S10nReceiveDlg(p)); 
}

void S10nReceiveDlg::on_Yes_clicked()
{
     Session& s    = G_App->getSession();
     // Approve the subscription request
     s << Presence(_Info.getFrom(), Presence::ptSubscribed);
     // Add the customized roster entry
     //s.getRoster_NC() << Roster::Item(_Info.getFrom(), getEntry("SubscriptionRequest_Nickname_txt")->get_text());
     if (getCheckButton("SubscriptionRequest_AddUser_chk")->get_active())
	  AddContactDruid::display(JID::getUserHost(_Info.getFrom())); // We don't want to subscribe to their resource...
     close();
}

void S10nReceiveDlg::on_No_clicked()
{
     // Send denial 
     G_App->getSession() << Presence(_Info.getFrom(), Presence::ptUnsubscribed);
     close();
}

void S10nReceiveDlg::on_UserInfo_clicked()
{
     Roster::Subscription type;
     // Try getting the subscription
     try {
	  type = G_App->getSession().roster()[JID::getUserHost(_Info.getFrom())].getSubsType();
     } catch (Roster::XCP_InvalidJID& e) {
	  type = Roster::rsNone;
     }
     // Pop up user info dialog
     ContactInfoDlg::display(_Info.getFrom(), type);
}

void S10nReceiveDlg::on_message_user()
{
     // Pull up a message dialog for this user
     G_App->getMessageManager().display(JID::getUserHost(_Info.getFrom()), MessageManager::translateType("normal"));
}
