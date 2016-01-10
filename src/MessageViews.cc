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

#include "MessageViews.hh"

#include "AddContactDruid.hh"
#include "ChatView.hh"
#include "ContactInfoInterface.hh"
#include "ContactInterface.hh"
#include "ErrorManager.hh"
#include "EventManager.hh"
#include "GabberApp.hh"
#include "GabberGPG.hh"
#include "GabberUtility.hh"
#include "GabberWidgets.hh"
#include "GabberWin.hh"
#include "GCInterface.hh"
#include "MessageManager.hh"

// GtkText extensions
#include "gtkspell.h"
#include "gtkurl.h"

#include <libgnome/gnome-help.h>
#include <libgnome/gnome-url.h>
#include <libgnomeui/gnome-window-icon.h>
#include <gtk--/arrow.h>
#include <gtk--/style.h>
#include <gtk--/table.h>
#include <gnome--/href.h>

using namespace jabberoo;
using namespace GabberUtil;

// -------------------------------------------------------------------
//
// Basic Message View
//
// -------------------------------------------------------------------
MessageView::MessageView(const jabberoo::Message& m, ViewMap& vm, bool only_userhost)
     : _jid(m.getFrom()), _only_userhost(only_userhost), _registrar(vm)
{
     // Register with the registrar
     if (_only_userhost)
     {
	  // only register the user@host portion of the JID
	  _registrar.insert(make_pair(JID::getUserHost(_jid), this));
     }
     else
     {
	  // register the entire JID
	  _registrar.insert(make_pair(_jid, this));
     }
}

MessageView::MessageView(const string& jid, ViewMap& vm, bool only_userhost)
     : _jid(jid), _only_userhost(only_userhost), _registrar(vm)
{
     // Register with the registrar
     if (_only_userhost)
     {
	  // only register the user@host portion of the JID
	  _registrar.insert(make_pair(JID::getUserHost(_jid), this));
     }
     else
     {
	  // register the entire JID
          _registrar.insert(make_pair(_jid, this));
     }
}

MessageView::~MessageView()
{
     // Remove the entry for this object
     if (_only_userhost)
     {
	  // remove only a user@host jid
	  _registrar.erase(JID::getUserHost(_jid));
     }
     else
     {
	  // remove the full JID
	  _registrar.erase(_jid);
     }
}

void MessageView::has_messages_waiting(bool msgswaiting)
{
}

void MessageView::raise() {
}


// -------------------------------------------------------------------
//
// Message Utilities
// Commonly used functions for the various message views
//
// -------------------------------------------------------------------

bool MessageUtil::gpg_toggle(Gtk::ToggleButton* tb, const string& jid, const string& resource)
{
     // If gpg is enabled, check for a signed presence for the jid
     if (G_App->getGPG().enabled() && !jid.empty())
     {
	  try {
	       PresenceDB::range r = G_App->getSession().presenceDB().equal_range(jid);
	       PresenceDB::const_iterator it = r.first;
	       // We only want online resources that have the same resource as the one we are sending to
               while (it != r.second && !resource.empty() && (it->getType() == Presence::ptUnavailable || JID::getResource(it->getFrom()) != resource))
	            it++;

	       Element* x = NULL;
	       if ((it == r.second) || !(x = it->findX("jabber:x:signed")))
	       {
		    // No presence or no signature on the presence so turn off encryption
		    tb->set_active(false);
		    tb->set_sensitive(false);
		    return false;
	       }
	       else
	       {
		    // Have a presence with a signature.  Should we verify the signature here or is it enough that
		    // it exists?
		    tb->set_sensitive(true);
		    tb->set_active(true);
		    return true;
	       }
          } catch (PresenceDB::XCP_InvalidJID& e) {
	       tb->set_active(false);
	       tb->set_sensitive(false);
	       return false;
          }
     }
     else
     {
	  tb->set_active(false);
	  tb->set_sensitive(false);
     }
     return false;
}

bool MessageUtil::gpg_toggle(Gtk::ToggleButton* tb, const Presence& p)
{
     if (G_App->getGPG().enabled())
     {
     	  Element* x = p.findX("jabber:x:signed");
	  if (x)
	  {
	       // Have a presence with a signature.  Should we verify the signature here or is it enough that
	       // it exists?
	       tb->set_sensitive(true);
	       tb->set_active(true);
	       return true;
	  }
	  else
	  {
	       tb->set_active(false);
	       tb->set_sensitive(false);
	       return false;
	  }
     }
     else
     {
	  tb->set_active(false);
	  tb->set_sensitive(false);
     }
     return false;
}


// -------------------------------------------------------------------
//
// Normal Message Send
// Used with the Normal Message Recv View
//
// -------------------------------------------------------------------
MessageSendDlg::MessageSendDlg(const string& jid)
     : BaseGabberWindow("MessageSend_win"),
       _msgRecvView(NULL),
       _jid(jid),
       _thread("")
{
     init();

     if (_jid.empty())
     {
//FIXME: implement grab_focus() in PrettyJID - focus goes to either _entJID or _cboResource->get_entry()
//	  _entSendTo->grab_focus();
	  // Set the title
	  _thisWindow->set_title(_("New Blank Message - Gabber"));
     }

     // Display
     show();
}

MessageSendDlg::MessageSendDlg(const jabberoo::Message& m, MessageRecvView* mrv)
     : BaseGabberWindow("MessageSend_win"),
       _msgRecvView(mrv),
       _jid(m.getFrom()),
       _thread(m.getThread())
{
     init();

     _entSubject->set_text(fromUTF8(_entSubject, m.getSubject()));

     // If this is a reply to a one-on-one chat (it's possible),
     if (m.getType() == Message::mtChat)
          // set send as ooochat
	  getCheckButton("MessageSend_OOOChat_chk")->set_active(true);

     // Display
     show();
}

MessageSendDlg::~MessageSendDlg()
{
}

void MessageSendDlg::init()
{
     _thisWindow->realize();

     // default is the username
     _nickname = JID::getUser(_jid);

     // Connect buttons to handler
     getButton("MessageSend_Send_btn")  ->clicked.connect(slot(this, &MessageSendDlg::on_Send_clicked));
     getButton("MessageSend_Cancel_btn")->clicked.connect(slot(this, &MessageSendDlg::on_Cancel_clicked));
     _btnAddContact = getButton("MessageSend_AddContact_btn");
     _btnAddContact ->clicked.connect(slot(this, &MessageSendDlg::on_AddContact_clicked));
     getButton("MessageSend_ContactInfo_btn")->clicked.connect(slot(this, &MessageSendDlg::on_ContactInfo_clicked));
     getButton("MessageSend_History_btn")    ->clicked.connect(slot(this, &MessageSendDlg::on_History_clicked));

//     getWidget<Gtk::EventBox>("MessageSend_Expand_evt")->button_press_event.connect(slot(this, &MessageSendDlg::on_Event_clicked));

     _thisWindow->key_press_event.connect(slot(this, &MessageSendDlg::on_window_key_press));
     _thisWindow->delete_event.connect(slot(this, &MessageSendDlg::on_window_delete));

     // Toolbar
     getWidget<Gtk::Toolbar>("MessageSend_toolbar")->set_style(GTK_TOOLBAR_ICONS);

     // Initialize pointers
     _txtBody  = getWidget<Gtk::Text>("MessageSend_Body_txt");
     _txtBody  ->set_word_wrap(true);
     if (gtkspell_running())
	  gtkspell_attach(_txtBody->gtkobj()); // Attach gtkspell for spell checking
     manage(new GabberDnDText(_txtBody)); // Attach some drag and drop logic
     _entSubject       = getEntry("MessageSend_Subject_ent");

     // Nickname and status display
     _pjid = manage(new PrettyJID(_jid, "", PrettyJID::dtNickRes, 128, true, _jid.empty()));
     _pjid ->changed.connect(slot(this, &MessageSendDlg::on_SendTo_changed));
     _pjid ->show();
     getWidget<Gtk::HBox>("MessageSend_JIDInfo_hbox")->pack_end(*_pjid, true, true, 0);
     _nickname = _pjid->get_nickname();
     _onRoster = _pjid->is_on_roster();

     // Pixmaps
     string pix_path = ConfigManager::get_PIXPATH();
     string window_icon = pix_path + "gnome-message.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();

     // _useGPG will be set when sendto is set or when a message is rendered
     _useGPG = false;
     Gtk::ToggleButton* tb = getWidget<Gtk::ToggleButton>("MessageSend_Encrypt_tglbtn");
     tb->set_sensitive(G_App->getGPG().enabled());
     tb->set_active(false);

     on_SendTo_changed(); // Call this to get the window title set

     _thisWindow->set_default_size(G_App->getCfg().msgs.width, G_App->getCfg().msgs.height);
     _thisWindow->show();
}

void MessageSendDlg::on_Send_clicked()
{
     // Compose message from text in the text box
     if (!_txtBody->get_chars(0, -1).empty() && !_jid.empty() && G_Win->is_connected())
     {
          // Ensure the message & jid are not editable while we're sending it...
          _txtBody->set_sensitive(false);
	  getHBox("MessageSend_JIDInfo_hbox")->set_sensitive(false);

          // If "Send as OOOChat" is picked, send the messages w/ a OOOChat flag
          Message::Type mtype;
          if (getCheckButton("MessageSend_OOOChat_chk")->get_active())
               mtype = Message::mtChat;
          else
               mtype = Message::mtNormal;

	  // message contents
	  string body = toUTF8(_txtBody, _txtBody->get_chars(0,-1));
          // Construct the message
	  Message m(_pjid->get_full_jid(), body, mtype);

	  MessageManager::Encryption enc = MessageManager::encNone;
	  // If the encryption toggle button is active encrypt the message
	  // send_message will check if gpg is enabled
	  if (getWidget<Gtk::ToggleButton>("MessageSend_Encrypt_tglbtn")->get_active())
	  {
	       // encrypt the message
	       enc = MessageManager::encEncrypt;
	  }
	  // If we aren't encrypting then sign the message
	  else if (_useGPG)
	  {
	       // sign the message
	       enc = MessageManager::encSign;
	  }

          // If they set a subject, add it to the message
          string subject = toUTF8(_entSubject, _entSubject->get_text());
          if (!subject.empty())
               m.setSubject(subject);

	  // Set the thread
	  if (!_thread.empty())
	       m.setThread(_thread);

          // Actually send the message
          bool message_sent = G_App->getMessageManager().send_message(m, enc);

          // If the message wasn't sent, allow them to edit it again.
          if (!message_sent)
	  {
               _txtBody->set_sensitive(true);
	       getHBox("MessageSend_JIDInfo_hbox")->set_sensitive(true);
	  }
          else
          {
               // And we're done... close
               close();
          }
     }
}

void MessageSendDlg::on_Cancel_clicked()
{
     close();
}

gint MessageSendDlg::on_window_delete(GdkEventAny* e)
{
     on_Cancel_clicked();
     return 0;
}

gint MessageSendDlg::on_window_key_press(GdkEventKey* e)
{
     // If they pressed the Keypad enter, make it act like a normal enter
     if (e->keyval == GDK_KP_Enter)
	  e->keyval = GDK_Return;

     // If they pressed Escape, act like a dialog and close
     if (e->keyval == GDK_Escape)
	  on_Cancel_clicked();
     // If they pressed space, run spellcheck
     else if (e->keyval == GDK_space && gtkspell_running())
	  gtkspell_check_all(_txtBody->gtkobj());
     // If Ctrl-Enter is pressed, send the message
     else if ( (e->keyval == GDK_Return) && (e->state & GDK_CONTROL_MASK) )
          on_Send_clicked();
     // If Shift-Enter is pressed, insert a newline
     else if ( (e->keyval == GDK_Return) && (e->state & GDK_SHIFT_MASK) )
	  e->state ^= GDK_SHIFT_MASK;
     // If Ctrl-w is pressed, close this window
     else if ( (e->keyval == GDK_w) && (e->state & GDK_CONTROL_MASK) )
	  on_Cancel_clicked();

     return 0;
}

void MessageSendDlg::on_SendTo_changed()
{
     // Update the JabberID and nickname
     _jid = _pjid->get_jid(); 
     _nickname = _pjid->get_nickname();

     // Update the add contact button
     if (_pjid->is_on_roster())
     {
	  _btnAddContact->set_sensitive(false);
     }
     else
     {
	  _btnAddContact->set_sensitive(true);
     }

     // Set the title
     if (_jid.empty())
     {
	  _thisWindow->set_title(_("New Blank Message - Gabber"));
     }
     else
     {
	  _thisWindow->set_title(substitute(_("Message to %s"), fromUTF8(_thisWindow, _nickname)) + _(" - Gabber"));
     }

     // Do GPG Checks
     _useGPG = MessageUtil::gpg_toggle(getWidget<Gtk::ToggleButton>("MessageSend_Encrypt_tglbtn"), _jid, JID::getResource(_pjid->get_full_jid()));
}

void MessageSendDlg::on_AddContact_clicked()
{
     // Start AddContactDruid with that JID
     if (!_jid.empty())
          AddContactDruid::display(JID::getUserHost(_jid));
}

void MessageSendDlg::on_ContactInfo_clicked()
{
     // Contact Info for that JID
     if (!_jid.empty())
     {
          if (_onRoster)
               ContactInfoDlg::display(_jid, _thisWindow);
          else
               ContactInfoDlg::display(_jid, Roster::rsNone, _thisWindow);
     }
}

void MessageSendDlg::on_History_clicked()
{
     if (!_jid.empty())
     {
	  string file = "file://";
	  file += string(G_App->getLogFile(_jid));
	  gnome_url_show(file.c_str());
     }
}

gint MessageSendDlg::on_Event_clicked(GdkEventButton* button)
{
/*    Gtk::Arrow* arrow = getWidget<Gtk::Arrow>("MessageSend_Expand_arw");
    Gtk::HBox*  hboxMulti = getHBox("MessageSend_SendToMulti_hbox");
    Gtk::HBox*  hboxSingle = getHBox("MessageSend_SendTo_hbox");

    if (hboxSingle->is_visible())
    {
	 hboxSingle->hide();
	 hboxMulti->show();
	 arrow->set(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
    }
    else
    {
	 hboxMulti->hide();
	 hboxSingle->show();
	 arrow->set(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
    }
*/
    return 1;
}

// -------------------------------------------------------------------
//
// Normal Message Recv
// Used with the Normal Message Send Dlg
//
// -------------------------------------------------------------------

// Init function to register view with MessageManager
void MessageRecvView::init_view(MessageManager& mm)
{
     mm.register_view_type("normal", (MessageManager::MessageViewInfo::ViewInfoFlags) 
			             (MessageManager::MessageViewInfo::vfPerResource |
				      MessageManager::MessageViewInfo::vfQueueMessage),
			   new_view_msg, new_view_jid, "glade-message.xpm", "#669999");
}

// Init function to register view with MessageManager
void MessageRecvView::init_oob_view(MessageManager& mm)
{
     mm.register_view_type("jabber:x:oob", (MessageManager::MessageViewInfo::ViewInfoFlags) 
			                   (MessageManager::MessageViewInfo::vfPerResource | 
					    MessageManager::MessageViewInfo::vfExtension |
					    MessageManager::MessageViewInfo::vfQueueMessage),
                           new_view_msg, new_view_jid, "glade-message.xpm", "#669999");
}

MessageView* MessageRecvView::new_view_msg(const Message& m, ViewMap& vm)
{
     MessageView* mv = manage(new MessageRecvView(m, vm));
     return mv;
}

MessageView* MessageRecvView::new_view_jid(const string& jid, ViewMap& vm)
{
     MessageView* mv = NULL;
     manage(new MessageSendDlg(jid));
     return mv;
}

MessageRecvView::MessageRecvView(const jabberoo::Message& m, ViewMap& vm)
     : MessageView(m, vm), BaseGabberWindow("MessageRecv_win"),
       _message(m),
       _Messages_current(_Messages.begin()),
       _msgswaiting(false),
       _Messages_should_queue(true)
{
     init();

     // Parse out the message
     render(m);

     // Display
     show();
}

MessageRecvView::~MessageRecvView()
{
     // delete the composing event
     if (_composing_msg != NULL)
     {
	  delete _composing_msg;
	  _composing_msg = NULL;
     }
     
     // Clear out the queue
     _Messages.clear();
}

void MessageRecvView::init()
{
     _thisWindow->realize();

     // Connect buttons to handler
     getButton("MessageRecv_Reply_btn")->clicked.connect(slot(this, &MessageRecvView::on_Reply_clicked));
     getButton("MessageRecv_Close_btn")->clicked.connect(slot(this, &MessageRecvView::on_Close_clicked));
     _btnAddContact = getButton("MessageRecv_AddContact_btn");
     _btnAddContact ->clicked.connect(slot(this, &MessageRecvView::on_AddContact_clicked));
     getButton("MessageRecv_ContactInfo_btn")->clicked.connect(slot(this, &MessageRecvView::on_ContactInfo_clicked));
     getButton("MessageRecv_History_btn")->clicked.connect(slot(this, &MessageRecvView::on_History_clicked));

     _thisWindow->key_press_event.connect(slot(this, &MessageRecvView::on_window_key_press));
     _thisWindow->delete_event.connect(slot(this, &MessageRecvView::on_window_delete));

     // Toolbar
     getWidget<Gtk::Toolbar>("MessageRecv_toolbar")->set_style(GTK_TOOLBAR_ICONS);

     // Initialize pointer to "prev" button (should be insensitive by default)
     _btnReadPrev = getButton("MessageRecv_ReadPrev_btn");
     _btnReadPrev->clicked.connect(slot(this, &MessageRecvView::on_ReadPrev_clicked));
     _btnReadPrev->set_sensitive(false);
     // Initialize pointer to "next" button (should be insensitive by default)
     _btnReadNext = getButton("MessageRecv_ReadNext_btn");
     _btnReadNext->clicked.connect(slot(this, &MessageRecvView::on_ReadNext_clicked));
     _btnReadNext->set_sensitive(false);

     // Initialize pointers
     _txtBody          = getWidget<Gtk::Text>("MessageRecv_Body_txt");
     _txtBody          ->set_word_wrap(true);
     gtkurl_attach(_txtBody->gtkobj()); // Attach gtkurl to this - thanks bratislav!
     _pixGPG           = getWidget<Gnome::Pixmap>("MessageRecv_GPG_pix");
     _evtGPG           = getWidget<Gtk::EventBox>("MessageRecv_GPG_evt");
     _gpgInfo          = GabberGPG::gpgNone;
     _composing_msg    = NULL;

     // Nickname and status display
     PrettyJID* pj = manage(new PrettyJID(_jid, "", PrettyJID::dtNickRes, 196));
     pj->show();
     getWidget<Gtk::HBox>("MessageRecv_JIDInfo_hbox")->pack_start(*pj, true, true, 0);
     _onRoster = pj->is_on_roster();
     _nickname = pj->get_nickname();
     
     // Update the add contact button
     if (pj->is_on_roster())
     {
	  _btnAddContact->set_sensitive(false);
     }
     else
     {
	  _btnAddContact->set_sensitive(true);
     }
     
     // Set the window title
     _thisWindow->set_title(substitute(_("Message from %s"), fromUTF8(_thisWindow, pj->get_nickname())) + _(" - Gabber"));

     // Pixmaps
     string pix_path = ConfigManager::get_PIXPATH();
     string window_icon = pix_path + "gnome-message.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();

     // Add button_press handler for GPGInfo Dialog
     _evtGPG->button_press_event.connect(slot(this, &MessageRecvView::on_GPGInfo_button_press));

     _thisWindow->set_default_size(G_App->getCfg().msgs.width, G_App->getCfg().msgs.height);
     _thisWindow->show();
}

void MessageRecvView::render(const jabberoo::Message& m)
{
     // if we should be adding it to the queue
     if (_Messages_should_queue)
     {
	  // Add the message to the queue
	  _Messages.push_back(m);
	  _Messages_current = --_Messages.end();
     }

     // This is the last message in the queue
     if (_Messages_current == --_Messages.end())
     {	  
	  // Set Read Next sensitivity -- in case we just came to the font
	  _btnReadNext->set_sensitive(_msgswaiting);
     }
     // we're somewhere within the queue
     else
	  _btnReadNext->set_sensitive(true);

     _Messages_should_queue = true;

     // If this is the first message we put in the queue 
     // then we have no previous messages
     if (_Messages_current == _Messages.begin())
	  _btnReadPrev->set_sensitive(false);
     else
	  _btnReadPrev->set_sensitive(true);

     // Clear out the body text
     _txtBody->delete_text(0, -1);
     // Remove any extras added to the header table
     // such as URLs
     getWidget<Gtk::Table>("MessageRecv_Header_tbl")->resize(3,2);

     if (!m.getSubject().empty())
     {
	  getLabel("MessageRecv_SubjectLabel_lbl")->show();
	  Gtk::Label* l = getLabel("MessageRecv_Subject_lbl");
	  l->set_text(fromUTF8(l, m.getSubject()));
	  l->show();
     }
     else
     {
	  getLabel("MessageRecv_SubjectLabel_lbl")->hide();
	  getLabel("MessageRecv_Subject_lbl")->hide();
     }
     getLabel("MessageRecv_Time_lbl")->set_text(m.getDateTime());

     int i = 0;
     if (m.getBody().length() > 0)
     {
	  _txtBody->insert_text(fromUTF8(_txtBody, m.getBody()).c_str(),
				fromUTF8(_txtBody, m.getBody()).length(), &i);
	  gtkurl_check_all(_txtBody->gtkobj()); // Highlight the URLs

	  // Hrmm, this doesn't seem to scroll it back to the beginning
	  _txtBody->set_point(0);
     }
     else
     {
	  // Probably just using MessageRecvView for extensions
	  _txtBody->hide();
     }

     // change to Encryption icon to unlock until we know whether this message has any encryption
     _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-unencrypted.xpm"));
     // Set the tooltip
     _tips.set_tip(*_evtGPG, _("This message is not encrypted"));

     Element* x = m.findX("jabber:x:signed");
     GabberGPG& gpg = G_App->getGPG();
     if (x != NULL && gpg.enabled())
     {
	  string message = m.getBody();
	  string encrypted = x->getCDATA();
	  GPGInterface::SigInfo info;
	  GPGInterface::Error err;

	  if ((err = gpg.verify(info, encrypted, message)) != GPGInterface::errOK)
	  {
	       if (err == GPGInterface::errPubKey)
	       {
		    cerr << "FIXME: need a way to get users Public Key" << endl;
	       }
	       // Set icon to invalid signature
	       _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-badsigned.xpm"));
	       // Set the tooltip
	       _tips.set_tip(*_evtGPG, _("Signature on message is invalid"));
	       _gpgInfo = GabberGPG::gpgInvalidSigned;
	  }
	  else
	  {
	       // It was a valid signature so add the KeyID to the KeyMap
	       gpg.add_jid_keyid(m.getFrom(), info.get_key().get_keyid());
	       // Set icon to valid signature icon
	       _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-signed.xpm"));
	       // Set the tooltip
	       _tips.set_tip(*_evtGPG, _("Signature on message is valid"));
	       _gpgInfo = GabberGPG::gpgValidSigned;
	  }
     }
     x = m.findX("jabber:x:encrypted");
     if (x != NULL && gpg.enabled()) 
     {
	  string message;
	  string encrypted = x->getCDATA();
	  GPGInterface::DecryptInfo info;
	  GPGInterface::Error err;

	  if ((err = gpg.decrypt(info, encrypted, message)) != GPGInterface::errOK)
	  {
	       if (err == GPGInterface::errPubKey)
	       {
		    cerr << "FIXME: need a way to get the user's Public Key" << endl;
	       }
	       // If the decryption failed, there's not much we can do with the message
	       return;
	  }
	  // Need to figure out if it's a WinJab encrypted message and make a second pass over the
	  // decrypted text to verify the signature
	  bool winjab_sig = false;
	  if (strncmp(message.c_str(), "-----BEGIN PGP", 14) == 0)
	  {
	       // It is a winjab message so we need to run it through gpg again
	       if ((err = gpg.verify_clear((GPGInterface::SigInfo &) info.get_sig(), message, message)) != GPGInterface::errOK)
	       {
		    cerr << "FIXME: don't have user's public key" << endl;
		    return; 
	       }
	       winjab_sig = true;
	  }
	  // Set the message text to what was just decrypted
	  _txtBody->delete_text(0, -1);
	  i = 0;
	  _txtBody->insert_text(fromUTF8(_txtBody, message).c_str(), 
				fromUTF8(_txtBody, message).length(), &i);
	  _txtBody->set_point(0);
	  gtkurl_check_all(_txtBody->gtkobj());

	  // If the encryption had a signature (which it should) add the keyid to the KeyMap
	  if ((info.has_sig() || winjab_sig) && info.get_sig().valid())
	       gpg.add_jid_keyid(m.getFrom(), info.get_sig().get_key().get_keyid());
	  _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-encrypted.xpm"));
	  // Set the tooltip
	  _tips.set_tip(*_evtGPG, _("This message is encrypted"));
	  _gpgInfo = GabberGPG::gpgValidEncrypted;
     }

     // Walk the list of children to find all extensions which can occur multiple times
     Gtk::Table* tblHeader = getWidget<Gtk::Table>("MessageRecv_Header_tbl");
     int row = 3;
     Element::const_iterator it = m.getBaseElement().begin();
     for (; it != m.getBaseElement().end(); it++)
     {
	  // Cast the child element into a tag
	  if ((*it)->getType() != Node::ntElement)
	       continue;
	  Element& x = *static_cast<Element*>(*it);

	  string xmlns = x.getAttrib("xmlns");
	  if (xmlns.empty())
	       continue;
	  if (xmlns == "jabber:x:oob")
	  {
	       // Create label
	       Gtk::Label* lbl = manage(new Gtk::Label(_("Attached URL:"), 1.0, 0.0));
	       lbl->set_justify(GTK_JUSTIFY_RIGHT);
	       tblHeader->attach(*lbl, 0, 1, row, row+1, GTK_FILL, GTK_FILL);
	       lbl->show();

	       // Create vbox for href
	       Gtk::VBox* vbox = manage(new Gtk::VBox(false, 4));

	       // Create hbox for href
	       Gtk::HBox* hbox = manage(new Gtk::HBox(false, 0));

	       // Empty label taking up extra space trick
	       lbl = manage(new Gtk::Label());
	       hbox->pack_end(*lbl, true, true);
	       lbl->show();
		    
	       // The href
	       string desc = x.getChildCData("desc");
	       string url = x.getChildCData("url");
	       // Ensure the desc is not too long
	       if (desc.length() > 128)
	       {
		    // If the desc is too long, we make it another label
		    lbl = manage(new Gtk::Label());
		    lbl->set_text(fromUTF8(lbl, desc));
		    lbl->set_justify(GTK_JUSTIFY_LEFT);
		    lbl->set_alignment(0,0);
		    lbl->set_line_wrap(true);
		    vbox->pack_end(*lbl, true, true);
		    lbl->show();
	       }
	       if (desc.empty() || desc.length() > 128)
	       {
		    // The description on the HRef will just be the url
		    desc = url;
		    if (desc.length() > 128)
			 desc = substitute(_("%s..."), desc.substr(0, 128));
	       }

	       Gnome::HRef* href = manage(new Gnome::HRef(url));
	       href->set_label(fromUTF8(href, desc));
	       hbox->pack_end(*href, false, true);
	       href->show();

	       vbox->pack_start(*hbox);
	       hbox->show();

	       tblHeader->attach(*vbox, 1, 2, row, row+1, GTK_EXPAND|GTK_FILL, 0);
	       vbox->show();

	       row++;
	  }
     }

     // current message
     if (x != NULL && _Messages_current == --_Messages.end())
     {
	  // message displayed event
	  G_App->getEventManager().message_displayed(m, _nickname, MessageManager::translateType(m));

	  // If there appear to be events in this message
	  x = m.findX("jabber:x:event");
	  if (x != NULL && x->findElement("composing") != NULL)
	  {
	       if (_composing_msg != NULL)
		    delete _composing_msg;
	       // Get the composing event message
	       _composing_msg = new Message(m.composing().getBaseElement());
	  }
     }

     // Popup the window if they like it that way
     if (G_App->getCfg().msgs.raise)
	  gdk_window_show(_thisWindow->get_window().gdkobj());
}

void MessageRecvView::has_messages_waiting(bool msgswaiting)
{
     _msgswaiting = msgswaiting;
     if (_Messages_current == --_Messages.end())
     {
	  // Enable the next button, if necessary
	  _btnReadNext->set_sensitive(msgswaiting);
     }
}

void MessageRecvView::on_Reply_clicked()
{
     // Send the composing event and delete it
     if (_composing_msg != NULL)
     {
	  G_App->getSession() << *_composing_msg;
	  delete _composing_msg;
	  _composing_msg = NULL;
     }

     // Open
     manage(new MessageSendDlg(_message, this));
}

void MessageRecvView::on_ReadPrev_clicked()
{
     _Messages_should_queue = false;
     render(*(--_Messages_current));
}

void MessageRecvView::on_ReadNext_clicked()
{
     if (_Messages_current == --_Messages.end())
     {
	  // We want to go grab a new message
	  MessageManager::EventList::iterator it = G_App->getMessageManager().getEvent(_jid);
	  if (it != G_App->getMessageManager().getEvents().end())
	  {
	       // display using it->first instead of defaultJID because defaultJID may not
	       // contain a resource or may not be the resource the event came from
	       G_App->getMessageManager().display(it->first, it->second);
	  }
     }
     else
     {
	  _Messages_should_queue = false;
	  // We're somewhere within the queue
	  render(*(++_Messages_current));
     }
}

void MessageRecvView::on_Close_clicked()
{
     close();
}


gint MessageRecvView::on_window_delete(GdkEventAny* e)
{
     on_Close_clicked();
     return 0;
}

gint MessageRecvView::on_window_key_press(GdkEventKey* e)
{
     // If they pressed the Keypad enter, make it act like a normal enter
     if (e->keyval == GDK_KP_Enter)
	  e->keyval = GDK_Return;

     // If they pressed Escape, act like a dialog and close
     if (e->keyval == GDK_Escape)
	  on_Close_clicked();
     // If Ctrl-Enter is pressed, reply to the message
     else if ( (e->keyval == GDK_Return) && (e->state & GDK_CONTROL_MASK) )
          on_Reply_clicked();
     // If Ctrl-w is pressed, close this window
     else if ( (e->keyval == GDK_w) && (e->state & GDK_CONTROL_MASK) )
	  on_Close_clicked();

     return 0;
}

void MessageRecvView::on_AddContact_clicked()
{
     if (!_jid.empty())
          AddContactDruid::display(JID::getUserHost(_jid));
}

void MessageRecvView::on_ContactInfo_clicked()
{
     if (!_jid.empty())
     {
          if (_onRoster)
               ContactInfoDlg::display(_jid, _thisWindow);
          else
               ContactInfoDlg::display(_jid, Roster::rsNone, _thisWindow);
     }
}

void MessageRecvView::on_History_clicked()
{
     if (!_jid.empty())
     {
	  string file = "file://";
	  file += string(G_App->getLogFile(_jid));
	  gnome_url_show(file.c_str());
     }
}

int MessageRecvView::on_GPGInfo_button_press(GdkEventButton* e)
{
     bool valid = false;

     if (_gpgInfo == GabberGPG::gpgNone)
	  return 1;
     else if (_gpgInfo == GabberGPG::gpgValidSigned || _gpgInfo == GabberGPG::gpgValidEncrypted)
	  valid = true;

     try {
          string keyid = G_App->getGPG().find_jid_key(_jid); 
	  GPGInfoDialog* dlg = manage(new GPGInfoDialog(keyid, valid));
	  dlg->getBaseDialog()->set_parent(*_thisWindow);
     } catch (GabberGPG::GPG_InvalidJID& e) {
	  GPGInfoDialog* dlg = manage(new GPGInfoDialog("", valid));
	  dlg->getBaseDialog()->set_parent(*_thisWindow);
     }
     return 0;
}


// -------------------------------------------------------------------
//
// Chat (One-on-One) Message View
//
// -------------------------------------------------------------------

// Init function to register view with MessageManager
void ChatMessageView::init_view(MessageManager& mm)
{
     mm.register_view_type("chat", (MessageManager::MessageViewInfo::ViewInfoFlags)
				   (MessageManager::MessageViewInfo::vfPerResource |
                                    MessageManager::MessageViewInfo::vfMultiMessage),
                           new_view_msg, new_view_jid, "glade-ooochat.xpm", "#996699");
}
 
MessageView* ChatMessageView::new_view_msg(const Message& m, ViewMap& vm)
{
     MessageView* mv = manage(new ChatMessageView(m, vm));
     return mv;
}
 
MessageView* ChatMessageView::new_view_jid(const string& jid, ViewMap& vm)
{
     MessageView* mv = manage(new ChatMessageView(jid, vm));
     return mv;
}

ChatMessageView::ChatMessageView(const jabberoo::Message& m, ViewMap& vm)
     : MessageView(m, vm), BaseGabberWindow("OOOChat_win"),
       _pix_path(ConfigManager::get_PIXPATH())
{
     // Basic initialization
     init();

     // If this is a reply to a normal message (it's possible),
     if (m.getType() == Message::mtNormal)
          // set send as ooochat
          _chkMessage->set_active(true);

     // Grab the subject
     _subject = m.getSubject();

     // Grab the thread
     _thread = m.getThread();

     // Add the message
     render(m);

     // Display
     show();

     // Raise
     _hboxChatview->get_window().show();
     _hboxChatview->get_window().raise();
}

ChatMessageView::ChatMessageView(const string& jid, ViewMap& vm)
     : MessageView(jid, vm), BaseGabberWindow("OOOChat_win"),
       _pix_path(ConfigManager::get_PIXPATH())
{
     // Basic initialization
     init();

     // Display
     show();
}

ChatMessageView::~ChatMessageView()
{
     delete _chatview;
}

void ChatMessageView::init()
{
     _thisWindow->realize();

     // Get widgets
     _chkMessage  = getCheckButton("OOOChat_Message_chk");
     _btnAddUser  = getButton("OOOChat_AddUser_btn");
     _hboxChatview = getWidget<Gtk::HBox>("OOOChat_Chatview");
     _memMsg      = getWidget<Gtk::Text>("OOOChat_Message_txt");
     if (gtkspell_running())
	  gtkspell_attach(_memMsg->gtkobj()); // Spell check
     manage(new GabberDnDText(_memMsg)); // Attach some drag and drop logic
     _pixGPG      = getWidget<Gnome::Pixmap>("OOOChat_GPG_pix");
     _evtGPG      = getWidget<Gtk::EventBox>("OOOChat_GPG_evt");
     _baseCloseChat = manage(new BaseGabberWidget("OOOChat_CloseChat_dlg", "OOOChat_win"));
     _composing_msg = NULL;

     // Nickname and status display
     PrettyJID* pj = manage(new PrettyJID(_jid, "", PrettyJID::dtNickRes, 128));
     pj->show();
     getWidget<Gtk::HBox>("OOOChat_JIDInfo_hbox")->pack_start(*pj, true, true, 0);
     _nickname = pj->get_nickname();
     _onRoster = pj->is_on_roster();


     _baseCloseChat->getLabel("OOOChat_CloseChat_lbl")->set_text(substitute(_("You just received a message from %s."), fromUTF8(_thisWindow, pj->get_nickname()))); 

     // Create chat view
     _chatview = new ChatView(_thisWindow, _hboxChatview, false);

     // Connect events
     _btnAddUser->clicked.connect(slot(this, &ChatMessageView::on_AddUser_clicked));
     _thisWindow->key_press_event.connect(slot(this, &ChatMessageView::on_window_key_press));
     _thisWindow->delete_event.connect(slot(this, &ChatMessageView::on_window_delete));

     G_App->getSession().evtPresence.connect(slot(this, &ChatMessageView::on_presence));
     G_App->getErrorManager().errorChat.connect(slot(this, &ChatMessageView::on_error));

//   Current implementation lacks such buttons
//     getButton("OOOChat_Send_btn")->clicked.connect(slot(this, &ChatMessageView::OnSend));
//     getButton("OOOChat_Close_btn")->clicked.connect(slot(this, &ChatMessageView::on_Close_clicked));
     getButton("OOOChat_EditUser_btn")->clicked.connect(slot(this, &ChatMessageView::on_EditUser_clicked));
     getButton("OOOChat_History_btn")->clicked.connect(slot(this, &ChatMessageView::on_History_clicked));
     _baseCloseChat->getButton("OOOChat_CloseChat_Keep_btn")->clicked.connect(slot(this, &ChatMessageView::on_CloseChat_Keep_clicked));
     _baseCloseChat->getButton("OOOChat_CloseChat_Close_btn")->clicked.connect(slot(this, &ChatMessageView::on_CloseChat_Close_clicked));


     // Tweak widgets
     _dlgCloseChat = static_cast<Gnome::Dialog*>(_baseCloseChat->get_this_widget());
     _dlgCloseChat->set_close(false);
     _dlgCloseChat->close_hides(true);
     _dlgCloseChat->set_parent(*_thisWindow);

     if (_onRoster)
	  _btnAddUser->set_sensitive(false);
     else
	  _btnAddUser->set_sensitive(!G_App->isGroupChatID(_jid));
     _memMsg->set_word_wrap(true);

     getWidget<Gtk::Toolbar>("OOOChat_toolbar")->set_style(GTK_TOOLBAR_ICONS);

     // Resource
     _resource = JID::getResource(_jid);

     // Set the window title
     _thisWindow->set_title(substitute(_("Chat with %s"), fromUTF8(_thisWindow, _nickname)) + _(" - Gabber"));

     string window_icon = string(ConfigManager::get_PIXPATH()) + "gnome-ooochat.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(), window_icon.c_str());
     gnome_window_icon_init();

     // Get user status
     get_status();

     // Add button press handler for GPG_evt
     _evtGPG->button_press_event.connect(slot(this, &ChatMessageView::on_GPGInfo_button_press));
     _gpgInfo = GabberGPG::gpgNone;

     // Setup DnD targets that we can receive
     GtkTargetEntry dnd_dest_targets[] = {
//	  {"text/x-jabber-roster-item", 0, 0},
	  {"text/x-jabber-id", 0, 0},
     };
     int dest_num = sizeof(dnd_dest_targets) / sizeof(GtkTargetEntry);
     gtk_drag_dest_unset(GTK_WIDGET(_hboxChatview->gtkobj()));
     gtk_drag_dest_set(GTK_WIDGET(_hboxChatview->gtkobj()), (GtkDestDefaults) (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP), dnd_dest_targets, dest_num, (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE));

     // Setup DnD callbacks
     _hboxChatview->drag_data_received.connect(slot(this, &ChatMessageView::on_Chatview_drag_data_received));

     _thisWindow->set_default_size(G_App->getCfg().chats.ooowidth, G_App->getCfg().chats.oooheight);
     _thisWindow->show();
}

void ChatMessageView::raise() {
     // If this window is visible
     if (_thisWindow->is_visible()) {
	  // Raise the window
	  gdk_window_show(_thisWindow->get_window().gdkobj());
	  _thisWindow->activate_focus();
     }
}

void ChatMessageView::get_status()
{
     try {
	  // Get the presence
	  const Presence& p = *G_App->getSession().presenceDB().find(_jid);
	  on_presence(p, p.getType());
     } catch (PresenceDB::XCP_InvalidJID& e) {
	  // No presences from any resources
     }
}

void ChatMessageView::render(const jabberoo::Message& m)
{
     g_assert(_thisWindow != NULL);
     g_assert(_chatview != NULL);
     g_assert(_baseCloseChat != NULL);
     g_assert(_dlgCloseChat != NULL);

     // Start the new message timer
     if (_new_message_timer.connected())
	  _new_message_timer.disconnect();
     if (!_dlgCloseChat->is_visible())
	  _new_message_timer = Gtk::Main::timeout.connect(slot(this, &ChatMessageView::on_new_message_timer), 1500);

     // change the GPG pixmap to unlock until we have determine if the message is encrypted
     _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-unencrypted.xpm"));
     // Set the tooltip
     _tips.set_tip(*_evtGPG, _("This message is not encrypted"));

     // Grab the subject and thread, since they may have changed
     _subject = m.getSubject();
     _thread = m.getThread();
     string message = m.getBody();

     // reset useGPG so signed messages are only sent if the received message was signed or encrypted
     _useGPG = false;
     // Check for a signature
     Element* x = m.findX("jabber:x:signed");
     GabberGPG& gpg = G_App->getGPG();
     if (x != NULL && gpg.enabled())
     {
	  string encrypted = x->getCDATA();
	  GPGInterface::SigInfo info;
	  GPGInterface::Error err;

          if ((err = gpg.verify(info, encrypted, message)) == GPGInterface::errOK)
	  {
	       // The signature was valid so add the keyid to the KeyMap
	       gpg.add_jid_key(m.getFrom(), info.get_key().get_keyid());
	       // Change the pixmap
	       _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-signed.xpm"));
	       // Set the tooltip
	       _tips.set_tip(*_evtGPG, _("Signature on message is valid"));
	       _gpgInfo = GabberGPG::gpgValidSigned;
	       _useGPG = true;
	  }
	  else
	  {
	       // Set the GPG pixmap to bad signature
	       _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-badsigned.xpm"));
	       // Set the tooltip
	       _tips.set_tip(*_evtGPG, _("Signature on message is invalid"));
	       _gpgInfo = GabberGPG::gpgInvalidSigned;
	  }
     }

     // Check for encryption
     x = m.findX("jabber:x:encrypted");
     if (x != NULL && gpg.enabled() && !x->getCDATA().empty()) 
     {
          string encrypted = x->getCDATA();
          GPGInterface::DecryptInfo info;
          GPGInterface::Error err;

          if ((err = gpg.decrypt(info, encrypted, message)) != GPGInterface::errOK)
          {
               if (err == GPGInterface::errPubKey)
               {
                    cerr << "FIXME: need a way to get the user's Public Key" << endl;
               }
               return;
          }
	  else
	  {
	       // Need to figure out if it's a WinJab encrypted message and make a second pass over the
	       // decrypted text to verify the signature
	       bool winjab_sig = false;
	       if (strncmp(message.c_str(), "-----BEGIN PGP", 14) == 0)
	       {
		    // It is a winjab message so we need to run it through gpg again
		    if ((err = gpg.verify_clear((GPGInterface::SigInfo &) info.get_sig(), message, message)) != GPGInterface::errOK)
		    {
			 cerr << "FIXME: don't have user's public key" << endl;
			 return;
		    }
		    winjab_sig = true;
	       }
	       // If the message had a valid signature, add the keyid to the KeyMap
	       if ((info.has_sig() || winjab_sig) && info.get_sig().valid())
	            gpg.add_jid_key(m.getFrom(), info.get_sig().get_key().get_keyid());
	       _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-encrypted.xpm"));
	       // Set the tooltip
	       _tips.set_tip(*_evtGPG, _("This message is encrypted"));
	       _gpgInfo = GabberGPG::gpgValidEncrypted;
	       // The message was encrypted so enable encryption for the reply
// FIXME: The message was encrypted, so we should have the key or something 
//        but in some circumstances, the presence isn't signed. Blah!
//	       getWidget<Gtk::ToggleButton>("OOOChat_Encrypt_tglbtn")->set_sensitive(true);
//	       getWidget<Gtk::ToggleButton>("OOOChat_Encrypt_tglbtn")->set_active(true);
//	       _useGPG = true;
	  }
     }

     // If they want timestamps, put them in
     if (G_App->getCfg().chats.oootime)
          _chatview->render(fromUTF8(GTK_WIDGET(_chatview->_xtext), message), 
			    fromUTF8(GTK_WIDGET(_chatview->_xtext), _nickname),
                            m.getDateTime(G_App->getCfg().dates.chatformat), RED);
     else  
	  _chatview->render(fromUTF8(GTK_WIDGET(_chatview->_xtext), message), 
			    fromUTF8(GTK_WIDGET(_chatview->_xtext), _nickname),
                            "", RED);

     // message displayed event
     G_App->getEventManager().message_displayed(m, _nickname, MessageManager::translateType(m));
     
     // If there appear to be events in this message
     x = m.findX("jabber:x:event");
     
     if (x != NULL && x->findElement("composing") != NULL)
     {
	  if (_composing_msg != NULL)
	       delete _composing_msg;
	  // Get the composing event message
	  _composing_msg = new Message(m.composing().getBaseElement());
	  
	  if (_composing_event.connected())
	       _composing_event.disconnect();
	  // hook up a connection to send the composing event
	  _composing_event = _memMsg->changed.connect(slot(this, &ChatMessageView::on_memMsg_changed));
     }
	       
     // Popup the window if they like it that way
     if (G_App->getCfg().msgs.raise)
	  gdk_window_show(_thisWindow->get_window().gdkobj());
}

void ChatMessageView::on_Send_clicked()
{
     string body = toUTF8(_memMsg, _memMsg->get_chars(0, -1));
     if(!body.empty() && G_Win->is_connected())
     {
          // If "Send as Normal Message" is checked, send message with normal flag
          Message::Type mtype;
          if (_chkMessage->get_active())
               mtype = Message::mtNormal;
          else
               mtype = Message::mtChat;

          // Construct message
          Message m(_jid, body, mtype);
// TODO: UI for composing
//	  m.requestComposing();

	  // Set the subject
	  if (!_subject.empty())
	       m.setSubject(_subject);

	  // Set the thread
	  if (!_thread.empty())
	       m.setThread(_thread);

	  // Do Encryption
	  MessageManager::Encryption enc = MessageManager::encNone;
          if (getWidget<Gtk::ToggleButton>("OOOChat_Encrypt_tglbtn")->get_active())
          {
	       // Encrypt the message
	       enc = MessageManager::encEncrypt;
          }
	  // Default to signing if GPG is enabled but we aren't encrypting
          else if (_useGPG)
          {
	       // sign the message
	       enc = MessageManager::encSign;
          }

          // Transmit the message and render it in the dialog, only if the message was sent
          bool message_sent = G_App->getMessageManager().send_message(m, enc);

	  // only render the message if it was actually sent
	  if (message_sent)
	  {
               // Render the message
               // If they want timestamps, put them in
               if (G_App->getCfg().chats.oootime)
                    _chatview->render(fromUTF8(GTK_WIDGET(_chatview->_xtext), body), 
				      fromUTF8(GTK_WIDGET(_chatview->_xtext), G_App->getCfg().get_nick()),
                                      m.getDateTime(G_App->getCfg().dates.chatformat), BLUE2);
               else
                    _chatview->render(fromUTF8(GTK_WIDGET(_chatview->_xtext), body), 
				      fromUTF8(GTK_WIDGET(_chatview->_xtext), G_App->getCfg().get_nick()),
                                      "", BLUE2);
               // Reset the message box
               _memMsg->delete_text(0, -1);

	       // Update encryption pixmap
	       if (enc == MessageManager::encNone)
	       {
		    _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-unencrypted.xpm"));
		    // set the tooltip
		    _tips.set_tip(*_evtGPG, _("This message is not encrypted"));
	       }
	       else if (enc == MessageManager::encSign)
	       {
		    _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-signed.xpm"));
		    // set the tooltip
		    _tips.set_tip(*_evtGPG, _("Signature on message is valid"));
	       }
	       else if (enc == MessageManager::encEncrypt)
	       {
		    _pixGPG->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-encrypted.xpm"));
		    // set the tooltip
		    _tips.set_tip(*_evtGPG, _("This message is encrypted"));
	       }
          }

	  if (_composing_msg != NULL && !_composing_event.connected())
	  {
	       // hook up a connection to send the composing event
	       _composing_event = _memMsg->changed.connect(slot(this, &ChatMessageView::on_memMsg_changed));
	  }
     }
}

void ChatMessageView::on_Close_clicked()
{
     // If they just got a message
     if (_new_message_timer.connected())
     {
	  _new_message_timer.disconnect();
	  // Display the question of doom: are they sure they want to close?
	  _dlgCloseChat->show();
     }
     else
     {
	  close();
     }
}

void ChatMessageView::on_CloseChat_Close_clicked()
{
     g_assert(_dlgCloseChat != NULL);
     // They decided to close the chat
     _dlgCloseChat->hide();
     close();
}

void ChatMessageView::on_CloseChat_Keep_clicked()
{
     g_assert(_dlgCloseChat != NULL);
     // They decided to keep the chat open
     _dlgCloseChat->hide();
}

gint ChatMessageView::on_new_message_timer()
{
     // The timer has expired
     if (_new_message_timer.connected())
	  _new_message_timer.disconnect();

     return 0;
}

gint ChatMessageView::on_window_delete(GdkEventAny* e)
{
     // If they just got a message
     if (_new_message_timer.connected())
     {
	  _new_message_timer.disconnect();

	  // Display the question of doom: are they sure they want to close?
	  _dlgCloseChat->show();

	  // Stop the signal
	  gtk_signal_emit_stop_by_name(GTK_OBJECT(_thisWindow->gtkobj()), "delete-event");

	  return 1;
     }
     else
     {
	  close();

	  return 0;
     }

     return 0;
}

gint ChatMessageView::on_window_key_press(GdkEventKey* e)
{
     // If they pressed escape, make it close
     if (e->keyval == GDK_Escape)
	  on_Close_clicked();

     // If they pressed the Keypad enter, make it act like a normal enter
     if (e->keyval == GDK_KP_Enter)
	  e->keyval = GDK_Return;

     if (e->keyval == GDK_space && gtkspell_running())
	  gtkspell_check_all(_memMsg->gtkobj());
     else if (e->keyval == GDK_Return)
     {
	  //enter a newline if shift-return is used
	  if (e->state & GDK_SHIFT_MASK)
	  {
	       //unset the shift bit. shift-return seems to have a special meaning for the widget
	       e->state ^= GDK_SHIFT_MASK;
	       return 0;
	  }

	  // Ctrl-Return sends the message so we don't need to check for it  
          on_Send_clicked();
          gtk_signal_emit_stop_by_name(GTK_OBJECT(_thisWindow->gtkobj()), "key_press_event");
     }
     // If Ctrl-w is pressed, close this window
     else if ( (e->keyval == GDK_w) && (e->state & GDK_CONTROL_MASK) )
	  on_Close_clicked();

     return 0;
}

void ChatMessageView::on_memMsg_changed()
{
     // composing event was requested and a reply is being typed
     if (_composing_msg != NULL && _memMsg->get_length() > 0)
     {
	  G_App->getSession() << *_composing_msg;
	  _composing_event.disconnect();
     }
}

void ChatMessageView::on_AddUser_clicked()
{
     // Start AddContactDruid with that JID
     AddContactDruid::display(JID::getUserHost(_jid));
}

void ChatMessageView::on_EditUser_clicked()
{
     if (_onRoster)
          ContactInfoDlg::display(_jid, _thisWindow);
     else
          ContactInfoDlg::display(_jid, Roster::rsNone, _thisWindow);
}

void ChatMessageView::on_History_clicked()
{
     string file = "file://";
     file += string(G_App->getLogFile(_jid));
     gnome_url_show(file.c_str());
}

void ChatMessageView::on_presence(const Presence& p, const Presence::Type prev)
{
     // Display a notification message if this presence packet
     // is from the JID associated with this dialog
     if (p.getFrom() != _jid)
	  return;

     // If this is an error, send it to the error manager
     if (p.getBaseElement().getAttrib("type") == "error")
	  G_App->getErrorManager().add(p);

     // Show the extra bar if they are not available or chatty
     if (p.getShow() == Presence::stOnline || p.getShow() == Presence::stChat)
     {
	  getWidget<Gtk::HBox>("OOOChat_Show_hbox")->hide();
     }
     else
     {
	  getWidget<Gtk::HBox>("OOOChat_Show_hbox")->show();
	  // If there is an explanation to go along with the short presence, display it properly
	  if (!fromUTF8(p.getStatus()).empty())
	  {
	       getLabel("OOOChat_Show_lbl")->set_text(getShowName(p.getShow()) + ":");
	       Gtk::Label* l = getLabel("OOOChat_Status_lbl");
	       l->set_text(fromUTF8(l, p.getStatus()));
	  }
	  else
	  {
	       getLabel("OOOChat_Show_lbl")->set_text(getShowName(p.getShow()));
	  }
     }

     // Turn on the Encryption toggle if gpg is enabled, otherwise disable it
     _useGPG = MessageUtil::gpg_toggle(getWidget<Gtk::ToggleButton>("OOOChat_Encrypt_tglbtn"), p);
}

void ChatMessageView::on_error(const string& concerning, int errorcode, const string& errormsg, const string& body)
{
     // If it's not for this view, ignore
     if (JID::compare(concerning, _jid) != 0)
	  return;

     // Display the error
     string nick = _("Error");
     if (errorcode != 0)
     {
	  char* errnum;
	  errnum = g_strdup_printf(" %d", errorcode);
	  nick += errnum;
	  g_free(errnum);
     }
     _chatview->render_error(fromUTF8(GTK_WIDGET(_chatview->_xtext), errormsg), nick,
			     "", RED);
}

int ChatMessageView::on_GPGInfo_button_press(GdkEventButton* e)
{
     bool valid = false;

     if (_gpgInfo == GabberGPG::gpgNone)
	  return FALSE;
     else if (_gpgInfo == GabberGPG::gpgValidSigned || _gpgInfo == GabberGPG::gpgValidEncrypted)
	  valid = true;

     try {
	  string keyid = G_App->getGPG().find_jid_key(_jid);
	  GPGInfoDialog* dlg = manage(new GPGInfoDialog(keyid, valid));
	  dlg->getBaseDialog()->set_parent(*_thisWindow);
     } catch (GabberGPG::GPG_InvalidJID& e) {
	  GPGInfoDialog* dlg = manage(new GPGInfoDialog("", false));
	  dlg->getBaseDialog()->set_parent(*_thisWindow);
     }
     return TRUE;
}

void ChatMessageView::on_Chatview_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time)
{
     // Create a (hopefully) unique room name
     string room;

     room = JID::getUser(_jid) + "_" + G_App->getCfg().server.username;
     // room += "-" + yyyymmddThhmmss - get datetime like that
     room += "@conference.jabber.org"; // temp hack for now, until we have browsing
     
     GCIDlg* gd = manage(new GCIDlg(room, _subject, true)); // arg[2] says we want to join this group ourself when we click invite
     gd->add_jid(_jid, _nickname);
     gd->push_drag_data(drag_ctx, x, y, data, info, time);
}

// -------------------------------------------------------------------
//
// GC (GroupChat) Message View
//
// -------------------------------------------------------------------

// Init function to register view with MessageManager
void GCMessageView::init_view(MessageManager& mm)
{
     mm.register_view_type("groupchat", (MessageManager::MessageViewInfo::ViewInfoFlags)
				     (MessageManager::MessageViewInfo::vfNoFlash |
                                      MessageManager::MessageViewInfo::vfMultiMessage),
                           new_view_msg, new_view_jid, " ", " ");
}
 
MessageView* GCMessageView::new_view_msg(const Message& m, ViewMap& vm)
{
     MessageView* mv = manage(new GCMessageView(m, vm));
     return mv;
}
 
MessageView* GCMessageView::new_view_jid(const string& jid, ViewMap& vm)
{
     MessageView* mv = manage(new GCMessageView(jid, vm));
     return mv;
}

GCMessageView::GCMessageView(const jabberoo::Message& m, ViewMap& vm)
     : MessageView(m, vm, true), BaseGabberWindow("GC_win"),
       _room(JID::getUserHost(m.getFrom())), _nick(JID::getResource(m.getFrom()))
{
     // Initialize the window
     init();

     // display the message
     render(m);
}

GCMessageView::GCMessageView(const string& jid, ViewMap& vm)
     : MessageView(jid, vm, true), BaseGabberWindow("GC_win"),
       _room(JID::getUserHost(jid)), _nick(JID::getResource(jid))
{
     init();
}

GCMessageView::~GCMessageView()
{
     if (G_Win != NULL && G_Win->is_connected())
     {
	  // Notify GC server that we're ready to leave
	  G_App->getSession() << Presence(_room + "/" + _nick, Presence::ptUnavailable);
     }

    Gtk::CList::RowIterator it = _lstUsers->rows().begin();
    for (; it != _lstUsers->rows().end(); it++)
    {
       Gtk::CList::Row& row = *(it);
       std::string nick = toUTF8(_lstUsers, row[0].get_text());
       
       if(nick != _nick)
       {
           G_App->getSession().presenceDB().remove(_room + "/" + nick);
       }
    }

     delete _chatview;
}

void GCMessageView::init()
{
     _thisWindow->realize();

     // Setup pointers
     _memMessage = getWidget<Gtk::Text>("GC_Message_txt");
     if (gtkspell_running())
	  gtkspell_attach(_memMessage->gtkobj());
     manage(new GabberDnDText(_memMessage)); // Attach some drag and drop logic
     _hboxChatview = getWidget<Gtk::HBox>("GC_Chatview");
     _entSubject = getEntry("GC_Subject_txt");
     _lstUsers   = getWidget<Gtk::CList>("GC_Users_list");
     _lscroll    = getWidget<Gtk::ScrolledWindow>("GC_Users_scroll");
     _optShow    = getWidget<Gtk::OptionMenu>("GC_Show_opt");
     _btnHistory = getButton("GC_History_btn");
//     _btnClose   = getButton("GC_Close_btn");
     _tglUsers   = getWidget<Gtk::ToggleButton>("GC_Users_toggle");
     _hpane      = getWidget<Gtk::Paned>("GC_hpane");
     _lblUserCount = getLabel("GC_MemberNum_lbl");
     _baseGCU    = manage(new BaseGabberWidget("GCUser_menu", "GC_win"));
     _menuGCUser = static_cast<Gtk::Menu*>(_baseGCU->get_this_widget());
     _oldnick    = _nick;

     bool indent = true;
     // If they want timestamps, disable indentation
     if (G_App->getCfg().chats.grouptime)
          indent = false;

     // Initialize chat view
     _chatview = new ChatView(_thisWindow, _hboxChatview, indent);

     // Setup window handler
     _thisWindow->delete_event.connect(slot(this, &GCMessageView::on_window_delete));

     // Setup event handlers
     _memMessage->key_press_event.connect(slot(this, &GCMessageView::on_Message_key_press));
     _entSubject->key_press_event.connect(slot(this, &GCMessageView::on_Subject_key_press));
     _lstUsers->button_press_event.connect(slot(this, &GCMessageView::on_Roster_button_press));
     _btnHistory->clicked.connect(slot(this, &GCMessageView::on_History_clicked));
//     _btnClose->clicked.connect(slot(this, &GCMessageView::on_Close_clicked));
     _tglUsers->toggled.connect(slot(this, &GCMessageView::on_Users_toggled));
     getButton("GC_Invite_btn")->clicked.connect(slot(this, &GCMessageView::on_Invite_clicked));
     // Right-click menu
     _baseGCU->getMenuItem("GCUser_Message_item")->activate.connect(slot(this, &GCMessageView::on_Message_activate));
     _baseGCU->getMenuItem("GCUser_OOOChat_item")->activate.connect(slot(this, &GCMessageView::on_OOOChat_activate));
     _baseGCU->getMenuItem("GCUser_SendContacts_item")->activate.connect(slot(this, &GCMessageView::on_SendContacts_activate));
     _baseGCU->getMenuItem("GCUser_ViewInfo_item")->activate.connect(slot(this, &GCMessageView::on_ViewInfo_activate));

     // Setup the default pane settings
     _posHpane = 360;
     _tglUsers->set_active(true);
     _hpane->set_position(360);

     // Connect to the session disconnect event
     G_App->getSession().evtDisconnected.connect(slot(this, &GCMessageView::on_session_disconnected));
     G_App->getSession().evtPresence.connect(slot(this, &GCMessageView::on_session_presence));
     G_Win->evtMyPresence.connect(slot(this, &GCMessageView::on_presence_changed));
     G_App->getErrorManager().errorGroupchat.connect(slot(this, &GCMessageView::on_error));

     // Setup option menu
//FIXME: See GCMessageView::on_Show_selected
//     _menuShow.add_presence_items();
//     _menuShow.finish_items();
//     _menuShow.selected.connect(slot(this, &GCMessageView::on_Show_selected));
//     _optShow->set_menu(_menuShow.menu);
//     _optShow->set_history(0);

     // We want icon-only toolbar for now
     getWidget<Gtk::Toolbar>("GC_toolbar")->set_style(GTK_TOOLBAR_ICONS);

     // Set the window title and frame title
     string roomname = JID::getUser(_room);
     string fullname = JID::getUser(_room);
     string::size_type percent = fullname.find('%');
     if (percent != string::npos)
          fullname = roomname.substr(0, percent) + " on " + roomname.substr(percent + 1);
     else
          fullname = roomname + " on " + JID::getHost(_room);
     _thisWindow->set_title(substitute(_("Group Chat in %s"), fromUTF8(_thisWindow, roomname)) + _(" - Gabber"));
     Gtk::Label* l = getLabel("GC_Group_lbl");
     l->set_text(fromUTF8(l, (fullname)));

     // Pixmaps
     string pix_path = ConfigManager::get_PIXPATH();
     string window_icon = pix_path + "gnome-groupchat.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();

     // Word wrap
     _memMessage->set_word_wrap(true);

     // Ensure roster resizes properly, does auto-sort and has a proper
     // column height
     _lstUsers->set_column_auto_resize(0, true);
     if (((_lstUsers->get_style()->get_font().ascent() + _lstUsers->get_style()->get_font().descent() + 1) < 17))
	  _lstUsers->set_row_height(17);                                        // Hack for making sure icons aren't chopped off

     // Groupchat nicknames are case sensitive, so we use a case sensitive compare
     _lstUsers->set_compare_func(&GabberUtil::strcasecmp_clist_items);

     // Setup tab-completion buffer
     _completer = g_completion_new(NULL);


     // Setup DnD targets that we can receive
     GtkTargetEntry dnd_dest_targets[] = {
//	  {"text/x-jabber-roster-item", 0, 0},
	  {"text/x-jabber-id", 0, 0},
     };
     int dest_num = sizeof(dnd_dest_targets) / sizeof(GtkTargetEntry);
     gtk_drag_dest_unset(GTK_WIDGET(_hboxChatview->gtkobj()));
     gtk_drag_dest_set(GTK_WIDGET(_hboxChatview->gtkobj()), (GtkDestDefaults) (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP), dnd_dest_targets, dest_num, (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE));
     gtk_drag_dest_unset(GTK_WIDGET(_lstUsers->gtkobj()));
     gtk_drag_dest_set(GTK_WIDGET(_lstUsers->gtkobj()), (GtkDestDefaults) (GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP), dnd_dest_targets, dest_num, (GdkDragAction) (GDK_ACTION_COPY | GDK_ACTION_MOVE));

     // Setup DnD callbacks
     _hboxChatview->drag_data_received.connect(slot(this, &GCMessageView::on_drag_data_received));
     _lstUsers->drag_data_received.connect(slot(this, &GCMessageView::on_drag_data_received));

     // Join room on the server
     G_App->getSession() << Presence(_room + "/" + _nick, Presence::ptAvailable, G_App->getCfg().get_show(), G_App->getCfg().get_status());

     // Display
     _thisWindow->set_default_size(G_App->getCfg().chats.groupwidth, G_App->getCfg().chats.groupheight);
     _thisWindow->show();
}

void GCMessageView::render(const jabberoo::Message& m)
{
     // Convert from UTF8
     string fromnick = fromUTF8(GTK_WIDGET(_chatview->_xtext), JID::getResource(m.getFrom()));
     string body = fromUTF8(GTK_WIDGET(_chatview->_xtext), m.getBody());

     // A message from myself
     if (fromnick == fromUTF8(GTK_WIDGET(_chatview->_xtext), _nick))
     {
          // If they want timestamps, put them in
          if (G_App->getCfg().chats.grouptime)
               _chatview->render(body, fromnick, m.getDateTime(G_App->getCfg().dates.gcformat), BLUE2);
          else
               _chatview->render(body, fromnick, "", BLUE2);
     }
     // A system message
     else if (fromnick.empty())
     {
          // If they want timestamps, put them in
          if (G_App->getCfg().chats.grouptime)
               _chatview->render(body, fromnick, m.getDateTime(G_App->getCfg().dates.gcformat), AQUA);
          else
               _chatview->render(body, fromnick, "", AQUA);
     }
     // A message from someone else
     else
     {
	  bool highlight = false;
	  if (body.find(_nick) != string::npos)
	       highlight = true;

          // If they want timestamps, put them in
          if (G_App->getCfg().chats.grouptime)
	  {
	       if (highlight)
		    _chatview->render_highlight(body, fromnick, m.getDateTime(G_App->getCfg().dates.gcformat), RED, RED);
	       else
		    _chatview->render(body, fromnick, m.getDateTime(G_App->getCfg().dates.gcformat), RED);
	  }
          else
	  {
	       if (highlight)
		    _chatview->render_highlight(body, fromnick, "", RED, RED);
	       else
		    _chatview->render(body, fromnick, "", RED);
	  }
     }

     const string& subj = m.getSubject();
     if (!subj.empty())
     {
	  // I don't think we should set this, subjects are often long - julian
	  //_thisWindow->set_title(_("Gabber: Group Chat - ") + fromUTF8(_thisWindow, subj));
	  _entSubject->set_text(fromUTF8(_entSubject, subj));
     }

     // message displayed event
     if (fromnick.empty())
	  fromnick = _("Server");
     if (fromnick != _nick) // If it's not from us
	  G_App->getEventManager().message_displayed(m, fromnick, MessageManager::translateType(m));
}

gint GCMessageView::on_window_delete(GdkEventAny* e)
{
     close();
     return 0;
}

gint GCMessageView::on_Subject_key_press(GdkEventKey* e)
{
     // If they pressed the Keypad enter, make it act like a normal enter
     if (e->keyval == GDK_KP_Enter)
	  e->keyval = GDK_Return;

     if (e->keyval == GDK_Return && G_Win->is_connected())
     {
          string subject = toUTF8(_entSubject, _entSubject->get_text());
          if (!subject.empty())
          {
               // Setup a message object
               Message m(_room, substitute(toUTF8(_entSubject, _("%s has changed the subject to: %s")), "/me", subject), Message::mtGroupchat);
               m.setSubject(subject);
               // Transmit the subject change
               G_App->getSession() << m;
          }
     }
     return 0;
}

int GCMessageView::on_Roster_button_press(GdkEventButton* e)
{
     if (!_lstUsers->get_selection_info(e->x, e->y, &_row, &_col))
	  return 0;

     _lstUsers->row(_row).select();

     switch (e->type) {
     case GDK_2BUTTON_PRESS: // Double-click
     {
	  if (e->button == 1)
	  { 
	       // Check for control key..
	       GdkEventButton* ex = (GdkEventButton*)e;
	       if (ex->state & GDK_CONTROL_MASK)
	       {
		    if (G_App->getCfg().msgs.sendmsgs)
			 G_App->getMessageManager().display(_room + "/" + toUTF8(_lstUsers, _lstUsers->cell(_row, _col).get_text()), MessageManager::translateType("chat"));
		    else
			 G_App->getMessageManager().display(_room + "/" + toUTF8(_lstUsers, _lstUsers->cell(_row, _col).get_text()), MessageManager::translateType("normal"));
	       }
	       else
	       {
		    if (G_App->getCfg().msgs.sendmsgs)
			 G_App->getMessageManager().display(_room + "/" + toUTF8(_lstUsers, _lstUsers->cell(_row, _col).get_text()), MessageManager::translateType("normal"));
		    else
			 G_App->getMessageManager().display(_room + "/" + toUTF8(_lstUsers, _lstUsers->cell(_row, _col).get_text()), MessageManager::translateType("chat"));
	       }
	  }
	  break;
     }
     case GDK_BUTTON_PRESS: // Single-click
	  // Check for rt-click
	  if (e->button == 3) 
	  {
	       _menuGCUser->show_all();
	       _menuGCUser->popup(e->button, e->time);
	  }
	  break;
     default:
	  break;
     }

     return 0;
}

void GCMessageView::on_Message_activate()
{
     string gcuser = toUTF8(_lstUsers, _lstUsers->cell(_row, _col).get_text());
     G_App->getMessageManager().display(_room + "/" + gcuser, MessageManager::translateType("normal"));
}

void GCMessageView::on_OOOChat_activate()
{
     string gcuser = toUTF8(_lstUsers, _lstUsers->cell(_row, _col).get_text());
     G_App->getMessageManager().display(_room + "/" + gcuser, MessageManager::translateType("chat"));
}

void GCMessageView::on_SendContacts_activate()
{
     string gcuser = toUTF8(_lstUsers, _lstUsers->cell(_row, _col).get_text());
     ContactSendDlg* dlg = manage(new ContactSendDlg(_room + "/" + gcuser));
     dlg->getBaseDialog()->set_parent(*_thisWindow);
}

void GCMessageView::on_ViewInfo_activate()
{
     string gcuser = toUTF8(_lstUsers, _lstUsers->cell(_row, _col).get_text());
     ContactInfoDlg::display(_room + "/" + gcuser, Roster::rsNone, _thisWindow);
}

void GCMessageView::on_History_clicked()
{
     string file = "file://";
     file += string(G_App->getLogFile(_room));
     gnome_url_show(file.c_str());
}

void GCMessageView::on_Close_clicked()
{
     close();
}

void GCMessageView::on_Users_toggled()
{
     // If they want to see the users, set it back to the old position
     if (_tglUsers->get_active())
     {
	  getWidget<Gtk::VBox>("GC_UserList_vbox")->show();
	  _hpane->set_gutter_size(12);
	  _hpane->set_position(_thisWindow->width() - _posHpane);
     }
     else
     {
          // if they don't, remember old position, and set to a big number. xchat uses 1200
          _posHpane = _thisWindow->width() - (_hpane->gtkobj()->handle_xpos + 12); // Add the gutter size. Yeah, that kinda sucks
	  // At this point, _posHane holds the width of the GroupChat contact list, not the position.
	  _hpane->set_position(2400);
	  _hpane->set_gutter_size(0);
	  getWidget<Gtk::VBox>("GC_UserList_vbox")->hide();
     }
}

gint GCMessageView::on_Message_key_press(GdkEventKey* e)
{
     // If they pressed the Keypad enter, make it act like a normal enter
     if (e->keyval == GDK_KP_Enter)
	  e->keyval = GDK_Return;

     if (e->keyval == GDK_space && gtkspell_running())
	  gtkspell_check_all(_memMessage->gtkobj());
     else if (e->keyval == GDK_Return)
     {
	  //enter a newline if shift-return is used
	  if (e->state & GDK_SHIFT_MASK)
	  {
	       //unset the shift bit. shift-return seems to have a special meaning for the widget
	       e->state ^= GDK_SHIFT_MASK;
	       return 0;
	  }

          string msg = toUTF8(_memMessage, _memMessage->get_chars(0, -1));
          if ( (!msg.empty()) && (!process_command(msg)) && G_Win->is_connected())
          {
               // Transmit the message
               G_App->getSession() << Message(_room, msg, Message::mtGroupchat);
          }
          // Clear display
          _memMessage->delete_text(0, -1);
          // HACK --  to stop the signal from continuing, and the return from being left over in the message
          gtk_signal_emit_stop_by_name(GTK_OBJECT(_memMessage->gtkobj()), "key_press_event");
     }
     // Handle tab-completion
     else if (e->keyval == GDK_Tab)
     {
          string msg = toUTF8(_memMessage, _memMessage->get_chars(0, -1));
          // Look for the last word
          if (!msg.empty())
          {
               string lastword;
               // Search for the last whitespace
               string::size_type n = msg.find_last_of(" ");
               if (n != string::npos)
                    lastword = msg.substr(n+1);
               else
                    lastword = msg;

               // Try and autocomplete
               gchar* prefix;
               g_completion_complete(_completer, (char*)lastword.c_str(), &prefix);
               if (prefix != NULL)
               {
                    // Replace the last word in the message and update
                    if (n != string::npos)
                         msg.replace(msg.find_last_of(" ")+1, msg.length(), prefix);
                    else
                         msg = string(prefix);
                    g_free(prefix);

                    // Update text
                    _memMessage->delete_text(0, -1);
                    _memMessage->insert(fromUTF8(_memMessage, msg));
               }
               // HACK --  to stop the signal from continuing, and the entry from losing focus
               gtk_signal_emit_stop_by_name (GTK_OBJECT(_memMessage->gtkobj()), "key_press_event");
               return 1;
          }
     }
     // If Ctrl-w is pressed, close this window
     else if ( (e->keyval == GDK_w) && (e->state & GDK_CONTROL_MASK) )
	  close();

     return 0;
}

void GCMessageView::on_session_disconnected()
{
     // Notify user that the session is disconnected and disable the
     // input
     _memMessage->set_sensitive(false);
     _chatview->render(_("Session Disconnected"), "", "", YELLOW);
}

void GCMessageView::on_session_presence(const Presence& p, const Presence::Type prev)
{
     // If this packet isn't for this room, ignore it
     if (JID::compare(JID::getUserHost(p.getFrom()), _room) != 0)
          return;

     // If this is an error, send it to the error manager
     if (p.getType() == Presence::ptError)
     {
	  G_App->getErrorManager().add(p);
	  // If this was a nick conflict error, 
	  // then the presence we got wasn't really from the user it claims 
	  // - that was the nick we tried to change to. So ignore this presence
	  if (p.getErrorCode() == 409)
	       return;
     }

     // Grab iterators from the presencedb for this room jid
     PresenceDB::range r;
     try
     {
        r = G_App->getSession().presenceDB().equal_range(_room);
     } 
     catch (PresenceDB::XCP_InvalidJID) 
     {
        // Renaming, or something weird.  Bail.
        return;
     }

     // Freeze the roster list
     _lstUsers->freeze();
     float hscroll, vscroll;
     hscroll = _lscroll->get_hadjustment()->get_value(); // Save the horizontal scroll position
     vscroll = _lscroll->get_vadjustment()->get_value(); // Save the vertical scroll position
     _lstUsers->clear();

     // Get a ref to the config manager
     ConfigManager& cfgm = G_App->getCfg();

     // Clear the completion list
     g_completion_clear_items(_completer);

     // Setup a G_List to hold all user strings
     GList* l = NULL;

     // Define a counter to hold # of users
     int usercount = 0;

     for (PresenceDB::const_iterator it = r.first; it != r.second; it++, usercount++)
     {
          // Grab presence reference
          const Presence& p = *it;

          // If this presence is a NA presence, then skip it
          if (p.getType() == Presence::ptUnavailable)
               continue;

          // Create a row in the list
          const char* data[1] = {""};
          int row = _lstUsers->append(data);

          // Extract the resource
          const string& s = JID::getResource(p.getFrom());

          // Create an entry in the roster
          gtk_clist_set_foreground(_lstUsers->gtkobj(), row, cfgm.getPresenceColor(p.getShow_str()));
          gtk_clist_set_pixtext(_lstUsers->gtkobj(), row, 0, fromUTF8(_lstUsers, s).c_str(), 5,
                                cfgm.getPresencePixmap(p.getShow_str()),
                                cfgm.getPresenceBitmap(p.getShow_str()));

          // Add an entry to the completion list
          l = g_list_append(l, g_strdup(s.c_str()));
     }

     // Sort the list
     _lstUsers->sort();

     // Update user count widget
     char ucount_str[15];
     g_snprintf((char*)&ucount_str, 15, _("%d members"), usercount);
     _lblUserCount->set_text((char*)&ucount_str);

     // Add /commands into the completer list
     l = g_list_append(l, g_strdup("/nick"));
     l = g_list_append(l, g_strdup("/quit"));
     l = g_list_append(l, g_strdup("/query"));
     l = g_list_append(l, g_strdup("/msg"));
     l = g_list_append(l, g_strdup("/topic"));
     l = g_list_append(l, g_strdup("/clear"));
     l = g_list_append(l, g_strdup("/help"));
     l = g_list_append(l, g_strdup("/whois"));
     l = g_list_append(l, g_strdup("/away"));
     l = g_list_append(l, g_strdup("/join"));

     // Update completer
     g_completion_add_items(_completer, l);

     // Reset the scroll position
     _lscroll->get_hadjustment()->set_value(hscroll); // Set the horizontal scroll position
     _lscroll->get_vadjustment()->set_value(vscroll); // Set the horizontal scroll position

     _lstUsers->thaw();
}

void GCMessageView::on_presence_changed(const Presence& p)
{
     _current_show = p.getShow();
     if (p.getType() == Presence::ptInvisible) // Special case for invisible: we'll treat it as available in gc
     {
	  _current_show = Presence::stOnline;
     }
     if (G_Win && G_Win->is_connected())
     {
	  G_App->getSession() << Presence(_room + "/" + _nick, Presence::ptAvailable, _current_show, p.getStatus()); 
	  if (_current_show != Presence::stInvalid && _current_show != Presence::stOffline)
	       _memMessage->set_sensitive(true);
     }
}

void GCMessageView::on_error(const string& concerning, int errorcode, const string& errormsg, const string& body)
{
     // If it's not for this view, ignore
     if (JID::compare(concerning, _room) != 0)
	  return;

     // If this was a nickname conflict error, set the nickname back to the old one
     if (errorcode == 409)
	  _nick = _oldnick;

     // Display the error
     string nick = _("Error");
     if (errorcode != 0)
     {
	  char* errnum;
	  errnum = g_strdup_printf(" %d", errorcode);
	  nick += errnum;
	  g_free(errnum);
     }
     _chatview->render_error(fromUTF8(GTK_WIDGET(_chatview->_xtext), errormsg), nick,
			     "", RED);
}

bool GCMessageView::process_command(const string& cmd)
{
     // Dummy check..
     if (cmd.at(0) != '/')
          return false;

     // NICK handler
     if ((cmd.substr(0,6) == "/nick ") && (cmd.length() > 6))
     {
          string newnick = cmd.substr(6);
          if (!newnick.empty())
          {
	       _oldnick = _nick;
               _nick = newnick;
               G_App->getSession() << Presence(_room + "/" + _nick, Presence::ptAvailable, _current_show);
          }
          return true;
     }
     // QUERY handler
     else if ((cmd.substr(0,6) == "/query") && (cmd.length() > 7))
     {
          string user = cmd.substr(7);
          if (!user.empty())
               G_App->getMessageManager().display(_room + "/" + user, MessageManager::translateType("chat"));
          return true;
     }
     // MSG handler
     else if ((cmd.substr(0,4) == "/msg") && (cmd.length() > 5))
     {
          string user = cmd.substr(5);
          if (!user.empty())
               G_App->getMessageManager().display(_room + "/" + user, MessageManager::translateType("normal"));
          return true;
     }
     // TOPIC handler
     else if ((cmd.substr(0,6) == "/topic") && (cmd.length() > 7))
     {
	  string subject = cmd.substr(7);
	  // Setup a message object
	  Message m(_room, substitute(toUTF8(_entSubject, _("%s has changed the subject to: %s")), "/me", subject), Message::mtGroupchat);
	  m.setSubject(subject);
	  // Transmit the subject change
	  G_App->getSession() << m;
	  return true;
     }
     // WHOIS handler
     else if ((cmd.substr(0,6) == "/whois") && (cmd.length() > 7))
     {
	  string gcuser = cmd.substr(7);
	  ContactInfoDlg::display(_room + "/" + gcuser, Roster::rsNone, _thisWindow);
	  return true;
     }
     // QUIT handler
     else if (cmd.substr(0,5) == "/quit")
     {
          close();
          return true;
     }
     // HELP handler
     else if (cmd.substr(0,5) == "/help")
     {
	  GnomeHelpMenuEntry help_entry = { "gabber", "msg.html#MSG-GC-MAIN" };
	  gnome_help_display (NULL, &help_entry);
	  return true;
     }
     // CLEAR handler
     else if (cmd.substr(0,6) == "/clear")
     {
	  _chatview->clearbuffer();
	  return true;
     }
     // AWAY handler
     else if (cmd.substr(0,6) == "/away")
     {
	  string status;
	  if (cmd.length() > 7)
	       status = cmd.substr(7);
	  else
	       status = _("I'm away.");
	  G_Win->display_status(jabberoo::Presence::stAway, status, G_App->getCfg().get_priority());
	  return true;
     }
     // JOIN handler
     else if ((cmd.substr(0,5) == "/join") && (cmd.length() > 6))
     {
	  string newroom = cmd.substr(6);
	  if (newroom.find("@") == string::npos)
	  {
	       // Join given_room @ current_server / current_nick
	       GCMessageView::join(newroom + "@" + JID::getHost(_room) + "/" + _nick);
	  }
	  else
	  {
	       // Join given_room / current_nick
	       GCMessageView::join(newroom + "/" + _nick);
	  }
	  return true;
     }
     return false;
}

void GCMessageView::on_Show_selected(int show_index)
{
     // FIXME: See GabberWin::on_Show_selected and make this work like that
     // Set the global _current_show
     //_current_show = indexShow(show_index);

     // Output new presence for this jid
     //G_App->getSession() << Presence(_room + "/" + _nick, Presence::ptAvailable, _current_show);
}

void GCMessageView::join(const string& jid)
{
     // Case 1: No jid is provided -- exit the procedure
     if (jid.empty())
          return;
     // Case 2: JID is provided, but no window can be found -- create a new window
     else if (!G_App->getMessageManager().hasView(jid, MessageManager::translateType("groupchat")))
          G_App->getMessageManager().display(jid, MessageManager::translateType("groupchat"));
     // Case 3: JID is provided and a window was found -- send new presence packet
     else
          G_App->getSession() << Presence(jid, Presence::ptAvailable, G_App->getCfg().get_show(), G_App->getCfg().get_status());
}

void GCMessageView::on_Invite_clicked()
{
     GCIDlg* gd = manage(new GCIDlg(_room, toUTF8(_entSubject, _entSubject->get_text())));
     gd->getBaseDialog()->set_parent(*_thisWindow);
}

void GCMessageView::on_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time)
{
     GCIDlg* gd = manage(new GCIDlg(_room, toUTF8(_entSubject, _entSubject->get_text())));
     gd->push_drag_data(drag_ctx, x, y, data, info, time);
}

// -------------------------------------------------------------------
//
// Join Group Chat Dialog
//
// -------------------------------------------------------------------

void GCJoinDlg::execute()
{
     manage(new GCJoinDlg());
}

GCJoinDlg::GCJoinDlg()
     : BaseGabberDialog("GCJoin_dlg")
{
     // Setup buttons
     getButton("GCJoin_OK_btn")->clicked.connect(slot(this, &GCJoinDlg::on_ok_clicked));
     getButton("GCJoin_Cancel_btn")->clicked.connect(slot(this, &GCJoinDlg::on_cancel_clicked));
     
     // Connect widget(s)
     _optProtocol = getWidget<Gtk::OptionMenu>("GCJoin_Protocol_opt");
     _thisWindow->key_press_event.connect(slot(this, &GCJoinDlg::on_key_pressed));
     _entNick   = getEntry("GCJoin_Nick_ent");
     _entNick   ->changed.connect(slot(this, &GCJoinDlg::changed));
     _entRoom   = getEntry("GCJoin_Room_ent");
     _entRoom   ->changed.connect(slot(this, &GCJoinDlg::changed));
     _entServer = getEntry("GCJoin_Server_ent");
     _entServer ->changed.connect(slot(this, &GCJoinDlg::changed));
     _entIRCServer = getEntry("GCJoin_IRCServer_ent");
     _gentNick  = getWidget<Gnome::Entry>("GCJoin_Nick_gent");
     _gentRoom  = getWidget<Gnome::Entry>("GCJoin_Room_gent");
     _gentServer = getWidget<Gnome::Entry>("GCJoin_Server_gent");

     // Grab the history and put it in the gnome entries (a cooler combo box)
     G_App->getCfg().loadEntryHistory(_gentNick);
     G_App->getCfg().loadEntryHistory(_gentRoom);
     G_App->getCfg().loadEntryHistory(_gentServer);
     G_App->getCfg().loadEntryHistory(getWidget<Gnome::Entry>("GCJoin_IRCServer_gent"));

     // Set the default nick
     _entNick->set_text(fromUTF8(_entNick, G_App->getCfg().get_nick()));

     // Let's give them some rooms to join and default it to conference.jabber.org - for now
     _gentRoom->prepend_history(false, "jabber");
     _gentRoom->prepend_history(false, "jdev");
     _entServer->set_text("conference.jabber.org");

     // Make the enter cycle nice
     _entNick->activate.connect(_entRoom->grab_focus.slot());
     _entRoom->activate.connect(_entServer->grab_focus.slot());
     _entServer->activate.connect(slot(this, &GCJoinDlg::on_Server_activate));
     _entIRCServer->activate.connect(slot(this, &GCJoinDlg::on_ok_clicked));

     // Give nickname the focus unless there is a nickname set
     if (_entNick->get_text().empty())
	  _entNick->grab_focus();
     else
	  _entRoom->grab_focus();

     // Setup the option menu
     _protocol = 1;
     _menuProtocol.add_item(_("Jabber Group Chat 1.0"), 1);
     _menuProtocol.add_item(_("IRC (Group Chat 1.0)"), 2);
     _menuProtocol.finish_items();
     _menuProtocol.selected.connect(slot(this, &GCJoinDlg::on_protocol_selected));
     _optProtocol->set_menu(_menuProtocol.get_menu());
     _optProtocol->set_history(0);

     // Pixmaps
     string pix_path = ConfigManager::get_PIXPATH();
     string window_icon = pix_path + "gnome-groupchat.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();

     // Poll the agents of this server and add the list of groupchat agents
     G_App->getSession().queryNamespace("jabber:iq:agents", slot(this, &GCJoinDlg::on_agents_reply), G_App->getCfg().get_server());

     show();
}

void GCJoinDlg::on_Server_activate()
{
     // If the IRC Server field is sensitive, then focus that, otherwise act like Join Room was clicked
     if (_entIRCServer->is_sensitive())
	  _entIRCServer->grab_focus();
     else
	  on_ok_clicked();
}

void GCJoinDlg::on_ok_clicked()
{
     if (_entRoom->get_text().empty()
	 || _entServer->get_text().empty()
	 || _entNick->get_text().empty())
	  return;
     
     // Get values from UI
     string handle = toUTF8(_entNick, _entNick->get_text());
     string room   = toUTF8(_entRoom, _entRoom->get_text());
     string server = toUTF8(_entServer, _entServer->get_text());
     string ircserver = toUTF8(_entIRCServer, _entIRCServer->get_text());
     
     string id;
     // If they're using IRC, set the irc server
     if (_protocol == 2 && !ircserver.empty())
	  id = room + "%" + ircserver + "@" + server + "/" + handle;
     else
	  id = room + "@" + server + "/" + handle;
     GCMessageView::join(id);
     // Save the gnome entry history
     G_App->getCfg().saveEntryHistory(_gentNick);
     G_App->getCfg().saveEntryHistory(_gentRoom);
     G_App->getCfg().saveEntryHistory(_gentServer);
     G_App->getCfg().saveEntryHistory(getWidget<Gnome::Entry>("GCJoin_IRCServer_gent"));
     close();
}
	 
void GCJoinDlg::on_cancel_clicked()
{
     close();
}

gint GCJoinDlg::on_key_pressed(GdkEventKey* e)
{
     // If Enter is pressed, attempt to continue
     switch (e->keyval)
     {
     case GDK_Escape:
	  on_cancel_clicked();
     }
     return FALSE;
}     

void GCJoinDlg::on_protocol_selected(int protocol)
{
     _protocol = protocol;
     switch (protocol)
     {
     case 1:
	  // Jabber
	  getWidget<Gtk::HBox>("GCJoin_IRCServer_hbox")->set_sensitive(false);
	  break;
     case 2:
	  getWidget<Gtk::HBox>("GCJoin_IRCServer_hbox")->set_sensitive(true);
	  break;
     }
}

void GCJoinDlg::changed()
{
     // If they left something out, then disable the OK button
     if (_entRoom->get_text().empty()
	 || _entServer->get_text().empty()
	 || _entNick->get_text().empty())
	  getButton("GCJoin_OK_btn")->set_sensitive(false);
     else
	  getButton("GCJoin_OK_btn")->set_sensitive(true);
}

void GCJoinDlg::on_agents_reply(const Element& iq)
{
     const Element* query = iq.findElement("query");
     Element::const_iterator it = query->begin();

     if (iq.cmpAttrib("type", "result")) 
     {
          for (; it != query->end(); it++)
          {
	       if ((*it)->getType() != Node::ntElement)
		    continue;
	       Element& agent = *static_cast<Element*>(*it);

	       if (agent.getName() != "agent")
                    continue;

               string jid = agent.getAttrib("jid");
	       Element* groupchat = agent.findElement("groupchat");

	       // Only add agents that have a groupchat tag
	       if (groupchat)
	       {
	            _gentServer->prepend_history(false, jid);
	       }
          }
     }
}


// -------------------------------------------------------------------
//
// Autoupdate Dialog
//
// -------------------------------------------------------------------

void AutoupdateDlg::init_view(MessageManager& mm)
{
     mm.register_view_type("jabber:x:autoupdate",
			   (MessageManager::MessageViewInfo::ViewInfoFlags)
                            (MessageManager::MessageViewInfo::vfExtension |
			     MessageManager::MessageViewInfo::vfNoFlash |
			     MessageManager::MessageViewInfo::vfPopup),
			   new_view, NULL, " ", "#000000");
}

MessageView* AutoupdateDlg::new_view(const Message& m, ViewMap& vm)
{
     MessageView* mv = manage(new AutoupdateDlg(m, vm));
     return mv;
}

AutoupdateDlg::AutoupdateDlg(const Message& m, ViewMap& vm)
     : BaseGabberDialog("Autoupdate_dlg"), MessageView(m, vm)
{
     // Hook up the buttons
     getButton("Autoupdate_OK_btn")->clicked.connect(slot(this, &AutoupdateDlg::on_OK_clicked));
     _thisWindow->delete_event.connect(slot(this, &AutoupdateDlg::on_window_delete));

     render(m);
}

void AutoupdateDlg::render(const Message& m)
{
     // The contents of the message aren't used right now, we simply assume
     // it's a gabber update.  This could be extended later for other updates

     // Get next session ID
     string id = G_App->getSession().getNextID();
     // Construct update info request
     Packet iq("iq");
     iq.setID(id);
     // Gabber's clientID on jabbercentral
     string autoupdateJID = "956878967";
     // the only place to grab updates right now
     autoupdateJID += "@update.jabber.org";
     iq.setTo(autoupdateJID);
     iq.getBaseElement().putAttrib("type", "get");
     Element* query = iq.getBaseElement().addElement("query");
     query->putAttrib("xmlns", "jabber:iq:autoupdate");
     // Send the update info request
     G_App->getSession() << iq;
     G_App->getSession().registerIQ(id, slot(this, &AutoupdateDlg::on_update_reply));
}

void AutoupdateDlg::on_update_reply(const Element& t)
{
     if (!t.empty())
     {
          const Element* query = t.findElement("query");
          if (query != NULL)
          {
               const Element* release = query->findElement("release");
               string curtext;
               if (release)
               {
                    // If they've already seen this update and don't want to see it again,
		    // close the update window.  (It isn't displayed yet but close to 
		    // destory it).  
                    if (G_App->getCfg().server.lastupdate == release->getChildCData("version"))
                         close();

		    // Fill in information about the update
		    Gtk::Label* l = getLabel("Autoupdate_UpdateVer_lbl");
		    l->set_text(fromUTF8(l, (string("Gabber ") + release->getChildCData("version"))));
		    l = getLabel("Autoupdate_UpdateDesc_lbl");
		    l->set_text(fromUTF8(l, release->getChildCData("desc")));
		    Gnome::HRef* b = getWidget<Gnome::HRef>("Autoupdate_UpdateURI_href");
		    b->set_url(fromUTF8(b, release->getChildCData("url")));
		    b->set_label(fromUTF8(b, release->getChildCData("url")));

		    // show the view
		    show();

		    return;
               }
          }
     }
     // If we reach here then the message view wasn't displayed so close to destroy it
     close();
}

void AutoupdateDlg::on_OK_clicked()
{
     // If they uncheck "display this again"
     if (!getCheckButton("AutoUpdate_DisplayAgain_chk")->get_active())
     {
	  // Save the version which was reported as being new
	  Gtk::Label* l = getLabel("Autoupdate_UpdateVer_lbl");
	  G_App->getCfg().server.lastupdate = _version;
     }
     close();
}

gint AutoupdateDlg::on_window_delete(GdkEventAny* e)
{
     close();
     return 0;
}
