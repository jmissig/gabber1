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

#include "WelcomeDruid.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "GabberWin.hh"
#include "TCPTransmitter.hh"

#include <libgnome/gnome-i18n.h>
#include <gtk--/spinbutton.h>

using namespace jabberoo;
using namespace GabberUtil;

WelcomeDruid* WelcomeDruid::_Dialog = NULL;

WelcomeDruid::WelcomeDruid()
     : BaseGabberWindow("Welcome_win")
{
     // Connect buttons to handlers
     _druid = getWidget<Gnome::Druid>("Welcome_Druid");
     _druid->cancel.connect(slot(this, &WelcomeDruid::OnCancel));

     // Connect the pages
     Gnome::DruidPage* page = getWidget<Gnome::DruidPage>("Welcome_Password");
     page->prepare.connect(slot(this, &WelcomeDruid::OnPasswordPrepare));
     page = getWidget<Gnome::DruidPage>("Welcome_Account");
     page->prepare.connect(slot(this, &WelcomeDruid::OnAccountPrepare));
     page = getWidget<Gnome::DruidPage>("Welcome_Confirm");
     page->prepare.connect(slot(this, &WelcomeDruid::OnConfirmPrepare));
     page = getWidget<Gnome::DruidPage>("Welcome_LoggingIn");
     page->prepare.connect(slot(this, &WelcomeDruid::OnLoggingInPrepare));
     page = getWidget<Gnome::DruidPage>("Welcome_Finish");
     page->finish.connect(slot(this, &WelcomeDruid::OnFinish));

     // Connect widgets
     _entFirstName = getEntry("Welcome_FirstName_ent");
     _entLastName  = getEntry("Welcome_LastName_ent");
     _entUsername  = getEntry("Welcome_Username_ent");
     _entUsername  ->changed.connect(slot(this, &WelcomeDruid::OnAccountChanged));
     _entServer    = getEntry("Welcome_Server_ent");
     _entServer    ->changed.connect(slot(this, &WelcomeDruid::OnAccountChanged));
     _entResource  = getEntry("Welcome_Resource_ent");
     _entResource  ->changed.connect(slot(this, &WelcomeDruid::OnResourceChanged));
     _entPassword  = getEntry("Welcome_Password_ent");
     _entPassword  ->changed.connect(slot(this, &WelcomeDruid::OnPasswordChanged));
     _chkSavePassword = getCheckButton("Welcome_SavePassword_chk");
     _entConfirmPassword = getEntry("Welcome_ConfirmPassword_ent");
     _entConfirmPassword->changed.connect(slot(this, &WelcomeDruid::OnPasswordChanged));
     _lblProgress  = getLabel("Welcome_LoggingIn_progress_lbl");
     _barProgress  = getWidget<Gtk::ProgressBar>("Welcome_LoggingIn_progress_bar");

     // Connect the Enter sequence
     // - Personal Info
     getEntry("Welcome_Nickname_ent")->activate.connect(_entFirstName->grab_focus.slot());
     _entFirstName->activate.connect(_entLastName->grab_focus.slot());
     _entLastName->activate.connect(getEntry("Welcome_eMail_ent")->grab_focus.slot());
//     getEntry("Welcome_eMail_ent")->activate.connect(getWidget<Gnome::DruidPage>("Welcome_Name")->next.slot());
     // - Jabber Account
     _entUsername->activate.connect(_entServer->grab_focus.slot());
//     _entServer->activate.connect(getWidget<Gtk::SpinButton>("Welcome_Port_spin")->grab_focus.slot());
//     getWidget<Gtk::SpinButton>("Welcome_Port_spin")->activate.connect(getWidget<Gnome::DruidPage>("Welcome_Account")->next.slot());
     // - Password
     _entPassword->activate.connect(_entConfirmPassword->grab_focus.slot());
//     _entConfirmPassword->activate.connect(getWidget<Gnome::DruidPage>("Welcome_Password")->next.slot());
     // - Resource
//     _entResource->activate.connect(getWidget<Gnome::DruidPage>("Welcome_Resource")->next.slot());

     _tried_connecting = false;

     // Display
     show();
}

WelcomeDruid::~WelcomeDruid()
{
     _Dialog = NULL;
}    

void WelcomeDruid::execute()
{
     // Create a welcome window
     _Dialog = manage(new WelcomeDruid());
}

bool WelcomeDruid::isRunning()
{
     // Return whether or not we're in the welcome druid
     return (_Dialog != NULL);
}

bool WelcomeDruid::hasTriedConnecting()
{
     // If we're currently running
     if (_Dialog != NULL)
     {
	  // if we haven't tried, set it so we have and return
	  if (!_Dialog->_tried_connecting)
	  {
	       _Dialog->_tried_connecting = true;
	       return false;
	  }
	  else
	       return true;
     }
     // Else they're not in the welcome druid, so they have
     else
	  return true;
}

void WelcomeDruid::OnCancel()
{
     if (G_Win == NULL)
     {
	  G_App->getCfg().wizards.welcomehasrun = false;
	  G_App->quit();
     }
     close();
}

void WelcomeDruid::OnAccountChanged()
{
     g_assert( _entUsername != NULL );
     g_assert( _druid != NULL );

     // If the username is empty, don't let them click next
     if (JID::isValidUser(toUTF8(_entUsername, _entUsername->get_text())) && 
	 JID::isValidHost(toUTF8(_entServer, _entServer->get_text())))
	  _druid->set_buttons_sensitive(true,true,true);
     else
	  _druid->set_buttons_sensitive(true,false,true);
}

void WelcomeDruid::OnResourceChanged()
{
     g_assert( _entResource != NULL );
     g_assert( _druid != NULL );

     // If the resource is empty, don't let them click next
     if (_entResource->get_text().empty())
	  _druid->set_buttons_sensitive(true,false,true);
     else
	  _druid->set_buttons_sensitive(true,true,true);
}

void WelcomeDruid::OnAccountPrepare()
{
     // Default the username to be the real username
     if (_entUsername->get_text().empty())
	  _entUsername->set_text(g_get_user_name());
}

void WelcomeDruid::OnPasswordPrepare()
{
     g_assert( _entPassword != NULL );
     g_assert( _druid != NULL );

     // If the password hasn't been set, or the passwords aren't equal
     // Don't let them click next
     if (_entPassword->get_text().empty() 
	 || _entPassword->get_text() != _entConfirmPassword->get_text())
	  _druid->set_buttons_sensitive(true,false,true);
}

void WelcomeDruid::OnPasswordChanged()
{
     g_assert( _entPassword != NULL );
     g_assert( _druid != NULL );

     // If the passwords are equal, allow them to click next
     if (_entPassword->get_text() == _entConfirmPassword->get_text() && !_entPassword->get_text().empty())
	  _druid->set_buttons_sensitive(true,true,true);
     else
	  _druid->set_buttons_sensitive(true,false,true);
}

void WelcomeDruid::OnConfirmPrepare()
{
     // Show them their user@server and user@server/resource JIDs
     // I think it's helpful for new users
     Gtk::Label* l = getLabel("Welcome_JabberID_lbl");
     l->set_text(_entUsername->get_text() + "@" + _entServer->get_text());
     l->queue_resize();
     l = getLabel("Welcome_JIDResource_lbl");
     l->set_text(_entUsername->get_text() + "@" + _entServer->get_text() + "/" + _entResource->get_text());
}

void WelcomeDruid::OnLoggingInPrepare()
{
     g_assert( _druid != NULL );
     g_assert( _entFirstName != NULL );
     g_assert( _entLastName != NULL );
     g_assert( _entUsername != NULL );
     g_assert( _entServer != NULL );
     g_assert( _entPassword != NULL );
     g_assert( _chkSavePassword != NULL );
     g_assert( _entResource != NULL );

     // Don't let them click next
     _druid->set_buttons_sensitive(true,false,true);

     // After they have filled out new account information
     // we try saving config and logging in
     Gtk::Entry* entEMail     = getEntry("Welcome_eMail_ent");
     Gtk::Entry* entNickname  = getEntry("Welcome_Nickname_ent");
     Gtk::SpinButton* spinPort = getWidget<Gtk::SpinButton>("Welcome_Port_spin");

     g_assert( entEMail != NULL );
     g_assert( entNickname != NULL );
     g_assert( spinPort != NULL );

     if (JID::isValidUser(toUTF8(_entUsername, _entUsername->get_text())) && 
	 JID::isValidHost(toUTF8(_entServer, _entServer->get_text())) &&
	 !_entPassword->get_text().empty() && 
	 !_entResource->get_text().empty())
     {
	  // Save current config
	  ConfigManager& c = G_App->getCfg();
	  // User Info
	  _first = toUTF8(_entFirstName, _entFirstName->get_text());
	  _last = toUTF8(_entLastName, _entLastName->get_text());
	  _email = toUTF8(entEMail, entEMail->get_text());
	  if (entNickname->get_text().empty())
	       _nick = toUTF8(_entUsername, _entUsername->get_text());
	  else
	       _nick = toUTF8(entNickname, entNickname->get_text());
	  c.user.firstname = _first;
	  c.user.lastname = _last;
	  c.user.fullname = _first + " " + _last;
	  c.user.email = _email;
	  c.set_nick(_nick);

	  // Account Info
	  c.server.username = toUTF8(_entUsername, _entUsername->get_text());
	  c.set_server(toUTF8(_entServer, _entServer->get_text()));
	  c.server.port = spinPort->get_value_as_int();
	  c.server.password = toUTF8(_entPassword, _entPassword->get_text());
	  c.server.savepassword = _chkSavePassword->get_active();
	  c.server.resource = toUTF8(_entResource, _entResource->get_text());
	  c.wizards.loggedin = false;

	  // Pop up the GabberWin
	  G_App->init_win();

	  // Try logging in
	  G_App->login();
	  _lblProgress->set_text(_("Attempting to log in..."));
	  _barProgress->set_activity_mode(true);
	  _refresh_timer = Gtk::Main::timeout.connect(slot(this, &WelcomeDruid::on_refresh), 50);
     }
}

void WelcomeDruid::Connected()
{
     g_assert( _Dialog != NULL );
     g_assert( _Dialog->_druid != NULL );

     // Be *sure* it's running
     if (_Dialog != NULL)
     {
	  // If they want to register in JUD, do so, otherwise, they're done
	  if (_Dialog->getCheckButton("Welcome_JUD_chk")->get_active())
	  {
	       _Dialog->_lblProgress->set_text(_("Login successful. Contacting the Jabber Users Directory...."));
	       _Dialog->get_jud_key("users.jabber.org");
	  }
	  else
	  {
	       _Dialog->_lblProgress->set_text(_("Login successful."));
	       _Dialog->finished();
	  }
     }
}

void WelcomeDruid::get_jud_key(const string& jid)
{
     // Get next session ID
     string id = G_App->getSession().getNextID();

     // Construct vCard request
     Packet iq("iq");
     iq.setID(id);
     iq.setTo(jid);
     iq.getBaseElement().putAttrib("type", "get");
     Element* query = iq.getBaseElement().addElement("query");
     query->putAttrib("xmlns", "jabber:iq:register");

     // Send the vCard request
     G_App->getSession() << iq;
     G_App->getSession().registerIQ(id, slot(this, &WelcomeDruid::get_jud));
}

void WelcomeDruid::get_jud(const Element& t)
{
     if (!t.empty())
     {
	  const Element* query = t.findElement("query");
	  if (query != NULL)
	  {
	       // - JUD
	       // Get next session ID
	       string id = G_App->getSession().getNextID();

	       // Construct iq
	       Packet iq("iq");
	       iq.setID(id);
	       iq.setTo(t.getAttrib("from"));
	       iq.getBaseElement().putAttrib("type", "set");
	       
	       // Generate a query element
	       Element * q = iq.getBaseElement().addElement("query");
	       q->addElement("first", _first);
	       q->addElement("last", _last);
	       q->addElement("nick", _nick);
	       q->addElement("email", _email);
	       q->addElement("key", query->getChildCData("key"));

	       // Add necessary flags to the tag
	       q->putAttrib("xmlns", "jabber:iq:register");
	       
	       // Send IQ header
	       G_App->getSession() << iq; 
	       G_App->getSession().registerIQ(id, slot(this, &WelcomeDruid::jud_finished));
	       
	       // Inform the user
	       _lblProgress->set_text(_("Jabber Users Directory contacted, sending information..."));
	  }
     }
}

void WelcomeDruid::jud_finished(const Element& t)
{
     _lblProgress->set_text(_("Information sent."));
     finished();
}

void WelcomeDruid::finished()
{
     // Send them to the finish page, and don't let them go back
     _druid->set_page(*_Dialog->getWidget<Gnome::DruidPage>("Welcome_Finish"));
     _druid->set_buttons_sensitive(false,true,true);
     
     // Status bar message
     _barProgress->set_activity_mode(false);
     if (_refresh_timer.connected())
	  _refresh_timer.disconnect();
}

void WelcomeDruid::OnFinish()
{
     // Exit 
     close();
}

gint WelcomeDruid::on_refresh()
{
     // Refresh the progressbar
     float pvalue = _barProgress->get_current_percentage();
     pvalue += 0.01;

     if (pvalue <= 0.01)
     {
	  _barProgress->set_orientation(GTK_PROGRESS_LEFT_TO_RIGHT);
	  pvalue = 0.01;
     }
     else if (pvalue >= 0.99)
     {
	  _barProgress->set_orientation(GTK_PROGRESS_RIGHT_TO_LEFT);
	  pvalue = 0.01;
     }

     _barProgress->set_percentage(pvalue);

     return TRUE;
}

