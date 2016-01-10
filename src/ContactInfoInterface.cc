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

#include "ContactInfoInterface.hh"

#include "AddContactDruid.hh"
#include "AgentInterface.hh"
#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "GabberWidgets.hh"
#include "GabberGPG.hh"

#include "gtkspell.h"
#include "gtkurl.h"

#include <libgnome/gnome-help.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-window-icon.h>
#include <gtk--/table.h>
#include <gnome--/href.h>
#include <gnome--/dateedit.h>

using namespace judo;
using namespace jabberoo;
using namespace GabberUtil;

// ---------------------------------------------------------
//
// Contact Information Dialog
//
// ---------------------------------------------------------

void ContactInfoDlg::display(const string& jid, Gtk::Window* parentWin)
{
     if (G_App->isAgent(jid))
     {
	  manage(new AgentInfoDlg(jid));
	  return;
     }
     ContactInfoDlg* e = manage(new ContactInfoDlg(jid, parentWin));
     // Most of the time, they're on gabber's roster
     e->_onroster = true;
}

void ContactInfoDlg::display(const string& jid, const Roster::Subscription& type, Gtk::Window* parentWin)
{
     if (G_App->isAgent(jid))
     {
	  manage(new AgentInfoDlg(jid));
	  return;
     }
     ContactInfoDlg* e = manage(new ContactInfoDlg(jid, type, parentWin));
     // In some cases, such as when called from the s10n request dlg,
     // the user isn't on gabber's roster. We don't want to call anything
     // which might crash because of that
     e->_onroster = false;
}

ContactInfoDlg::ContactInfoDlg(const string& jid, Gtk::Window* parentWin)
     : BaseGabberDialog("ContactInfo_dlg"),
       _pix_path(ConfigManager::get_PIXPATH()),
       _share_dir(ConfigManager::get_SHAREDIR()),
       _item(G_App->getSession().roster()[JID::getUserHost(jid)]),
       _jid(jid),
       _parentWin(parentWin)
{
     cerr << "ContactInfo: " << jid << endl;
     init();

     // Get basic info
     _entNickname->set_text(fromUTF8(_entNickname, _item.getNickname()));

     // Load the subscription image
     string pix_s10n;
     switch (_item.getSubsType())
     {
     case Roster::rsBoth:
     case Roster::rsTo:
	  pix_s10n = _share_dir + "glade-s10n.xpm";
	  break;
     case Roster::rsFrom:
     case Roster::rsNone:
     default:
	  pix_s10n = _pix_path + "stalker.xpm";
     }
     _pixS10n->load(pix_s10n);

     // If they're pending, show the pending information
     if (_item.isPending())
	  getWidget<Gtk::Table>("ContactInfo_Pending_tbl")->show();
     else
	  getWidget<Gtk::Table>("ContactInfo_Pending_tbl")->hide();

     getLabel("ContactInfo_s10nType_lbl")->set_text(getS10nName(_item.getSubsType()));
     _tips.set_tip(*_evtS10n, getS10nName(_item.getSubsType()));
     getLabel("ContactInfo_s10nE9n_lbl")->set_text(getS10nInfo(_item.getSubsType()));

     send_vcard_request();
     show();
}

ContactInfoDlg::ContactInfoDlg(const string& jid, const Roster::Subscription& type, Gtk::Window* parentWin)
     : BaseGabberDialog("ContactInfo_dlg"),
       _pix_path(ConfigManager::get_PIXPATH()),
       _share_dir(ConfigManager::get_SHAREDIR()),
       _item(jid, jid),
       _jid(jid)
{
	_parentWin = parentWin;
     // _item is unused in this case

     init();

     // Load the subscription image
     string pix_s10n;
     switch (type)
     {
     case Roster::rsBoth:
     case Roster::rsTo:
	  pix_s10n = _share_dir + "glade-s10n.xpm";
	  break;
     case Roster::rsFrom:
     case Roster::rsNone:
     default:
	  pix_s10n = _pix_path + "stalker.xpm";
     }
     _pixS10n->load(pix_s10n);

     getLabel("ContactInfo_s10nType_lbl")->set_text(getS10nName(type));
     _tips.set_tip(*_evtS10n, getS10nName(type));
     getLabel("ContactInfo_s10nE9n_lbl")->set_text(getS10nInfo(type));

     send_vcard_request();
     show();
}

void ContactInfoDlg::init()
{
     // Set parent window
     if (_parentWin == NULL)
	  main_dialog(_thisDialog);
     else 
	  _thisDialog->set_parent(*_parentWin);

     // Setup buttons
     getButton("ContactInfo_OK_btn")->clicked.connect(slot(this, &ContactInfoDlg::on_ok_clicked));
     getButton("ContactInfo_Cancel_btn")->clicked.connect(slot(this, &ContactInfoDlg::on_cancel_clicked));
     getButton("ContactInfo_Help_btn")->clicked.connect(slot(this, &ContactInfoDlg::on_help_clicked));
     getButton("ContactInfo_Nickname_Default_btn")->clicked.connect(slot(this, &ContactInfoDlg::on_default_clicked));

     // Get pointers
     _entNickname = getEntry("ContactInfo_Nickname_txt");
     _entNickname ->changed.connect(slot(this, &ContactInfoDlg::on_nickname_changed));
     _lblShow     = getLabel("ContactInfo_Show_lbl");
     _lblStatus   = getLabel("ContactInfo_Status_lbl");
     _pixShow     = getWidget<Gnome::Pixmap>("ContactInfo_Show_pix");
     _evtShow     = getWidget<Gtk::EventBox>("ContactInfo_Show_evt");
     _pixS10n     = getWidget<Gnome::Pixmap>("ContactInfo_s10nType_pix");
     _evtS10n     = getWidget<Gtk::EventBox>("ContactInfo_s10nType_evt");
     _gpgInfo     = GabberGPG::gpgNone;
     getWidget<Gtk::EventBox>("ContactInfo_GPG_evt")->button_press_event.connect(slot(this, &ContactInfoDlg::on_GPGInfo_press_event));

     gtkurl_attach(getWidget<Gtk::Text>("ContactInfo_About_txt")->gtkobj());

     _pjid = manage(new PrettyJID(_jid, "", PrettyJID::dtJIDRes, 128, true));
     _pjid->show_pixmap(false);
     _pjid->changed.connect(slot(this, &ContactInfoDlg::on_PrettyJID_changed));
     // This is called since signal was connected after _jid was set
     on_PrettyJID_changed();
     _pjid->show();
     getWidget<Gtk::HBox>("ContactInfo_JIDInfo_hbox")->pack_start(*_pjid, true, true, 0);

     // Pixmaps
     string window_icon = _pix_path + "gnome-userinfo.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();
}

void ContactInfoDlg::send_vcard_request()
{
     // Get next session ID
     string id = G_App->getSession().getNextID();

     // Construct vCard request
     Packet iq("iq");
     iq.setID(id);
     iq.setTo(_jid);
     iq.getBaseElement().putAttrib("type", "get");
     Element* vCard = iq.getBaseElement().addElement("vCard");
     vCard->putAttrib("xmlns", "vcard-temp");
     vCard->putAttrib("version", "2.0");
     vCard->putAttrib("prodid", "-//HandGen//NONSGML vGen v1.0//EN");

     // Send the vCard request
     G_App->getSession() << iq;
     G_App->getSession().registerIQ(id, slot(this, &ContactInfoDlg::parse_vcard));
}

void ContactInfoDlg::set_info_label(const string& label_name, const string& frame_name, const string& tab_name, const string& data)
{
     if (!data.empty())
     {
	  Gtk::Label* l = getLabel(label_name.c_str());
	  l->set_text(fromUTF8(l, data));
	  l->show();
	  getLabel(string(label_name + "_lbl").c_str())->show();
	  getFrame(frame_name.c_str())->show();
	  getLabel(tab_name.c_str())->set_sensitive(true);
     }
}

void ContactInfoDlg::parse_vcard(const Element& t)
{
     if (!t.empty())
     {
	  string curtext;
	  const Element* vCard = t.findElement("vCard");
	  if (vCard != NULL)
	  {
	       curtext = vCard->getChildCData("NICKNAME");
	       string nickname = toUTF8(_entNickname, _entNickname->get_text());
	       if (nickname.empty())
		    _entNickname->set_text(fromUTF8(_entNickname, curtext));
	       // Save the old nickname
	       _oldnick = curtext;
	       // Allow them to revert to the old nickname if there actually is one
	       getButton("ContactInfo_Nickname_Default_btn")->set_sensitive(!_oldnick.empty() && nickname != _oldnick);

	       set_info_label("ContactInfo_FullName_lbl",
			      "ContactInfo_PersInfo_frm",
			      "ContactInfo_UserInfo_lbl",
			      vCard->getChildCData("FN"));
	       set_info_label("ContactInfo_eMail_lbl",
			      "ContactInfo_PersInfo_frm",
			      "ContactInfo_UserInfo_lbl",		      
			      vCard->getChildCData("EMAIL"));
	       set_info_label("ContactInfo_Birthday_lbl",
			      "ContactInfo_Birthday_frm",
			      "ContactInfo_About_lbl",
			      vCard->getChildCData("BDAY"));

	       curtext = vCard->getChildCData("URL");
	       if (!curtext.empty())
	       {
		    Gnome::HRef* hrefWeb = getWidget<Gnome::HRef>("ContactInfo_WebSite_href");
		    hrefWeb->set_url(fromUTF8(hrefWeb, curtext));
		    hrefWeb->set_label(fromUTF8(hrefWeb, curtext));
		    getHBox("ContactInfo_WebSite_hbox")->show();
		    getLabel("ContactInfo_WebSite_lbl")->show();
	       }

	       set_info_label("ContactInfo_HomePhone_lbl",
			      "ContactInfo_PersInfo_frm",
			      "ContactInfo_UserInfo_lbl",
			      vCard->getChildCData("TEL"));

	       const Element* ADR = vCard->findElement("ADR");
	       if (ADR != NULL)
	       {
		    set_info_label("ContactInfo_Street_lbl",
				   "ContactInfo_Location_frm",
				   "ContactInfo_Location_lbl",
				   ADR->getChildCData("STREET"));
		    set_info_label("ContactInfo_Extadd_lbl",
				   "ContactInfo_Location_frm",
				   "ContactInfo_Location_lbl",
				   ADR->getChildCData("EXTADD"));
		    set_info_label("ContactInfo_City_lbl",
				   "ContactInfo_Location_frm",
				   "ContactInfo_Location_lbl",
				   ADR->getChildCData("LOCALITY"));
		    set_info_label("ContactInfo_State_lbl",
				   "ContactInfo_Location_frm",
				   "ContactInfo_Location_lbl",
				   ADR->getChildCData("REGION"));
		    set_info_label("ContactInfo_PCode_lbl",
				   "ContactInfo_Location_frm",
				   "ContactInfo_Location_lbl",
				   ADR->getChildCData("PCODE"));
		    set_info_label("ContactInfo_Country_lbl",
				   "ContactInfo_Location_frm",
				   "ContactInfo_Location_lbl",
				   ADR->getChildCData("COUNTRY"));
	       }
	       const Element* ORG = vCard->findElement("ORG");
	       if (ORG != NULL)
	       {
		    set_info_label("ContactInfo_OrgName_lbl",
				   "ContactInfo_OrgDetails_frm",
				   "ContactInfo_Org_lbl",
				   ORG->getChildCData("ORGNAME"));
		    set_info_label("ContactInfo_OrgUnit_lbl",
				   "ContactInfo_OrgDetails_frm",
				   "ContactInfo_Org_lbl",
				   ORG->getChildCData("ORGUNIT"));
	       }
	       set_info_label("ContactInfo_Title_lbl",
			      "ContactInfo_OrgPersDetails_frm",
			      "ContactInfo_Org_lbl",
			      vCard->getChildCData("TITLE"));
	       set_info_label("ContactInfo_Role_lbl",
			      "ContactInfo_OrgPersDetails_frm",
			      "ContactInfo_Org_lbl",
			      vCard->getChildCData("ROLE"));

	       // Description, aka About
	       curtext = vCard->getChildCData("DESC");
	       Gtk::Text* t = getWidget<Gtk::Text>("ContactInfo_About_txt");
	       t->set_word_wrap(true);
	       int i = 0;
	       if (curtext.length() > 0)
	       {
		    t->insert_text(fromUTF8(t, curtext).c_str(),
				   fromUTF8(t, curtext).length(), &i);
		    gtkurl_check_all(t->gtkobj());
		    // Hrmm, this doesn't seem to scroll it back to the beginning
		    t->set_point(0);
		    getFrame("ContactInfo_About_frm")->show();
		    getLabel("ContactInfo_About_lbl")->set_sensitive(true);
	       }
	  }
     }
}

void ContactInfoDlg::send_last_request()
{
     // Get next session ID
     string id = G_App->getSession().getNextID();

     // Construct version request
     Packet iq("iq");
     iq.setID(id);
     iq.setTo(_pjid->get_full_jid());
     iq.getBaseElement().putAttrib("type", "get");
     Element* vCard = iq.getBaseElement().addElement("query");
     vCard->putAttrib("xmlns", "jabber:iq:last");

     // Send the version request
     G_App->getSession() << iq;
     G_App->getSession().registerIQ(id, slot(this, &ContactInfoDlg::parse_last));
}

void ContactInfoDlg::parse_last(const Element& t)
{
     if (!t.empty())
     {
	  int seconds;
	  string lasttime;
	  const Element* query = t.findElement("query");
	  if (query != NULL)
	  {
	       // Grab the idle or last logged out time
	       seconds = atoi(query->getAttrib("seconds").c_str());
	       if (seconds == 0)
		    return;
	       else if (_last_logout)
		    lasttime = "(last logged out";
	       else 
		    lasttime = "(idle";
	       // Conversion magic
	       int mins, hrs, days, secs;
	       days = (seconds / (3600 * 24));
	       hrs  = ((seconds / 3600) - (days * 24));
	       mins = ((seconds / 60) - (days * 24 * 60) - (hrs * 60));
	       secs = (seconds - (days * 24 * 60 * 60) - (hrs * 60 * 60) - (mins * 60));
	       char *tdays, *thrs, *tmins, *tsecs;
	       tdays = g_strdup_printf("%d", days);
	       thrs  = g_strdup_printf("%d", hrs);
	       tmins = g_strdup_printf("%d", mins);
	       tsecs = g_strdup_printf("%d", secs);

	       // Figure out the exact text to display
	       bool prevt;
	       if (string(tdays) == "1")
	       {
		    lasttime += " " + string(tdays) + _(" day");
		    prevt = true;
	       }
	       else if (string(tdays) != "0")
	       {
		    lasttime += " " + string(tdays) + _(" days");
		    prevt = true;
	       }
	       else
		    prevt = false;
	       g_free(tdays);

	       if (string(thrs) == "1")
	       {
		    if (prevt)
			 lasttime += ",";
		    lasttime += " " + string(thrs) + _(" hr");
		    prevt = true;
	       }
	       else if (string(thrs) != "0")
	       {
		    if (prevt)
			 lasttime += ",";
		    lasttime += " " + string(thrs) + _(" hrs");
		    prevt = true;
	       }
	       else
		    prevt = false;
	       g_free(thrs);
	       
	       if (string(tmins) == "1")
	       {
		    if (prevt)
			 lasttime += ",";
		    lasttime += " " + string(tmins) + _(" min");
		    prevt = true;
	       }
	       else if (string(tmins) != "0")
	       {
		    if (prevt)
			 lasttime += ",";
		    lasttime += " " + string(tmins) + _(" mins");
		    prevt = true;
	       }
	       else
		    prevt = false;
	       g_free(tmins);

	       if (!prevt)
	       {
		    if (string(tsecs) == "1")
			 lasttime += " " + string(tsecs) + _(" sec");
		    else if (string(tsecs) != "0")
			 lasttime += " " + string(tsecs) + _(" secs");
	       }
	       g_free(tsecs);

	       if (_last_logout)
		    lasttime += " ago";
	       lasttime += ")";

	       string status = fromUTF8(_lblStatus, t.getChildCData("query")); // Get the status from iq:last
	       if (_has_presence || status.empty())       // unless we already have status
		    status = _lblStatus->get_text();

	       _lblStatus->set_text(lasttime + " " + status);
	       _lblStatus->show();
	  }
     }
}

void ContactInfoDlg::send_version_request()
{
     // Get next session ID
     string id = G_App->getSession().getNextID();

     // Construct version request
     Packet iq("iq");
     iq.setID(id);
     iq.setTo(_pjid->get_full_jid());
     iq.getBaseElement().putAttrib("type", "get");
     Element* vCard = iq.getBaseElement().addElement("query");
     vCard->putAttrib("xmlns", "jabber:iq:version");

     // Send the version request
     G_App->getSession() << iq;
     G_App->getSession().registerIQ(id, slot(this, &ContactInfoDlg::parse_version));
}

void ContactInfoDlg::parse_version(const Element& t)
{
     if (!t.empty())
     {
	  const Element* query = t.findElement("query");
	  if (query != NULL)
	  {
	       set_info_label("ContactInfo_Client_lbl",
			      "ContactInfo_JabberClient_frm",
			      "ContactInfo_ClientInfo_lbl",
			      query->getChildCData("name"));
	       set_info_label("ContactInfo_ClientVersion_lbl",
			      "ContactInfo_JabberClient_frm",
			      "ContactInfo_ClientInfo_lbl",
			      query->getChildCData("version"));
	       set_info_label("ContactInfo_OS_lbl",
			      "ContactInfo_Computer_frm",
			      "ContactInfo_ClientInfo_lbl",
			      query->getChildCData("os"));
	  }
     }
}

void ContactInfoDlg::send_time_request()
{
     // Get next session ID
     string id = G_App->getSession().getNextID();

     // Construct version request
     Packet iq("iq");
     iq.setID(id);
     iq.setTo(_pjid->get_full_jid());
     iq.getBaseElement().putAttrib("type", "get");
     Element* vCard = iq.getBaseElement().addElement("query");
     vCard->putAttrib("xmlns", "jabber:iq:time");

     // Send the version request
     G_App->getSession() << iq;
     G_App->getSession().registerIQ(id, slot(this, &ContactInfoDlg::parse_time));
}

void ContactInfoDlg::parse_time(const Element& t)
{
     if (!t.empty())
     {
	  string curtext;
	  const Element* query = t.findElement("query");
	  if (query != NULL)
	  {
	       curtext = query->getChildCData("display");
	       curtext += " "; // Don't duplicate the time zone:
	       if(curtext.find(query->getChildCData("tz"))==string::npos)
		       curtext += query->getChildCData("tz");
	       set_info_label("ContactInfo_Time_lbl",
			      "ContactInfo_Computer_frm",
			      "ContactInfo_ClientInfo_lbl",
			      curtext);
	  }
     }
}

void ContactInfoDlg::get_status()
{
     try {
	  // Get the presence
	  const Presence& p = G_App->getSession().presenceDB().findExact(_pjid->get_full_jid());
	  
	  // Set the labels
	  _lblShow->set_text(getShowName(p.getShow()) + ":");
	  string show_pix = _pix_path + p.getShow_str() + ".xpm";
	  _pixShow->load(show_pix);
	  _lblStatus->set_text(fromUTF8(_lblStatus, p.getStatus()));
	  _tips.set_tip(*_evtShow, getShowName(p.getShow()) + ": " + fromUTF8(_evtShow, p.getStatus()));
	  _has_presence = true;
	  _last_logout = !G_App->getSession().presenceDB().available(_pjid->get_full_jid());

	  // Check if the presence has a signature
	  Element* x = p.findX("jabber:x:signed");
	  GabberGPG& gpg = G_App->getGPG();
          if (x != NULL && gpg.enabled()) 
	  {
	       string encrypted = x->getCDATA();
	       string status = p.getStatus();
	       GPGInterface::SigInfo info;
	       GPGInterface::Error err;

               if ((err = gpg.verify(info, encrypted, status)) != GPGInterface::errOK)
               {
                    if (err == GPGInterface::errPubKey)
                    {
                         cerr << "FIXME: need a way to get users Public Key" << endl;
                    }
		    // The signature was invalid so set the appropriate icon
                    getWidget<Gnome::Pixmap>("ContactInfo_GPG_pix")->load(_share_dir + "gpg-badsigned.xpm");
		    // Set the tooltip as well
		    _tips.set_tip(*getWidget<Gtk::EventBox>("ContactInfo_GPG_evt"), _("Presence signature is invalid"));
		    getLabel("ContactInfo_GPG_lbl")->set_text(_("Presence signature is invalid"));
		    _gpgInfo = GabberGPG::gpgInvalidSigned;
               }
	       else
	       {
		    // Good signature, set the icon
                    getWidget<Gnome::Pixmap>("ContactInfo_GPG_pix")->load(_share_dir + "gpg-signed.xpm");
		    // Set the tooltip as well
		    _tips.set_tip(*getWidget<Gtk::EventBox>("ContactInfo_GPG_evt"), _("Presence signature is valid"));
		    getLabel("ContactInfo_GPG_lbl")->set_text(_("Presence signature is valid"));
		    _gpgInfo = GabberGPG::gpgValidSigned;
	       }
          }
	  else
	  {
	       // No signature, set the icon appropriately
	       getWidget<Gnome::Pixmap>("ContactInfo_GPG_pix")->load(_share_dir + "gpg-unsigned.xpm");
	       // Set the tooltip as well
	       _tips.set_tip(*getWidget<Gtk::EventBox>("ContactInfo_GPG_evt"), _("Presence is not signed"));
	       getLabel("ContactInfo_GPG_lbl")->set_text(_("Presence is not signed"));
	  }
     } 
     catch (PresenceDB::XCP_InvalidJID& e) {
	  // Set the labels appropriately
	  _lblShow->set_text(_("offline:"));
	  string show_pix = _pix_path + "offline.xpm";
	  _pixShow->load(show_pix);
	  _lblStatus->set_text(_("No presence has been received."));
	  _has_presence = false;
	  _last_logout = true;
     }
}

void ContactInfoDlg::on_ok_clicked()
{
     if (_onroster)
     {
	  // Save nickname
	  _item.setNickname(toUTF8(_entNickname, _entNickname->get_text()));
	  
	  // Make sure that the item doesn't actually set any of our virtual groups
	  _item.delFromGroup("Unfiled");
	  _item.delFromGroup("Pending");
	  _item.delFromGroup("Agents");

	  // Update the roster
	  G_App->getSession().roster() << _item;
     }
     close();
}

void ContactInfoDlg::on_cancel_clicked()
{
     close();
}

void ContactInfoDlg::on_help_clicked()
{
     // call the manual
     GnomeHelpMenuEntry help_entry = { "gabber", "users.html#USERS-EDITUSER" };
     gnome_help_display (NULL, &help_entry);
}

void ContactInfoDlg::on_default_clicked()
{
     _entNickname->set_text(fromUTF8(_entNickname, _oldnick));
}

void ContactInfoDlg::on_nickname_changed()
{
     string nickname = toUTF8(_entNickname, _entNickname->get_text());
     if (!nickname.empty())
     {
	  _thisWindow->set_title(substitute(_("%s's Contact Information"), fromUTF8(_thisWindow, nickname)) + _(" - Gabber"));
	  Gtk::Frame* f = getFrame("ContactInfo_Computer_frm");
	  f->set_label(substitute(_("%s's Computer"), fromUTF8(f, nickname)));
     }
     getButton("ContactInfo_OK_btn")->set_sensitive(!nickname.empty());
     getButton("ContactInfo_Nickname_Default_btn")->set_sensitive(!_oldnick.empty() && _oldnick != nickname);
}

void ContactInfoDlg::on_PrettyJID_changed()
{
     // Hide all the client info, since we'll need to re-get it for this other resource
     // No need to reset labels, since they'll only be shown if there's something new to set them to
     getLabel("ContactInfo_ClientInfo_lbl")->set_sensitive(false);
     getFrame("ContactInfo_JabberClient_frm")->hide();
     getLabel("ContactInfo_Client_lbl_lbl")->hide();
     getLabel("ContactInfo_Client_lbl")->hide();
     getLabel("ContactInfo_ClientVersion_lbl_lbl")->hide();
     getLabel("ContactInfo_ClientVersion_lbl")->hide();
     getFrame("ContactInfo_Computer_frm")->hide();
     getLabel("ContactInfo_OS_lbl_lbl")->hide();
     getLabel("ContactInfo_OS_lbl")->hide();
     getLabel("ContactInfo_Time_lbl_lbl")->hide();
     getLabel("ContactInfo_Time_lbl")->hide();

     send_version_request();
     send_time_request();
     send_last_request();
     get_status();
}

int ContactInfoDlg::on_GPGInfo_press_event(GdkEventButton* e)
{
     if (_gpgInfo == GabberGPG::gpgNone)
	  return FALSE;

     bool valid = false;
     if (_gpgInfo == GabberGPG::gpgValidSigned)
	  valid = true;

     try {
	  string keyid = G_App->getGPG().find_jid_key(_item.getJID());
	  GPGInfoDialog* dlg = manage(new GPGInfoDialog(keyid, valid));
	  dlg->getBaseDialog()->set_parent(*_thisWindow);
     } catch (GabberGPG::GPG_InvalidJID& e) {
	  GPGInfoDialog* dlg = manage(new GPGInfoDialog("", false));
	  dlg->getBaseDialog()->set_parent(*_thisWindow);
     }
     return TRUE;
}


// ---------------------------------------------------------
//
// My Contact Information Window
//
// ---------------------------------------------------------

MyContactInfoWin* MyContactInfoWin::_Dialog = NULL;

void MyContactInfoWin::execute()
{
     if (_Dialog == NULL)
	  _Dialog = manage(new MyContactInfoWin());
}

MyContactInfoWin::~MyContactInfoWin()
{
     _Dialog = NULL;
}

MyContactInfoWin::MyContactInfoWin()
     : BaseGabberDialog("MyContactInfo_win")
{
     main_dialog(_thisDialog);

     // Connect the widgets
     getButton("MyContactInfo_OK_btn")->clicked.connect(slot(this, &MyContactInfoWin::on_ok_clicked));
     getButton("MyContactInfo_Help_btn")->clicked.connect(slot(this, &MyContactInfoWin::on_help_clicked));
     getButton("MyContactInfo_Cancel_btn")->clicked.connect(slot(this, &MyContactInfoWin::on_cancel_clicked));

     _thisWindow->delete_event.connect(slot(this, &MyContactInfoWin::on_window_delete));

     // JUD
     _chkNoJUD = getCheckButton("MyContactInfo_NoJUD_chk");
     _chkNoJUD->toggled.connect(slot(this, &MyContactInfoWin::changed));

     // Basic Info
     _entNickname  = getEntry("MyContactInfo_Basic_Nickname_txt");
     _entNickname  ->changed.connect(slot(this, &MyContactInfoWin::changed));
     _entFirstName = getEntry("MyContactInfo_Basic_FirstName_txt");
     _entFirstName ->changed.connect(slot(this, &MyContactInfoWin::on_name_changed));
     _entLastName  = getEntry("MyContactInfo_Basic_LastName_txt");
     _entLastName  ->changed.connect(slot(this, &MyContactInfoWin::on_name_changed));
     _entFullName  = getEntry("MyContactInfo_Basic_FullName_txt");
     _entFullName  ->changed.connect(slot(this, &MyContactInfoWin::changed));
     _entEMail     = getEntry("MyContactInfo_Basic_eMail_txt");
     _entEMail     ->changed.connect(slot(this, &MyContactInfoWin::changed));

     // Extended info
     getEntry("MyContactInfo_Personal_WebSite_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_Personal_HomePhone_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getWidget<Gnome::DateEdit>("MyContactInfo_Personal_dateedit")->date_changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_Address_Street_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_Address_Extadd_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_Address_City_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_Address_State_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_Address_PCode_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_Address_Country_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_OrgName_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_OrgUnit_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_Title_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     getEntry("MyContactInfo_Role_txt")->changed.connect(slot(this, &MyContactInfoWin::changed));
     _txtAbout = getWidget<Gtk::Text>("MyContactInfo_About_txt");
     _txtAbout->set_word_wrap(true);
     _txtAbout ->changed.connect(slot(this, &MyContactInfoWin::changed));
     if (gtkspell_running())
	  gtkspell_attach(_txtAbout->gtkobj()); // Attach gtkspell for spell checking

     // Pixmaps
     string pix_path = ConfigManager::get_PIXPATH();
     string window_icon = pix_path + "gnome-userinfo.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();

     loadconfig();
     show();
}

void MyContactInfoWin::manage_query(const string& jid)
{
     // If we've used the key already
     if (_keyUsed)
     {
// 	  const Agents::ItemMap& m = G_App->getSession().getAgents().getItems();
// 	  const Agents::ItemMap::const_iterator it = m.find(jid);
// 	  if (it != m.end())
// 	  {
// 	       // Get next session ID
// 	       string id = G_App->getSession().getNextID();
// 	       // Get the iq element
// 	       G_App->getSession().registerIQ(id, slot(this, &MyContactInfoWin::getKey));
// 	       // Tell the agent to send the registration request
// 	       it->second.registerAgent(id);
// 	  }
     }
}

void MyContactInfoWin::get_key(const Element& t)
{
     // Set the <key>
     const Element* query = t.findElement("query");
     if (query)
     {
	  _key = query->getChildCData("key");
	  _keyUsed = false;
     }
}

void MyContactInfoWin::get_info(const string& jid)
{
     // Get next session ID
     string id = G_App->getSession().getNextID();

     // Construct vCard request
     Packet iq("iq");
     iq.setID(id);
     iq.setTo(jid);
     iq.getBaseElement().putAttrib("type", "get");
     Element* vCard = iq.getBaseElement().addElement("vCard");
     vCard->putAttrib("xmlns", "vcard-temp");
     vCard->putAttrib("version", "2.0");
     vCard->putAttrib("prodid", "-//HandGen//NONSGML vGen v1.0//EN");

     // Send the vCard request
     G_App->getSession() << iq;
     G_App->getSession().registerIQ(id, slot(this, &MyContactInfoWin::get_vCard));
}

void MyContactInfoWin::get_vCard(const Element& t)
{
     if (!t.empty())
     {
	  string curtext;
	  const Element* vCard = t.findElement("vCard");
	  if (vCard != NULL)
	  {
	       // Nickname
	       curtext = vCard->getChildCData("NICKNAME");
	       _entNickname->set_text(fromUTF8(_entNickname, curtext));
	       // Name
	       const Element* N = vCard->findElement("N");
	       if (N != NULL)
	       {
		    // Family Name
		    curtext = N->getChildCData("FAMILY");
		    _entLastName->set_text(fromUTF8(_entLastName, curtext));
		    // Given Name
		    curtext = N->getChildCData("GIVEN");
		    _entFirstName->set_text(fromUTF8(_entFirstName, curtext));
	       }
	       // Full Name
	       curtext = vCard->getChildCData("FN");
	       _entFullName->set_text(fromUTF8(_entFullName, curtext));
	       // Email
	       curtext = vCard->getChildCData("EMAIL");
	       _entEMail->set_text(fromUTF8(_entEMail, curtext));
	       // Birthday
	       curtext = vCard->getChildCData("BDAY");
	       // We need a way to parse out the birthday and set it if it fits the proper format
	       // Should be in iso 8601 format YYYY-MM-DD (time, which I didn't look at)
	       // Time isn't important for birthday
	       time_t bday = 0;
	       if ((curtext[4] == '-') && (curtext[7] == '-'))
	       {
		    struct tm bday_tm;

		    memset(&bday_tm, 0, sizeof(bday_tm));
		    sscanf(curtext.c_str(), "%d-%d-%d", &bday_tm.tm_year, &bday_tm.tm_mon, &bday_tm.tm_mday);
		    // tm_year is number of years since 1900
		    bday_tm.tm_year -= 1900;
		    // tm_mon goes from 0 to 11
		    bday_tm.tm_mon  -= 1;

		    bday = mktime(&bday_tm);
	       }
	       getWidget<Gnome::DateEdit>("MyContactInfo_Personal_dateedit")->set_time(bday);
	       // Web Site
	       curtext = vCard->getChildCData("URL");
	       Gtk::Entry* e = getEntry("MyContactInfo_Personal_WebSite_txt");
	       e->set_text(fromUTF8(e, curtext));
	       // Phone Number
	       curtext = vCard->getChildCData("TEL");
	       e = getEntry("MyContactInfo_Personal_HomePhone_txt");
	       e->set_text(fromUTF8(e, curtext));
	       // Address
	       const Element* ADR = vCard->findElement("ADR");
	       if (ADR != NULL)
	       {
		    curtext = ADR->getChildCData("STREET");
		    e = getEntry("MyContactInfo_Address_Street_txt");
		    e->set_text(fromUTF8(e, curtext));
		    curtext = ADR->getChildCData("EXTADD");
		    e = getEntry("MyContactInfo_Address_Extadd_txt");
		    e->set_text(fromUTF8(e, curtext));
		    curtext = ADR->getChildCData("LOCALITY");
		    e = getEntry("MyContactInfo_Address_City_txt");
		    e->set_text(fromUTF8(e, curtext));
		    curtext = ADR->getChildCData("REGION");
		    e = getEntry("MyContactInfo_Address_State_txt");
		    e->set_text(fromUTF8(e, curtext));
		    curtext = ADR->getChildCData("PCODE");
		    e = getEntry("MyContactInfo_Address_PCode_txt");
		    e->set_text(fromUTF8(e, curtext));
		    curtext = ADR->getChildCData("COUNTRY");
		    e = getEntry("MyContactInfo_Address_Country_txt");
		    e->set_text(fromUTF8(e, curtext));
	       }
	       const Element* ORG = vCard->findElement("ORG");
	       if (ORG != NULL)
	       {
		    curtext = ORG->getChildCData("ORGNAME");
		    e = getEntry("MyContactInfo_OrgName_txt");
		    e->set_text(fromUTF8(e, curtext));
		    curtext = ORG->getChildCData("ORGUNIT");
		    e = getEntry("MyContactInfo_OrgUnit_txt");
		    e->set_text(fromUTF8(e, curtext));
	       }
	       curtext = vCard->getChildCData("TITLE");
	       e = getEntry("MyContactInfo_Title_txt");
	       e->set_text(fromUTF8(e, curtext));
	       curtext = vCard->getChildCData("ROLE");
	       e = getEntry("MyContactInfo_Role_txt");
	       e->set_text(fromUTF8(e, curtext));
	       // Description, aka About
	       curtext = vCard->getChildCData("DESC");
	       Gtk::Text* t = getWidget<Gtk::Text>("MyContactInfo_About_txt");
	       t->set_word_wrap(true);
	       // Clear the default text
	       t->delete_text(0, -1);
	       int i = 0;
	       // Insert the text we found
	       t->insert_text(fromUTF8(t, curtext).c_str(),
			      fromUTF8(t, curtext).length(), &i);
	       // Hrmm, this doesn't seem to scroll it back to the beginning
	       t->set_point(0);
	  }
     }
}

void MyContactInfoWin::set_info(const string& jid)
{
     // If we haven't already used current key - Only set JUD stuff if they want to
     if (!_keyUsed && !_chkNoJUD->get_active())
     {
	  // - JUD
	  // Generate a query element
	  Element q("query");
	  q.addElement("first", toUTF8(_entFirstName, _entFirstName->get_text()));
	  q.addElement("last", toUTF8(_entLastName, _entLastName->get_text()));
	  q.addElement("nick", toUTF8(_entNickname, _entNickname->get_text()));
	  q.addElement("email", toUTF8(_entEMail, _entEMail->get_text()));
	  q.addElement("key", _key);
	  
	  // Get a session id
	  string id = G_App->getSession().getNextID();
	  
	  // Add necessary flags to the tag
	  q.putAttrib("xmlns", "jabber:iq:register");
	  
	  // Send IQ header
	  //G_App->getSession() << "<iq type='set' id='" << id.c_str() << "' to='" << jid.c_str() << "'>" << q.toString() << "</iq>";

	  // We've set stuff. done.
	  _keyUsed = true;
     }
     // - vCard
     // Get next session ID
     string id = G_App->getSession().getNextID();

     // Construct vCard
     Packet iq("iq");
     iq.setID(id);
     iq.getBaseElement().putAttrib("type", "set");
     Element* vCard = iq.getBaseElement().addElement("vCard");
     vCard->putAttrib("xmlns", "vcard-temp");
     vCard->putAttrib("version", "2.0");
     vCard->putAttrib("prodid", "-//HandGen//NONSGML vGen v1.0//EN");
     vCard->addElement("FN", toUTF8(_entFullName, _entFullName->get_text()));
     Element* N = vCard->addElement("N");
     N->addElement("FAMILY", toUTF8(_entLastName, _entLastName->get_text()));
     N->addElement("GIVEN", toUTF8(_entFirstName, _entFirstName->get_text()));
     vCard->addElement("NICKNAME", toUTF8(_entNickname, _entNickname->get_text()));
     Gtk::Entry* e = getEntry("MyContactInfo_Personal_WebSite_txt");
     vCard->addElement("URL", toUTF8(e, e->get_text()));
     Element* ADR = vCard->addElement("ADR");
     e = getEntry("MyContactInfo_Address_Street_txt");
     ADR->addElement("STREET", toUTF8(e, e->get_text()));
     e = getEntry("MyContactInfo_Address_Extadd_txt");
     ADR->addElement("EXTADD", toUTF8(e, e->get_text()));
     e = getEntry("MyContactInfo_Address_City_txt");
     ADR->addElement("LOCALITY", toUTF8(e, e->get_text()));
     e = getEntry("MyContactInfo_Address_State_txt");
     ADR->addElement("REGION", toUTF8(e, e->get_text()));
     e = getEntry("MyContactInfo_Address_PCode_txt");
     ADR->addElement("PCODE", toUTF8(e, e->get_text()));
     e = getEntry("MyContactInfo_Address_Country_txt");
     ADR->addElement("COUNTRY", toUTF8(e, e->get_text()));
     e = getEntry("MyContactInfo_Personal_HomePhone_txt");
     vCard->addElement("TEL", toUTF8(e, e->get_text()));
     vCard->addElement("EMAIL", toUTF8(_entEMail, _entEMail->get_text()));
     Element* ORG = vCard->addElement("ORG");
     e = getEntry("MyContactInfo_OrgName_txt");
     ORG->addElement("ORGNAME", toUTF8(e, e->get_text()));
     e = getEntry("MyContactInfo_OrgUnit_txt");
     ORG->addElement("ORGUNIT", toUTF8(e, e->get_text()));
     e = getEntry("MyContactInfo_Title_txt");
     vCard->addElement("TITLE", toUTF8(e, e->get_text()));
     e = getEntry("MyContactInfo_Role_txt");
     vCard->addElement("ROLE", toUTF8(e, e->get_text()));
     Gtk::Text* t = getWidget<Gtk::Text>("MyContactInfo_About_txt");
     vCard->addElement("DESC", toUTF8(t, t->get_chars(0, -1)));

     // Why is the Gnome::DateEdit function set_time but get_date??
     time_t bday = getWidget<Gnome::DateEdit>("MyContactInfo_Personal_dateedit")->get_date();
     struct tm *bday_tm = localtime(&bday);
     char bday_str[11];

     strftime(bday_str, 11, "%Y-%m-%d", bday_tm);
     vCard->addElement("BDAY", bday_str);

     // Send vCard
     G_App->getSession() << iq;
}

void MyContactInfoWin::loadconfig()
{
     // Load the current configuration
     ConfigManager& c = G_App->getCfg();

     // JUD
     _chkNoJUD->set_active(c.user.nojud);

     // Grab the vCard
     get_info(c.server.username + "@" + c.get_server());

     // Basic Info
     // Only set the info if we already have it in their prefs and the vCard messed up somehow
     if (_entNickname->get_text().empty())
	  _entNickname ->set_text(fromUTF8(_entNickname, c.get_nick()));
     if (_entFirstName->get_text().empty())
	  _entFirstName->set_text(fromUTF8(_entFirstName, c.user.firstname));
     if (_entLastName->get_text().empty())
	  _entLastName ->set_text(fromUTF8(_entLastName, c.user.lastname));
     if (_entFullName->get_text().empty())
	  _entFullName ->set_text(fromUTF8(_entFullName, c.user.fullname));
     if (_entEMail->get_text().empty())
	  _entEMail    ->set_text(fromUTF8(_entEMail, c.user.email));
     Gtk::Entry* e = getEntry("MyContactInfo_Address_Country_txt");
     if (e->get_text().empty())
	  e            ->set_text(fromUTF8(e, c.user.country));
}

void MyContactInfoWin::saveconfig()
{
     // Save the config
     ConfigManager& c = G_App->getCfg();

     // JUD
     c.user.nojud = _chkNoJUD->get_active();

     // Set the JUD/vCard info
     set_info("users.jabber.org");

     // Basic Info
     c.set_nick(toUTF8(_entNickname, _entNickname->get_text()));
     c.user.firstname = toUTF8(_entFirstName, _entFirstName->get_text());
     c.user.lastname = toUTF8(_entLastName, _entLastName->get_text());
     c.user.fullname = toUTF8(_entFullName, _entFullName->get_text());
     c.user.email = toUTF8(_entEMail, _entEMail->get_text());
     Gtk::Entry* e = getEntry("MyContactInfo_Address_Country_txt");
     c.user.country = toUTF8(e, e->get_text());

     // Sync with the actual file and set to unmodified
     c.sync();
}

void MyContactInfoWin::changed()
{
     getButton("MyContactInfo_OK_btn")->set_sensitive(!_entNickname->get_text().empty());
}

void MyContactInfoWin::on_name_changed()
{
     // Make the full name equal First Name plus Last Name
     _entFullName->set_text(_entFirstName->get_text() + " " + _entLastName->get_text());
}

void MyContactInfoWin::on_ok_clicked()
{
     // Save changes
     saveconfig();
     // Get new key
     //manage_query("users.jabber.org");

     close();
}

void MyContactInfoWin::on_help_clicked()
{
     GnomeHelpMenuEntry help_entry = { "gabber", "userinfo.html" };
     gnome_help_display (NULL, &help_entry);
}

void MyContactInfoWin::on_cancel_clicked()
{
     close();
}

gint MyContactInfoWin::on_window_delete(GdkEventAny* e)
{
     on_cancel_clicked();
     return 0;
}


// ---------------------------------------------------------
//
// Agent Information Dialog
//
// ---------------------------------------------------------

AgentInfoDlg::AgentInfoDlg(const Agent& cur_agent)
     : BaseGabberDialog("TransInfo_dlg"), 
     _pix_path(ConfigManager::get_PIXPATH()),
     _share_dir(ConfigManager::get_SHAREDIR()),
     _agent((Agent*) &cur_agent)
{
     main_dialog(_thisDialog);

     // Grab the JID
     _jid = _agent->JID();

     // Setup buttons
     getButton("TransInfo_OK_btn")->clicked.connect(slot(this, &AgentInfoDlg::on_ok_clicked));
     getButton("TransInfo_Register_btn")->clicked.connect(slot(this, &AgentInfoDlg::on_register_clicked));
     getButton("TransInfo_Search_btn")->clicked.connect(slot(this, &AgentInfoDlg::on_search_clicked));
     getButton("TransInfo_Browse_btn")->clicked.connect(slot(this, &AgentInfoDlg::on_browse_clicked));
     getButton("TransInfo_Cancel_btn")->clicked.connect(slot(this, &AgentInfoDlg::on_cancel_clicked));

     // Fill in the info
     getEntry("TransInfo_JID_txt")->set_text(JID::getUserHost(_jid));
     _entNickname = getEntry("TransInfo_Nickname_txt");
     string nickname;
     try {
          nickname = G_App->getSession().roster()[_jid].getNickname();
	  _entNickname->set_text(fromUTF8(_entNickname, nickname));
     } catch (Roster::XCP_InvalidJID& e) {
	  _entNickname->hide();
	  getLabel("TransInfo_Nickname_lbl")->hide();
     }
     _pixS10n     = getWidget<Gnome::Pixmap>("TransInfo_s10nType_pix");

     Gtk::Label* l = getLabel("TransInfo_Service_lbl");
     l->set_text(fromUTF8(l, cur_agent.service()));
     l = getLabel("TransInfo_Description_lbl");
     l->set_text(fromUTF8(l, cur_agent.description()));
     if (_agent->isSearchable())
     {
	  getLabel("TransInfo_Search_lbl")->set_text(_("yes"));
	  getButton("TransInfo_Search_btn")->set_sensitive(true);
     }
     if (_agent->isRegisterable())
          getButton("TransInfo_Register_btn")->set_sensitive(true);
     if (_agent->hasAgents())
     {
	  getLabel("TransInfo_SubAgents_lbl")->set_text(_("yes"));
	  getButton("TransInfo_Browse_btn")->set_sensitive(true);
     }
     if (_agent->isGCCapable())
	  getLabel("TransInfo_GCCapable_lbl")->set_text(_("yes"));

     // Try to get a roster item
     try {
	  Roster::Item item(G_App->getSession().roster()[_jid]);
	  getLabel("TransInfo_s10nType_lbl")->set_text(getS10nName(item.getSubsType()));
	  getLabel("TransInfo_s10nE9n_lbl")->set_text(getS10nInfo(item.getSubsType()));
	  // Load the subscription image
	  string pix_s10n;
	  switch (item.getSubsType())
	  {
	  case Roster::rsBoth:
	  case Roster::rsTo:
	       pix_s10n = _share_dir + "glade-s10n.xpm";
	       break;
	  case Roster::rsFrom:
	  case Roster::rsNone:
	  default:
	       pix_s10n = _pix_path + "stalker.xpm";
	  }
	  _pixS10n->load(pix_s10n);

     } catch (Roster::XCP_InvalidJID& e)
     {
	  // It's not on the roster
	  getLabel("TransInfo_s10nType_lbl")->set_text(getS10nName(Roster::rsNone));
	  getLabel("TransInfo_s10nE9n_lbl")->set_text(getS10nInfo(Roster::rsNone));
	  _pixS10n->load(_pix_path + "stalker.xpm");
     }

     show();
}

AgentInfoDlg::AgentInfoDlg(const string& jid)
     : BaseGabberDialog("TransInfo_dlg"), _jid(jid),
       _pix_path(ConfigManager::get_PIXPATH()),
       _share_dir(ConfigManager::get_SHAREDIR()),
       _agent(0)
//       _agentsitem(G_App->getSession().getAgents().getItem(jid))
{
     main_dialog(_thisDialog);
// HELP: We need a way to get an Agent from a JID!

     // Setup buttons
     getButton("TransInfo_OK_btn")->clicked.connect(slot(this, &AgentInfoDlg::on_ok_clicked));
     getButton("TransInfo_Cancel_btn")->clicked.connect(slot(this, &AgentInfoDlg::on_cancel_clicked));

     // Fill in the info
     getEntry("TransInfo_JID_txt")->set_text(JID::getUserHost(_jid));
     _entNickname = getEntry("TransInfo_Nickname_txt");
     string nickname;
     try {
          nickname = G_App->getSession().roster()[_jid].getNickname();
	  _entNickname->set_text(fromUTF8(_entNickname, nickname));
     } catch (Roster::XCP_InvalidJID& e) {
	  _entNickname->hide();
	  getLabel("TransInfo_Nickname_lbl")->hide();
     }
     _pixS10n     = getWidget<Gnome::Pixmap>("TransInfo_s10nType_pix");

     // Try to get a roster item
     try {
	  Roster::Item item(G_App->getSession().roster()[_jid]);
	  getLabel("TransInfo_s10nType_lbl")->set_text(getS10nName(item.getSubsType()));
	  getLabel("TransInfo_s10nE9n_lbl")->set_text(getS10nInfo(item.getSubsType()));
	  // Load the subscription image
	  string pix_s10n;
	  switch (item.getSubsType())
	  {
	  case Roster::rsBoth:
	  case Roster::rsTo:
	       pix_s10n = _share_dir + "glade-s10n.xpm";
	       break;
	  case Roster::rsFrom:
	  case Roster::rsNone:
	  default:
	       pix_s10n = _pix_path + "stalker.xpm";
	  }
	  _pixS10n->load(pix_s10n);
     } catch (Roster::XCP_InvalidJID& e)
     {
	  // It's not on the roster
	  getLabel("TransInfo_s10nType_lbl")->set_text(getS10nName(Roster::rsNone));
	  getLabel("TransInfo_s10nE9n_lbl")->set_text(getS10nInfo(Roster::rsNone));
	  _pixS10n->load(_pix_path + "stalker.xpm");
     }
     show();
}

void AgentInfoDlg::on_ok_clicked()
{
     if (_entNickname->is_visible())
     {
	  try {
	       Roster::Item item(G_App->getSession().roster()[_jid]);
	       item.setNickname(toUTF8(_entNickname, _entNickname->get_text()));
	       G_App->getSession().roster() << item;
	  } catch (Roster::XCP_InvalidJID& e) {
	       cerr << "Nickname change unsuccessful" << endl;
	  }
     }
     close();
}

void AgentInfoDlg::on_register_clicked()
{
     if (_agent != NULL)
     {
          manage(new AgentRegisterDruid(*_agent));
     }
}

void AgentInfoDlg::on_search_clicked()
{
     if (_agent != NULL)
     {
	  AddContactDruid::display(*_agent);
     }
}

void AgentInfoDlg::on_browse_clicked()
{
     if (_agent != NULL)
     {
	  cerr << "start agentbrowser with " << _jid << endl;
     }
}

void AgentInfoDlg::on_cancel_clicked()
{
     close();
}
