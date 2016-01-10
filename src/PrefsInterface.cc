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

#include "PrefsInterface.hh"

#include "GabberApp.hh"
#include "GabberGPG.hh"
#include "GabberUtility.hh"
#include "GabberWin.hh"
#include "RosterView.hh"
#include "TCPTransmitter.hh"
#include "WelcomeDruid.hh"

#include <libgnome/gnome-help.h>
#include <gtk--/table.h>

using namespace GabberUtil;

// ---------------------------------------------------------
//
// Login Dialog
//
// ---------------------------------------------------------

LoginDlg* LoginDlg::_Dialog = NULL;

LoginDlg::LoginDlg(bool is_settings)
     : BaseGabberDialog("Login_dlg"),
       _is_settings(is_settings)
{
     main_dialog(_thisDialog);

     // Connect buttons to handlers
     getButton("Login_Cancel_btn")->clicked.connect(slot(this, &LoginDlg::on_Cancel_clicked));
     getButton("Login_Help_btn")->clicked.connect(slot(this, &LoginDlg::on_Help_clicked));
     _btnLogin = getButton("Login_Login_btn");
     _btnLogin->clicked.connect(slot(this, &LoginDlg::on_Login_clicked));
     _btnOK = getButton("Login_OK_btn");
     _btnOK->clicked.connect(slot(this, &LoginDlg::on_OK_clicked));

     // * DELETE *
     _thisWindow->delete_event.connect(slot(this, &LoginDlg::on_window_delete));

     // Show Login/OK depending on situation
     if (_is_settings)
     {
	  _btnLogin->hide();
	  _btnOK->show();
	  getWidget<Gtk::VBox>("Login_Logo_vbox")->hide();
	  _thisWindow->set_title(_("Connection Settings - Gabber"));
	  _thisWindow->set_modal(false);
     }
     else
     {
	  _btnLogin->show();
	  _btnOK->hide();
	  getWidget<Gtk::VBox>("Login_Logo_vbox")->show();
	  _thisWindow->set_title(_("Log in - Gabber"));
	  _thisWindow->set_modal(true);
     }

     // Initialize the rest of the pointers
     _entUsername  = getEntry("Login_Username_txt");
     _entUsername  ->changed.connect(slot(this, &LoginDlg::changed));
     _entUsername  ->key_press_event.connect(slot(this, &LoginDlg::on_Username_key_press));
     _entServer    = getEntry("Login_Server_txt");
     _entServer    ->changed.connect(slot(this, &LoginDlg::changed));
     _spinPort     = getWidget<Gtk::SpinButton>("Login_Port_spin");
     _spinPort     ->changed.connect(slot(this, &LoginDlg::changed));
     _spinPriority = getWidget<Gtk::SpinButton>("Login_Priority_spin");
     _spinPriority ->changed.connect(slot(this, &LoginDlg::changed));
     _entPassword  = getEntry("Login_Password_txt");
     _entPassword  ->changed.connect(slot(this, &LoginDlg::changed));
     _entResource  = getEntry("Login_Resource_txt");
     _entResource  ->changed.connect(slot(this, &LoginDlg::changed));
     _chkAutologin = getCheckButton("Login_Autologin_chk");
     _chkAutoreconnect = getCheckButton("Login_Autoreconnect_chk");
     _chkAutoreconnect->set_sensitive(true);
     _chkSavePassword = getCheckButton("Login_SavePassword_chk");
     _chkPlaintext = getCheckButton("Login_Plaintext_chk");
     _chkSSL       = getCheckButton("Login_SSL_chk");
#ifdef WITH_SSL
     _chkSSL       ->show();
     _chkSSL       ->set_sensitive(true);
#endif
     _chkSSL       ->toggled.connect(slot(this, &LoginDlg::on_SSL_toggled));

     // Proxy selection
     getCheckButton("Login_NoProxy_rdo")->toggled.connect(slot(this, &LoginDlg::proxy_changed));
     getCheckButton("Login_SOCKS4_rdo")->toggled.connect(slot(this, &LoginDlg::proxy_changed));
     getCheckButton("Login_SOCKS5_rdo")->toggled.connect(slot(this, &LoginDlg::proxy_changed));
     getCheckButton("Login_HTTP_rdo")->toggled.connect(slot(this, &LoginDlg::proxy_changed));

     // Make "Enter" cycle through
     _entUsername  ->activate.connect(_entServer->grab_focus.slot());
     _entServer    ->activate.connect(_entPassword->grab_focus.slot());
     _entPassword  ->activate.connect(slot(this, &LoginDlg::on_Login_clicked));
     _entResource  ->activate.connect(slot(this, &LoginDlg::on_Login_clicked));

     // Load configuration
     loadconfig();

     // Set the focus
     if (_entUsername->get_text().empty())
	  _entUsername->grab_focus();
     else if (_entPassword->get_text().empty())
	  _entPassword->grab_focus();
     else if (_btnLogin->is_visible())
	  _btnLogin->grab_focus();
     else
	  _btnOK->grab_focus();

     // display
     show();
}

LoginDlg::~LoginDlg()
{
}    

void LoginDlg::execute()
{
     // Create a login dialog
     _Dialog = manage(new LoginDlg(false));
}

void LoginDlg::execute_as_settings()
{
     _Dialog = manage(new LoginDlg(true));
}

void LoginDlg::on_OK_clicked()
{
     // Save configuration
     saveconfig();
     
     // Close
     close();
}

void LoginDlg::on_Login_clicked()
{
     g_assert( _entUsername != NULL );
     g_assert( _entServer != NULL );
     g_assert( _entPassword != NULL );
     g_assert( _entResource != NULL );

     if (JID::isValidUser(toUTF8(_entUsername, _entUsername->get_text())) && 
	 JID::isValidHost(toUTF8(_entServer, _entServer->get_text())) &&
	 !_entPassword->get_text().empty() &&
	 !_entResource->get_text().empty())
     {
	  // Save configuration
	  saveconfig();

	  // Hide the dialog now that we're trying to log in
	  hide();
	  
	  // Start G_Win
	  G_App->init_win();

	  // FIXME: Attempt to sign something here so it loops to get the correct passphrase.  It
	  // would be better to do this when the passphrase is first needed but there is some wierd problem
	  // with Gnome::Dialog->run() and receiving the roster at the same time causing the roster to be ignored.
	  GabberGPG& gpg = G_App->getGPG();
	  string dest;
	  // Enable gpg so it will prompt for the correct passphrase if the user
	  // pressed cancel before
	  gpg.enable();
	  if (gpg.enabled() && (gpg.sign(GPGInterface::sigClear, string(""), dest) == GPGInterface::errPass))
	  {
	       // If we couldn't get the right passphrase, disable gpg
	       gpg.disable();
	  }

	  // Start login process
	  G_App->login();
	  
	  // And now close it all up, we're done
	  close();
     }
}

void LoginDlg::on_Cancel_clicked()
{
     if (!_is_settings)
	  // Start G_Win
	  G_App->init_win();

     close();
}

gint LoginDlg::on_window_delete(GdkEventAny* e)
{
     on_Cancel_clicked();
     return 0;
}

void LoginDlg::on_Help_clicked()
{
     // Do help stuff 
     GnomeHelpMenuEntry help_entry = { "gabber", "login.html" };
     gnome_help_display (NULL, &help_entry);
}

void LoginDlg::loadconfig()
{     
     ConfigManager& c = G_App->getCfg();

     _entUsername     ->set_text(fromUTF8(_entUsername, c.server.username));
     _entServer       ->set_text(fromUTF8(_entServer, c.get_server()));  
     _chkSSL          ->set_active(c.server.ssl);
     _spinPort        ->set_value(c.server.port);
     _spinPriority    ->set_value(c.status.priority);
     _entResource     ->set_text(fromUTF8(_entResource, c.server.resource));
     _chkSavePassword ->set_active(c.server.savepassword);
     if (_chkSavePassword->get_active())
	  _entPassword->set_text(fromUTF8(_entPassword, c.server.password));
     else
	  _entPassword->set_text("");
     _chkPlaintext    ->set_active(c.server.plaintext);
     _chkAutologin    ->set_active(c.server.autologin);
     _chkAutoreconnect->set_active(c.server.autoreconnect);
     set_proxy_selection(indexProxy(c.proxy.type));
     Gtk::Entry* e = getEntry("Login_ProxyServer_ent");
     e->set_text(fromUTF8(e, c.proxy.server));
     getWidget<Gtk::SpinButton>("Login_ProxyPort_spin")->set_value(c.proxy.port);
     e = getEntry("Login_ProxyUsername_ent");
     e->set_text(fromUTF8(e, c.proxy.username));
     e = getEntry("Login_ProxyPassword_ent");
     e->set_text(fromUTF8(e, c.proxy.password));

     // Grab the history and put it in the gnome entries (a cooler combo box)
     G_App->getCfg().loadEntryHistory(getGEntry("Login_Resource_gent"));
     G_App->getCfg().loadEntryHistory(getGEntry("Login_ProxyServer_gent"));
}


void LoginDlg::saveconfig()
{
     ConfigManager& c = G_App->getCfg();
     c.server.username = toUTF8(_entUsername, _entUsername->get_text());
     c.set_server(toUTF8(_entServer, _entServer->get_text()));
     c.server.port = _spinPort->get_value_as_int();
     c.status.priority = _spinPriority->get_value_as_int();
     c.server.resource = toUTF8(_entResource, _entResource->get_text());
     c.server.savepassword =_chkSavePassword->get_active();
     // If they want to save the password, write it, it will not be saved by ConfigManager
     c.server.password = toUTF8(_entPassword, _entPassword->get_text());
     c.server.plaintext = _chkPlaintext->get_active();
     c.server.ssl = _chkSSL->get_active();
     c.server.autologin = _chkAutologin->get_active();
     c.server.autoreconnect = _chkAutoreconnect->get_active();
     c.proxy.type = indexProxy(get_proxy_selection());
     Gtk::Entry* e = getEntry("Login_ProxyServer_ent");
     c.proxy.server = toUTF8(e, e->get_text());
     c.proxy.port = getWidget<Gtk::SpinButton>("Login_ProxyPort_spin")->get_value_as_int();
     e = getEntry("Login_ProxyUsername_ent");
     c.proxy.username = toUTF8(e, e->get_text());
     e = getEntry("Login_ProxyPassword_ent");
     c.proxy.password = toUTF8(e, e->get_text());

     // Grab the history and put it in the gnome entries (a cooler combo box)
     G_App->getCfg().saveEntryHistory(getGEntry("Login_Resource_gent"));
     G_App->getCfg().saveEntryHistory(getGEntry("Login_ProxyServer_gent"));

     c.sync();
}

void LoginDlg::changed()
{
     if (JID::isValidUser(toUTF8(_entUsername, _entUsername->get_text())) && 
	 JID::isValidHost(toUTF8(_entServer, _entServer->get_text())) &&
	 !_entPassword->get_text().empty() &&
	 !_entResource->get_text().empty())
	  _btnLogin->set_sensitive(true);
     else
	  _btnLogin->set_sensitive(false);
}

gint LoginDlg::on_Username_key_press(GdkEventKey* e)
{
     if (e->keyval == GDK_at)
     {
	  // HACK --  to stop the signal from continuing, and the @ from being put into the Username field
	  gtk_signal_emit_stop_by_name(GTK_OBJECT(_entUsername->gtkobj()), "key_press_event");
	  _entServer->grab_focus();
     }
     return 0;
}

void LoginDlg::on_SSL_toggled()
{
     // If they want to use SSL, set the default port to 5223
     if (_chkSSL->get_active())
     {
	  _spinPort->set_value(5223);
     }
     // otherwise set the default port to 5222
     else
     {
	  _spinPort->set_value(5222);
     }
}

ConfigManager::Proxy LoginDlg::get_proxy_selection()
{
     if (getCheckButton("Login_NoProxy_rdo")->get_active())
	  return ConfigManager::proxyNone;
     else if (getCheckButton("Login_SOCKS4_rdo")->get_active())
	  return ConfigManager::proxySOCKS4;
     else if (getCheckButton("Login_SOCKS5_rdo")->get_active())
	  return ConfigManager::proxySOCKS5;
     else if (getCheckButton("Login_HTTP_rdo")->get_active())
	  return ConfigManager::proxyHTTP;
     return ConfigManager::proxyNone;
}

void LoginDlg::set_proxy_selection(ConfigManager::Proxy proxy_selection)
{
     switch (proxy_selection)
     {
     case ConfigManager::proxyNone:
	  getCheckButton("Login_NoProxy_rdo")->set_active(true);
	  break;
     case ConfigManager::proxySOCKS4:
	  getCheckButton("Login_SOCKS4_rdo")->set_active(true);
	  break;
     case ConfigManager::proxySOCKS5:
	  getCheckButton("Login_SOCKS5_rdo")->set_active(true);
	  break;
     case ConfigManager::proxyHTTP:
	  getCheckButton("Login_HTTP_rdo")->set_active(true);
	  break;
     default:
	  getCheckButton("Login_NoProxy_rdo")->set_active(true);
     }
}

void LoginDlg::proxy_changed()
{
     switch (get_proxy_selection())
     {
     case ConfigManager::proxySOCKS5:
     case ConfigManager::proxyHTTP:
	  getFrame("Login_ProxyServer_frame")->set_sensitive(true);
	  getFrame("Login_ProxyAuth_frame")->set_sensitive(true);
	  break;
     case ConfigManager::proxySOCKS4:
	  getFrame("Login_ProxyServer_frame")->set_sensitive(true);
	  getFrame("Login_ProxyAuth_frame")->set_sensitive(false);
	  break;
     case ConfigManager::proxyNone:
     default:
	  getFrame("Login_ProxyServer_frame")->set_sensitive(false);
	  getFrame("Login_ProxyAuth_frame")->set_sensitive(false);
     }
}

// ---------------------------------------------------------
//
// Preferences Window
//
// ---------------------------------------------------------

PrefsWin* PrefsWin::_Dialog = NULL;

// Utility function for loading a color (given a config location) and
// loading it into a gnome color-picker
void loadColor(const string& cfgid, Gnome::ColorPicker& p, map<string, Gdk_Color> & cm)
{
     map<string, Gdk_Color>::iterator it = cm.find(cfgid);
     p.set((gushort)it->second.get_red(), (gushort)it->second.get_green(), 
	   (gushort)it->second.get_blue(), 65535);
}

void saveColor(const string& cfgid, Gnome::ColorPicker& p, map<string, Gdk_Color> & cm)
{
     map<string, Gdk_Color>::iterator it = cm.find(cfgid);
     if (it != cm.end())
	  cm.erase(it);

     guint16 r, g, b, a;
     p.get(r, g, b, a);

     Gdk_Color color;
     color.set_rgb(r, g, b);
     cm.insert(make_pair(cfgid, color));
}

void PrefsWin::execute()
{
     if (_Dialog == NULL)
          _Dialog = manage(new PrefsWin());
}

PrefsWin::~PrefsWin()
{
     _Dialog = NULL;
}

PrefsWin::PrefsWin()
     : BaseGabberDialog("Prefs_win")
{
     main_dialog(_thisDialog);

     // Connect the widgets
     getButton("Prefs_OK_btn")->clicked.connect(slot(this, &PrefsWin::on_ok_clicked));
     getButton("Prefs_Help_btn")->clicked.connect(slot(this, &PrefsWin::on_help_clicked));
     getButton("Prefs_Cancel_btn")->clicked.connect(slot(this, &PrefsWin::on_cancel_clicked));

     _thisWindow->delete_event.connect(slot(this, &PrefsWin::on_window_delete));

     // Log widgets
     _chkLogs             = getCheckButton("Prefs_Logs_SaveLogs_chk");
     _chkLogs             ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkLogsEncrypted    = getCheckButton("Prefs_Logs_LogEncrypted_chk");
     _chkLogsEncrypted    ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkLogsGroupchat    = getCheckButton("Prefs_Logs_LogGroupChats_chk");
     _chkLogsGroupchat    ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkLogsSave         = getCheckButton("Prefs_Logs_Type_movemonthly_rdo");
     _chkLogsSave         ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkLogsPurge        = getCheckButton("Prefs_Logs_Type_purgemonthly_rdo");
     _chkLogsPurge        ->toggled.connect(slot(this, &PrefsWin::changed));
     //_fileLogsDir         = getWidget<Gnome::FileEntry>("Prefs_Logs_location_file");
     _entLogsDir          = getEntry("Prefs_Logs_location_txt");
     _entLogsDir          ->changed.connect(slot(this, &PrefsWin::changed));
     _chkLogsHTML         = getCheckButton("Prefs_Logs_Frmt_html_rdo");
     _chkLogsHTML         ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkLogsXML          = getCheckButton("Prefs_Logs_Frmt_xml_rdo");
     _chkLogsXML          ->toggled.connect(slot(this, &PrefsWin::changed));

     // Messages options
     _chkMsgsSendMsgs     = getCheckButton("Prefs_Msgs_SendMsgs_rdo");
     _chkMsgsSendMsgs     ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkMsgsSendOOOChats = getCheckButton("Prefs_Msgs_SendOOOChats_rdo");
     _chkMsgsSendOOOChats ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkMsgsRecvNon      = getCheckButton("Prefs_Msgs_RecvNon_rdo");
     _chkMsgsRecvNon      ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkMsgsRecvMsgs     = getCheckButton("Prefs_Msgs_RecvMsgs_rdo");
     _chkMsgsRecvMsgs     ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkMsgsRecvOOOChats = getCheckButton("Prefs_Msgs_RecvOOOChats_rdo");
     _chkMsgsRecvOOOChats ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkMsgsOpenMsgs     = getCheckButton("Prefs_Msgs_OpenMsgs_chk");
     _chkMsgsOpenMsgs     ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkMsgsRaise        = getCheckButton("Prefs_Msgs_Raise_chk");
     _chkMsgsRaise        ->toggled.connect(slot(this, &PrefsWin::changed));

     // Colors
     _colorAvailable   = getWidget<Gnome::ColorPicker>("Prefs_Roster_available_color");
     _colorAvailable   ->color_set.connect(slot(this, &PrefsWin::colorchanged));
     _colorChat        = getWidget<Gnome::ColorPicker>("Prefs_Roster_chat_color");
     _colorChat        ->color_set.connect(slot(this, &PrefsWin::colorchanged));
     _colorAway        = getWidget<Gnome::ColorPicker>("Prefs_Roster_away_color");
     _colorAway        ->color_set.connect(slot(this, &PrefsWin::colorchanged));
     _colorXa          = getWidget<Gnome::ColorPicker>("Prefs_Roster_xa_color");
     _colorXa          ->color_set.connect(slot(this, &PrefsWin::colorchanged));
     _colorDnd         = getWidget<Gnome::ColorPicker>("Prefs_Roster_dnd_color");
     _colorDnd         ->color_set.connect(slot(this, &PrefsWin::colorchanged));
     _colorUnavailable = getWidget<Gnome::ColorPicker>("Prefs_Roster_offline_color");
     _colorUnavailable ->color_set.connect(slot(this, &PrefsWin::colorchanged));
     _colorNotInList   = getWidget<Gnome::ColorPicker>("Prefs_Roster_NotInList_color");
     _colorNotInList   ->color_set.connect(slot(this, &PrefsWin::colorchanged));
     _colorStalker     = getWidget<Gnome::ColorPicker>("Prefs_Roster_Stalker_color");
     _colorStalker     ->color_set.connect(slot(this, &PrefsWin::colorchanged));
     _colorMessage     = getWidget<Gnome::ColorPicker>("Prefs_Roster_Message_color");
     _colorMessage     ->color_set.connect(slot(this, &PrefsWin::colorchanged));
     _colorOOOChat     = getWidget<Gnome::ColorPicker>("Prefs_Roster_OOOChat_color");
     _colorOOOChat     ->color_set.connect(slot(this, &PrefsWin::colorchanged));

     // Icons
     _chkIcons       = getCheckButton("Prefs_Roster_Icons_chk");
     _chkIcons       ->toggled.connect(slot(this, &PrefsWin::changed));

     // Size and Pos
     _chkSize        = getCheckButton("Prefs_Window_SaveSize_chk");
     _chkSize        ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkPos         = getCheckButton("Prefs_Window_SavePos_chk");
     _chkPos         ->toggled.connect(slot(this, &PrefsWin::changed));

     // Toolbar options
     _chkMenubarShow = getCheckButton("Prefs_Menubar_Show_chk");
     _chkMenubarShow ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkToolbarShow = getCheckButton("Prefs_Toolbar_Show_chk");
     _chkToolbarShow ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkStatusShow   = getCheckButton("Prefs_Status_Show_chk");
     _chkStatusShow   ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkPresenceShow = getCheckButton("Prefs_Presence_Show_chk");
     _chkPresenceShow ->toggled.connect(slot(this, &PrefsWin::changed));

     // Spelling
     _chkSpellCheck  = getCheckButton("Prefs_Spell_Check_chk");
     _chkSpellCheck  ->toggled.connect(slot(this, &PrefsWin::changed));

     // Status Docklet
     _chkDockletShow = getCheckButton("Prefs_Docklet_Show_chk");
     _chkDockletShow ->toggled.connect(slot(this, &PrefsWin::changed));

     // Chats
     _chkChatsOOOTime = getCheckButton("Prefs_Chats_Time_ooochat_chk");
     _chkChatsOOOTime ->toggled.connect(slot(this, &PrefsWin::changed));
     _chkChatsGroupTime = getCheckButton("Prefs_Chats_Time_groupchat_chk");
     _chkChatsGroupTime->toggled.connect(slot(this, &PrefsWin::changed));

     // Encryption
     _chkGPGEnable = getCheckButton("Prefs_GPG_Enable_chk");
     _chkGPGEnable ->toggled.connect(slot(this, &PrefsWin::changed));
     _entGPGKeyserver = getEntry("Prefs_GPG_Keyserver_ent");
//     _entGPGKeyserver ->changed.connect(slot(this, &PrefsWin::changed));
     _clistGPGKey  = getWidget<Gtk::CList>("Prefs_GPG_Key_clist");
     _clistGPGKey  ->select_row.connect(slot(this, &PrefsWin::on_GPGKey_select_row));
     // whether the secret keys have been retrieved from gpg and added to the clist yet
     _gotKeys = false;

     // Auto-away
     // Auto-away
     _chkAutoAway   = getCheckButton("Prefs_Away_AutoChange_chk");
     _chkAutoAway   ->toggled.connect(slot(this, &PrefsWin::changed));
     _spinAwayAfter = getWidget<Gtk::SpinButton>("Prefs_Away_AwayAfter_spin");
     _spinAwayAfter ->changed.connect(slot(this, &PrefsWin::changed));
     _spinXAAfter   = getWidget<Gtk::SpinButton>("Prefs_Away_XAAfter_spin");
     _spinXAAfter   ->changed.connect(slot(this, &PrefsWin::changed));
     _txtStatus     = getWidget<Gtk::Text>("Prefs_Away_Status_txt");
     _txtStatus     ->changed.connect(slot(this, &PrefsWin::changed));
     _txtStatus     ->set_word_wrap(true);
     _txtStatus     ->set_editable(true);
     _chkChangePriority = getCheckButton("Prefs_Away_ChangePriority_chk");
     _chkChangePriority ->toggled.connect(slot(this, &PrefsWin::changed));

     // Load configuration and display
     loadconfig();
     show();
}

void PrefsWin::loadconfig()
{
     // Load the current configuration
     ConfigManager& c = G_App->getCfg();

     // Logs Options
     _chkLogs                ->set_active(!c.logs.none);
     _chkLogsEncrypted       ->set_active(c.logs.encrypted);
     _chkLogsGroupchat       ->set_active(c.logs.groupchat);
     _chkLogsSave            ->set_active(c.logs.save);
     _chkLogsPurge           ->set_active(c.logs.purge);
     _entLogsDir             ->set_text(c.logs.dir);
     _chkLogsHTML            ->set_active(c.logs.html);
     _chkLogsXML             ->set_active(c.logs.xml);

     // Message Options
     _chkMsgsSendMsgs      ->set_active(c.msgs.sendmsgs);
     _chkMsgsSendOOOChats  ->set_active(c.msgs.sendooochats);
     _chkMsgsRecvNon       ->set_active(c.msgs.recvnon);
     _chkMsgsRecvMsgs      ->set_active(c.msgs.recvmsgs);
     _chkMsgsRecvOOOChats  ->set_active(c.msgs.recvooochats);
     _chkMsgsOpenMsgs      ->set_active(c.msgs.openmsgs);
     _chkMsgsRaise         ->set_active(c.msgs.raise);

     // Color Options -- HACK -- Color defaults should be loaded from 
     // a central piece of code
     loadColor("available", *_colorAvailable, c.colors.presence_colors);
     loadColor("chat", *_colorChat, c.colors.presence_colors);
     loadColor("away", *_colorAway, c.colors.presence_colors);
     loadColor("xa", *_colorXa, c.colors.presence_colors);
     loadColor("dnd", *_colorDnd, c.colors.presence_colors);
     loadColor("unavailable", *_colorUnavailable, c.colors.presence_colors);
     loadColor("NotInList", *_colorNotInList, c.colors.presence_colors);
     loadColor("Stalker", *_colorStalker, c.colors.presence_colors);
     loadColor("Message", *_colorMessage, c.colors.presence_colors);
     loadColor("OOOChat", *_colorOOOChat, c.colors.presence_colors);

     // Icons
     _chkIcons->set_active(c.colors.icons);

     // Size and Pos
     _chkSize->set_active(c.window.savesize);
     _chkPos->set_active(c.window.savepos);

     // Toolbar options
     _chkMenubarShow->set_active(c.toolbars.menubar);
     _chkToolbarShow->set_active(c.toolbars.toolbar);
     _chkStatusShow->set_active(c.toolbars.status);
     _chkPresenceShow->set_active(c.toolbars.presence);

     // Spelling
     _chkSpellCheck->set_active(c.spelling.check);

     // Docklet options
     _chkDockletShow->set_active(c.docklet.show);

     // Chats
     _chkChatsOOOTime->set_active(c.chats.oootime);
     _chkChatsGroupTime->set_active(c.chats.grouptime);

     // Encryption
     GabberGPG& gpg = G_App->getGPG();
     // Changed this to use the config file instead of gpg.enabled() since gpg could be disabled while still
     // enabled in the config file
     bool value = c.gpg.enabled;
     _chkGPGEnable->set_active(value);
     _entGPGKeyserver->set_text(fromUTF8(c.gpg.keyserver));
     G_App->getCfg().loadEntryHistory(getGEntry("Prefs_GPG_Keyserver_gent"));
     if (gpg.find_gpg() && !_gotKeys)
     {
	  for (unsigned int i = 0; i < _clistGPGKey->columns().size(); i++)
	  {
	       _clistGPGKey->set_column_resizeable(i, false);
	       _clistGPGKey->set_column_auto_resize(i, true);
	  }
	  _clistGPGKey->freeze();
	  // Add the Secret Keys to the CList
          list<GPGInterface::KeyInfo> keys;
          if (gpg.get_secret_keys(keys) == GPGInterface::errOK)
	  {
               list<GPGInterface::KeyInfo>::iterator it = keys.begin();
               string key = c.gpg.secretkeyid;
               for ( ; it != keys.end(); it++)
               {
                    const char* rowdata[2] = { fromUTF8(it->get_userid()).c_str(), it->get_keyid().c_str() };
	            gint row = _clistGPGKey->append_row(rowdata);
	            if (it->get_keyid() == key)
	                 _clistGPGKey->row(row).select();
               }
	       _gotKeys = true;
	  }
	  for (unsigned int i = 0; i < _clistGPGKey->columns().size(); i++)
	       _clistGPGKey->set_column_resizeable(i, true);
	  _clistGPGKey->thaw();
     }
     else if (!gpg.find_gpg())
     {
	  getWidget<Gtk::Frame>("Prefs_GPG_Key_frm")->set_sensitive(false);
	  getWidget<Gtk::Table>("Prefs_GPG_extra_tbl")->set_sensitive(false);
     }

     // Auto-away
     _chkAutoAway  ->set_active(c.autoaway.enabled);
     _spinAwayAfter->set_value(c.autoaway.awayafter);
     _spinXAAfter  ->set_value(c.autoaway.xaafter);
     string awymsg = c.autoaway.status;
     int i = 0;
     if (awymsg.length() > 0)
	  _txtStatus->insert(fromUTF8(_txtStatus, awymsg));
     else
	  _txtStatus->insert(_("Automatically away due to being idle"));
     _chkChangePriority->set_active(c.autoaway.changepriority);
}

void PrefsWin::saveconfig()
{
     g_assert(_clistGPGKey != NULL);

     // Save the config
     ConfigManager& c = G_App->getCfg();
     
     // Logs config
     c.logs.none = !_chkLogs->get_active();
     c.logs.encrypted = _chkLogsEncrypted->get_active();
     c.logs.groupchat = _chkLogsGroupchat->get_active();
     c.logs.save = _chkLogsSave->get_active();
     c.logs.purge = _chkLogsPurge->get_active();
     c.logs.dir = _entLogsDir->get_text();
     c.logs.html = _chkLogsHTML->get_active();
     c.logs.xml = _chkLogsXML->get_active();

     // Messages config
     c.msgs.sendmsgs = _chkMsgsSendMsgs->get_active();
     c.msgs.sendooochats = _chkMsgsSendOOOChats->get_active();
     c.msgs.recvnon = _chkMsgsRecvNon->get_active();
     c.msgs.recvmsgs = _chkMsgsRecvMsgs->get_active();
     c.msgs.recvooochats = _chkMsgsRecvOOOChats->get_active();
     c.msgs.openmsgs = _chkMsgsOpenMsgs->get_active();
     c.msgs.raise = _chkMsgsRaise->get_active();

     // Color config
     saveColor("available", *_colorAvailable, c.colors.presence_colors);
     saveColor("chat", *_colorChat, c.colors.presence_colors);
     saveColor("away", *_colorAway, c.colors.presence_colors);
     saveColor("xa", *_colorXa, c.colors.presence_colors);
     saveColor("dnd", *_colorDnd, c.colors.presence_colors);
     saveColor("unavailable", *_colorUnavailable, c.colors.presence_colors);
     saveColor("NotInList", *_colorNotInList, c.colors.presence_colors);
     saveColor("Stalker", *_colorStalker, c.colors.presence_colors);
     saveColor("Message", *_colorMessage, c.colors.presence_colors);
     saveColor("OOOChat", *_colorOOOChat, c.colors.presence_colors);

     // Icons
     c.colors.icons = _chkIcons->get_active();

     // Size and Pos
     c.window.savesize = _chkSize->get_active();
     c.window.savepos = _chkPos->get_active();
  
     // Toolbar config
     c.toolbars.menubar = _chkMenubarShow->get_active();
     c.toolbars.toolbar = _chkToolbarShow->get_active();
     c.toolbars.status = _chkStatusShow->get_active();
     c.toolbars.presence = _chkPresenceShow->get_active();

     c.spelling.check = _chkSpellCheck->get_active();

     // Docklet
     c.docklet.show = _chkDockletShow->get_active();

     // Chats
     c.chats.oootime = _chkChatsOOOTime->get_active();
     c.chats.grouptime = _chkChatsGroupTime->get_active();

     // Encryption
     bool gpg_changed = false; // Whether or not the GPG config has changed
     if (c.gpg.enabled != _chkGPGEnable->get_active())
	  gpg_changed = true;
     c.gpg.enabled = _chkGPGEnable->get_active();
     c.gpg.keyserver = toUTF8(_entGPGKeyserver->get_text());
     G_App->getCfg().saveEntryHistory(getGEntry("Prefs_GPG_Keyserver_gent"));
     // If the clist has a selection, store the keyid
     if (_clistGPGKey->selection().begin() != _clistGPGKey->selection().end())
     {
          Gtk::CList::Row& r = *_clistGPGKey->selection().begin();
	  string seckeyid = r[1].get_text();
	  if (c.gpg.secretkeyid != seckeyid)
	       gpg_changed = true;
	  // keyid is second column
          c.gpg.secretkeyid = seckeyid;
     }
     // Export the public key for the selected key to the keyserver to ensure it is there
     // so it can be retrieved by other jabber users
     // Only export if GPG is enabled
     // and only do this if the GPG config changed
     if (gpg_changed)
     {
	  if (_chkGPGEnable->get_active())
	  {
	       // Make sure gpg is enabled
	       G_App->getGPG().enable();
	       cerr << "Trying to export key" << endl;
	       G_App->getGPG().send_key(c.gpg.secretkeyid);
	  }
	  else
	  {
	       // If we don't want gpg, turn it off
	       G_App->getGPG().disable();
	  }
     }

     // Auto-away
     c.autoaway.enabled = _chkAutoAway->get_active();
     c.autoaway.awayafter = _spinAwayAfter->get_value_as_int();
     c.autoaway.xaafter = _spinXAAfter->get_value_as_int();
     c.autoaway.status = toUTF8(_txtStatus, _txtStatus->get_chars(0, -1));
     c.autoaway.changepriority = _chkChangePriority->get_active();

     // Sync it all
     c.sync();
}

void PrefsWin::changed()
{
     // If they want to save logs, enable the logging options
     if (_chkLogs->get_active())
     {
	  _chkLogsEncrypted->set_sensitive(true);
	  getWidget<Gtk::Frame>("Prefs_Logs_Purging_frame")->set_sensitive(true);
	  getWidget<Gtk::Frame>("Prefs_Logs_Location_frame")->set_sensitive(true);
	  getWidget<Gtk::Frame>("Prefs_Logs_Format_frame")->set_sensitive(true);
     }
     else
     {
	  _chkLogsEncrypted->set_sensitive(false);
	  getWidget<Gtk::Frame>("Prefs_Logs_Purging_frame")->set_sensitive(false);
	  getWidget<Gtk::Frame>("Prefs_Logs_Location_frame")->set_sensitive(false);
	  getWidget<Gtk::Frame>("Prefs_Logs_Format_frame")->set_sensitive(false);
     }

     GabberGPG& gpg = G_App->getGPG();
     // If the user enabled gpg AND we can find gpg then add the secret keys to the CList and enable the Frame
     if (_chkGPGEnable->get_active() && gpg.find_gpg())
     {
	  for (unsigned int i = 0; i < _clistGPGKey->columns().size(); i++)
	  {
	       _clistGPGKey->set_column_resizeable(i, false);
	       _clistGPGKey->set_column_auto_resize(i, true);
	  }
	  _clistGPGKey->freeze();
          list<GPGInterface::KeyInfo> keys;
          if (gpg.get_secret_keys(keys) == GPGInterface::errOK && !_gotKeys)
	  {
               list<GPGInterface::KeyInfo>::iterator it = keys.begin();
               string key = G_App->getCfg().gpg.secretkeyid;
               for ( ; it != keys.end(); it++)
               {
                    const char* rowdata[2] = { fromUTF8(it->get_userid()).c_str(), it->get_keyid().c_str() };
                    gint row = _clistGPGKey->append_row(rowdata);
                    if (it->get_keyid() == key)
			 _clistGPGKey->row(row).select();
               }
	       _gotKeys = true;
	  }
	  for (unsigned int i = 0; i < _clistGPGKey->columns().size(); i++)
	       _clistGPGKey->set_column_resizeable(i, true);
	  _clistGPGKey->thaw();
	  getWidget<Gtk::Frame>("Prefs_GPG_Key_frm")->set_sensitive(true);
	  getWidget<Gtk::Table>("Prefs_GPG_extra_tbl")->set_sensitive(true);

     }
     // Otherwise diable the GPG frame and disbale the GPG check
     else
     {
	  getWidget<Gtk::Frame>("Prefs_GPG_Key_frm")->set_sensitive(false);
	  getWidget<Gtk::Table>("Prefs_GPG_extra_tbl")->set_sensitive(false);
	  _chkGPGEnable->set_active(false);
     }

     // Auto-away
     if (_chkAutoAway->get_active())
     {
	  getWidget<Gtk::Table>("Prefs_Away_After_tbl")->set_sensitive(true);
	  getWidget<Gtk::Frame>("Prefs_Away_Status_frame")->set_sensitive(true);
     }
     else
     {
	  getWidget<Gtk::Table>("Prefs_Away_After_tbl")->set_sensitive(false);
	  getWidget<Gtk::Frame>("Prefs_Away_Status_frame")->set_sensitive(false);
     }

     // <INSTANT-APPLY>
     // Instant-apply a few things

     // Display/hide toolbars based on settings chosen, but don't save those settings
     G_Win->display_toolbars(_chkMenubarShow->get_active(), _chkToolbarShow->get_active(), _chkStatusShow->get_active(), _chkPresenceShow->get_active());

     // Save the current setting
     bool dockletshow = G_App->getCfg().docklet.show;
     bool colorsicons = G_App->getCfg().colors.icons;

     // Temporarily set new setting
     G_App->getCfg().docklet.show = _chkDockletShow->get_active();
     G_App->getCfg().colors.icons = _chkIcons->get_active();

     // Refresh the roster to reflect this
     G_Win->refresh_roster();

     // Update status to initiate status dock update
     G_Win->display_status(G_App->getCfg().get_show(),
			   G_App->getCfg().get_status(),
			   G_App->getCfg().get_priority(),
			   false);

     // Set back to the old setting
     G_App->getCfg().docklet.show = dockletshow;
     G_App->getCfg().colors.icons = colorsicons;

     // </INSTANT-APPLY>
}

void PrefsWin::colorchanged(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4)
{
     // Not that we have any color prefs currently :)
}

void PrefsWin::on_ok_clicked()
{
     if (!_entLogsDir->get_text().empty())
     {
	  // Apply button has been clicked, save changes
	  saveconfig();
	  // Display/hide toolbar
	  G_Win->display_toolbars(G_App->getCfg().toolbars.menubar, G_App->getCfg().toolbars.toolbar, G_App->getCfg().toolbars.status, G_App->getCfg().toolbars.presence);
	  // Update the roster completely (reloads all icons and colors)
	  G_Win->refresh_roster();
	  // Start/Stop spell-checking
	  if (_chkSpellCheck->get_active())
	       G_App->init_spellcheck();
	  else
	       G_App->stop_spellcheck();

	  // Update status to initiate status dock update
	  G_Win->display_status(G_App->getCfg().get_show(),
				G_App->getCfg().get_status(),
				G_App->getCfg().get_priority(),
				false);

	  // Set the logging parameters
	  G_App->setLogHTML(_chkLogsHTML->get_active());
	  G_App->setLogDir(_entLogsDir->get_text());
	  G_App->setLogging(_chkLogs->get_active());
     }

     close();
}

void PrefsWin::on_help_clicked()
{
     // Do help stuff
     GnomeHelpMenuEntry help_entry = { "gabber", "pref.html" };
     gnome_help_display(NULL, &help_entry);
}

void PrefsWin::on_cancel_clicked()
{
     // <INSTANT-UNAPPLY>
     // Set the stuff which may have been instant-applied

     // Display/hide toolbar
     G_Win->display_toolbars(G_App->getCfg().toolbars.menubar, G_App->getCfg().toolbars.toolbar, G_App->getCfg().toolbars.status, G_App->getCfg().toolbars.presence);

     // Update status to initiate status dock update
     G_Win->display_status(G_App->getCfg().get_show(),
			   G_App->getCfg().get_status(),
			   G_App->getCfg().get_priority(),
			   false);

     // Refresh the roster to update settings
     G_Win->refresh_roster();

     // </INSTANT-UNAPPLY>

     close();
}

gint PrefsWin::on_window_delete(GdkEventAny* e)
{
     on_cancel_clicked();
     return 0;
}

void PrefsWin::on_GPGKey_select_row(gint row, gint col, GdkEvent* e)
{
     // mark the preferences box as changed when the users changes what key is selected
     // in the CList
}
