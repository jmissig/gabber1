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
 *  Copyright (C) 1999-2003 Dave Smith & Julian Missig
 */

#include "GabberConfig.hh" // Needed for the ifdef.. hrmm

#include "GabberWin.hh"

// General headers
#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "RosterView.hh"

// Dialogs
#include "AddContactDruid.hh"
#include "AgentInterface.hh"
#include "ContactInfoInterface.hh"
#include "DebugInterface.hh"
#include "FilterInterface.hh"
#include "MessageManager.hh"
#include "MessageViews.hh"
#include "IgnoreInterface.hh"
#include "PrefsInterface.hh"
#include "RosterInterface.hh"
#include "StatusInterface.hh"
#include "WelcomeDruid.hh"

// GPG
#include "GabberGPG.hh"

#include <vector>
#include <libgnome/gnome-help.h>
#include <libgnome/gnome-triggers.h>
#include <gtk--/menubar.h>
#include <gnome--/about.h>
#include <gnome--/dock-item.h>

#ifdef HAVE_XMMS
#include <xmms/xmmsctrl.h>
#endif // HAVE_XMMS

using namespace jabberoo;
using namespace GabberUtil;

GabberWin* G_Win = NULL;

GabberWin::GabberWin()
     : BaseGabberWindow("Gabber_win"),
       connected(false),
       _pix_path(ConfigManager::get_PIXPATH())
{
     // Initialize G_Win
     G_Win = this;

     _thisWindow->realize();

     MainWin = getWidget<Gnome::App>("Gabber_win");

     // Setup roster
     _roster = new RosterView(getWidget<Gtk::CTree>("Gabber_RosterTree"),
			      getWidget<Gtk::ScrolledWindow>("Gabber_RosterScroll"),
			      G_App->getSession());

     // * DELETE *
     _thisWindow->delete_event.connect(slot(this, &GabberWin::on_window_delete));

     // MENUS
     init_menus();

     // StatusBar
     StatusBar = getWidget<Gnome::AppBar>("Gabber_statusbar");
//     MainWin->set_menus(*getWidget<Gtk::MenuBar>("Gabber_Menu_bar"));
//     MainWin->set_statusbar(*StatusBar);
     MainWin->install_menu_hints();
     MainWin->enable_layout_config(true);

     // Connect to session events
     G_App->getSession().evtConnected.connect(slot(this, &GabberWin::on_session_connected));
     G_App->getTransmitter().evtDisconnected.connect(slot(this, &GabberWin::on_transmitter_disconnected));

     // Grab dockitems
     _dockMenubar   = getWidget<Gnome::DockItem>("Gabber_Menu_dockitem");
     _dockMenubar   ->button_press_event.connect(bind(slot(this, &GabberWin::on_button_press), _dockMenubar));
     Gtk::MenuBar* mb = getWidget<Gtk::MenuBar>("Gabber_Menu_bar");
     mb->button_press_event.connect(bind(slot(this, &GabberWin::on_button_press), mb));
     _dockToolbar   = getWidget<Gnome::DockItem>("Gabber_Toolbar_dockitem");
     _dockToolbar   ->button_press_event.connect(bind(slot(this, &GabberWin::on_button_press), _dockToolbar));
     _dockStatus    = getWidget<Gnome::DockItem>("Gabber_Status_dockitem");
     _dockStatus    ->button_press_event.connect(bind(slot(this, &GabberWin::on_button_press), _dockStatus));
     _dockPresence  = getWidget<Gnome::DockItem>("Gabber_Presence_dockitem");
     _dockPresence  ->button_press_event.connect(bind(slot(this, &GabberWin::on_button_press), _dockPresence));
     mb = getWidget<Gtk::MenuBar>("Gabber_Presence_bar");
     mb->button_press_event.connect(bind(slot(this, &GabberWin::on_button_press), mb));
     Gtk::EventBox* eb = getEventBox("Gabber_Statusbar_evt");
     eb->button_press_event.connect(bind(slot(this, &GabberWin::on_button_press), eb));

     // Set the hide offline users toggle and run the check
     _m_HideOffline->set_active(G_App->getCfg().roster.hideoffline);
     _m_HideAgents->set_active(G_App->getCfg().roster.hideagents);
     on_HideOfflineUsers_toggled();
     // Do the same for show headlines
     _m_ShowHeadlines->set_active(G_App->getCfg().headlines.displayticker);
     on_ShowHeadlines_toggled();

     // Set window defaults
     if (G_App->getCfg().window.savesize)
	  _thisWindow->set_default_size(G_App->getCfg().window.width, G_App->getCfg().window.height);
     else
	  _thisWindow->set_default_size(185, 270);
     if (G_App->getCfg().window.savepos)
	  _thisWindow->set_uposition(G_App->getCfg().window.x, G_App->getCfg().window.y);
     _thisWindow->show();

     // Refresh the roster so that all the icons & colors get loaded
     refresh_roster();

     // Hide toolbar if people (DizzyD ;) hate it
     display_toolbars(G_App->getCfg().toolbars.menubar,
		      G_App->getCfg().toolbars.toolbar, 
		      G_App->getCfg().toolbars.status,
		      G_App->getCfg().toolbars.presence);
     on_ShowHeadlines_toggled();

     // Status Indicator(s)
     StatusIndicator::display_presence(Presence("", Presence::ptUnavailable, Presence::stOffline, toUTF8(_thisWindow, _("Disconnected"))));

     // Load spooled messages
     G_App->getMessageManager().load_spool();
}

void GabberWin::init_menus()
{
     // GABBER *
     // * SHOW AS *
     _m_Presence = getPixmapMenuItem("Gabber_Showas_menu");
     // * LOGIN *
     _m_Login = getMenuItem("Gabber_Login_item");
     _m_Login->activate.connect(slot(&LoginDlg::execute));
     // * LOGOUT *
     _m_Logout = getMenuItem("Gabber_Logout_item");
     _m_Logout->activate.connect(slot(this, &GabberWin::on_disconnect));
     _m_Logout->set_sensitive(false);
     // * LOGOUT REASON*
     _m_LogoutR = getMenuItem("Gabber_LogoutReason_item");
     _m_LogoutR->activate.connect(slot(this, &GabberWin::on_logout_reason_clicked));
     _m_LogoutR->set_sensitive(false);
     // * ROSTER *
     _m_Roster = getMenuItem("Gabber_Roster_menu");
     _m_Roster->set_sensitive(false);
     // ** EXPORT ROSTER *
     getMenuItem("Gabber_Export_item")->activate.connect(slot(this, &GabberWin::on_Export_activate));
     // ** IMPORT ROSTER *
     getMenuItem("Gabber_Import_item")->activate.connect(slot(this, &GabberWin::on_Import_activate));
     // * HIDE OFFLINE USERS *
     _m_HideOffline = getWidget<Gtk::CheckMenuItem>("Gabber_HideOffline_item");
     _m_HideOffline->toggled.connect(slot(this, &GabberWin::on_HideOfflineUsers_toggled));
     // * HIDE AGENTS *
     _m_HideAgents = getWidget<Gtk::CheckMenuItem>("Gabber_HideAgents_item");
     _m_HideAgents->toggled.connect(slot(this, &GabberWin::on_HideOfflineUsers_toggled));
     // * HIDE SPECIFIC GROUPS *
     //getMenuItem("Gabber_HideGroups_item")->set_sensitive(false);
     // * CONNECTION SETTINGS *
     getMenuItem("Gabber_ConnectionSettings_item")->activate.connect(slot(this, &GabberWin::on_ConnectionSettings_activate));
     // * USER INFO *
     _m_UserInfo = getMenuItem("Gabber_UserInfo_item");
     _m_UserInfo->activate.connect(slot(&MyContactInfoWin::execute));
     _m_UserInfo->set_sensitive(false);
     // * PREFS *
     getMenuItem("Gabber_Preferences_item")->activate.connect(slot(&PrefsWin::execute));
     // * EXIT *
     getMenuItem("Gabber_Exit_item")->activate.connect(slot(this, &GabberWin::on_Exit_activate));

     // SERVICES *
     _m_Services = getMenuItem("Gabber_Services_menu");
     _m_Services->set_sensitive(false);
     // * BLANK MESSAGE *
     getMenuItem("Gabber_BlankMessage_item")->activate.connect(slot(this, &GabberWin::on_BlankMessage_activate));
     // * ADD USER *
     getMenuItem("Gabber_AddUser_item")->activate.connect(slot(this, &GabberWin::on_AddUser_activate));
     // * ADD GROUP *
     getMenuItem("Gabber_AddGroup_item")->activate.connect(slot(this, &GabberWin::on_AddGroup_activate));
     // * TRANSPORTS *
     getMenuItem("Gabber_Agents_item")->activate.connect(slot(&AgentBrowserDlg::execute));
     // * GROUP CHAT *
     getMenuItem("Gabber_GroupChat_item")->activate.connect(slot(&GCJoinDlg::execute));
     // * FILTERS *
     getMenuItem("Gabber_Filters_item")->activate.connect(slot(&FilterListView::execute));
     // * SHOW HEADLINES *
     _m_ShowHeadlines = getWidget<Gtk::CheckMenuItem>("Gabber_ShowHeadlines_item");
     _m_ShowHeadlines->toggled.connect(slot(this, &GabberWin::on_ShowHeadlines_toggled));
     // * IGNORE LIST */
     getMenuItem("Gabber_IgnoreList_item")->activate.connect(slot(&IgnoreDlg::execute));

     // HELP *
     // * MANUAL *
     getMenuItem("Gabber_Manual_item")->activate.connect(slot(this, &GabberWin::on_Manual_activate));
     // * ABOUT *
     getMenuItem("Gabber_About_item")->activate.connect(slot(this, &GabberWin::on_About_activate));
     // * DEBUG *
     // ** RAW XML INPUT
     getMenuItem("Gabber_RawXML_item")->activate.connect(slot(&RawXMLInput::execute));

     _baseMU = manage(new BaseGabberWidget("ManageUser_menu", "Gabber_win"));
     // USER MENU*
     _menuUser = static_cast<Gtk::Menu*>(_baseMU->get_this_widget());
     _menuUser->accelerate(*_thisWindow);
     // * MESSAGE *
     _baseMU->getMenuItem("ManageUser_Message_item")->activate.connect(slot(_roster, &RosterView::on_Message_activate));
     // * OOOCHAT *
     _baseMU->getMenuItem("ManageUser_OOOChat_item")->activate.connect(slot(_roster, &RosterView::on_OOOChat_activate));
     // * SEND ROSTER ITEM *
     _baseMU->getMenuItem("ManageUser_SendRoster_item")->activate.connect(slot(_roster, &RosterView::on_SendRoster_activate));
     // * SEND FILE *
     _baseMU->getMenuItem("ManageUser_SendFile_item")->activate.connect(slot(_roster, &RosterView::on_SendFile_activate));
     // * RE-REQUEST S10N *
     _baseMU->getMenuItem("ManageUser_Rerequest_item")->activate.connect(slot(_roster, &RosterView::on_S10nRequest_activate));
     // * EDIT USER *
     _baseMU->getMenuItem("ManageUser_EditUser_item")->activate.connect(slot(_roster, &RosterView::on_EditUser_activate));
     // * EDIT GROUPS *
     _baseMU->getMenuItem("ManageUser_EditGroups_item")->activate.connect(slot(_roster, &RosterView::on_EditGroups_activate));
     // * USER HISTORY *
     _baseMU->getMenuItem("ManageUser_History_item")->activate.connect(slot(_roster, &RosterView::on_History_activate));
     // * DELETE USER *
     _baseMU->getMenuItem("ManageUser_DeleteUser_item")->activate.connect(slot(_roster, &RosterView::on_DeleteUser_activate));
     // * ADD USER (for NIL Users) *
     _baseMU->getMenuItem("ManageUser_AddUsertoRoster_item")->activate.connect(slot(_roster, &RosterView::on_AddUser_activate));
     // hide by default
     _baseMU->getMenuItem("ManageUser_AddUsertoRoster_item")->hide();


     // TRANSPORT MENU*
     _baseMA = manage(new BaseGabberWidget("ManageAgent_menu", "Gabber_win"));
     _menuAgent = static_cast<Gtk::Menu*>(_baseMA->get_this_widget());
     _menuAgent->accelerate(*_thisWindow);
     // * MESSAGE *
     _baseMA->getMenuItem("ManageAgent_Message_item")->activate.connect(slot(_roster, &RosterView::on_Message_activate));
     // * OOOCHAT *
     _baseMA->getMenuItem("ManageAgent_OOOChat_item")->activate.connect(slot(_roster, &RosterView::on_OOOChat_activate));
     // * LOGIN TRANS *
     _baseMA->getMenuItem("ManageAgent_ALogin_item")->activate.connect(slot(_roster, &RosterView::on_LoginTransport_activate));
     // * LOGOUT TRANS *
     _baseMA->getMenuItem("ManageAgent_ALogout_item")->activate.connect(slot(_roster, &RosterView::on_LogoutTransport_activate));
     // * TRANSPORT INFO *
     _baseMA->getMenuItem("ManageAgent_AInfo_item")->activate.connect(slot(_roster, &RosterView::on_TransInfo_activate));
     // * EDIT GROUPS
     _baseMA->getMenuItem("ManageAgent_AEditGroups_item")->activate.connect(slot(_roster, &RosterView::on_EditGroups_activate));
     // * REMOVE TRANS *
     _baseMA->getMenuItem("ManageAgent_ARemove_item")->activate.connect(slot(_roster, &RosterView::on_DeleteUser_activate));

     // DOCKLET MENU *
     _baseD = manage(new BaseGabberWidget("Docklet_menu", "Gabber_win"));
     _menuDocklet = static_cast<Gtk::Menu*>(_baseD->get_this_widget());
     // Status
     _md_Presence = _baseD->getPixmapMenuItem("Docklet_Show_menu");
     // * LOGIN *
     _md_Login = _baseD->getMenuItem("Docklet_Login_item");
     _md_Login->activate.connect(slot(&LoginDlg::execute));
     // * LOGOUT *
     _md_Logout = _baseD->getMenuItem("Docklet_Logout_item");
     _md_Logout->activate.connect(slot(this, &GabberWin::on_disconnect));
     _md_Logout->set_sensitive(false);
     // * PREFERENCES *
     _baseD->getMenuItem("Docklet_Preferences_item")->activate.connect(slot(&PrefsWin::execute));
     // * ABOUT *
     _baseD->getMenuItem("Docklet_About_item")->activate.connect(slot(this, &GabberWin::on_About_activate));
     // * HIDE WINDOW *
     _md_Hide = _baseD->getMenuItem("Docklet_Hide_item");
     _md_Hide->activate.connect(slot(this, &GabberWin::toggle_visibility));
     // * SHOW WINDOW *
     _md_Show = _baseD->getMenuItem("Docklet_Show_item");
     _md_Show->activate.connect(slot(this, &GabberWin::toggle_visibility));
     _md_Show->set_sensitive(false);

     // TOOLBAR MENU *
     _baseTB = manage(new BaseGabberWidget("Toolbar_menu", "Gabber_win"));
     _menuToolbar = static_cast<Gtk::Menu*>(_baseTB->get_this_widget());
     _menuToolbar->accelerate(*_thisWindow);
     // * MENUBAR *
     _mtb_Menubar = _baseTB->template getWidget<Gtk::CheckMenuItem>("Toolbar_Menubar_item");
     _mtb_Menubar->set_active(G_App->getCfg().toolbars.menubar);
     _mtb_Menubar->activate.connect(slot(this, &GabberWin::on_Toolbar_activate));
     // * TOOLBAR *
     _mtb_Toolbar = _baseTB->template getWidget<Gtk::CheckMenuItem>("Toolbar_Toolbar_item");
     _mtb_Toolbar->set_active(G_App->getCfg().toolbars.toolbar);
     _mtb_Toolbar->activate.connect(slot(this, &GabberWin::on_Toolbar_activate));
     // * PRESENCE BAR *
     _mtb_Presence = _baseTB->template getWidget<Gtk::CheckMenuItem>("Toolbar_Presence_item");
     _mtb_Presence->set_active(G_App->getCfg().toolbars.presence);
     _mtb_Presence->activate.connect(slot(this, &GabberWin::on_Toolbar_activate));
     // * MESSAGE QUEUE *
     _mtb_Status = _baseTB->template getWidget<Gtk::CheckMenuItem>("Toolbar_Status_item");
     _mtb_Status->set_active(G_App->getCfg().toolbars.status);
     _mtb_Status->activate.connect(slot(this, &GabberWin::on_Toolbar_activate));

     // Toolbar
     getWidget<Gtk::Toolbar>("Gabber_Common_toolbar")->set_style(GTK_TOOLBAR_ICONS);
     // - Add User
     getButton("Gabber_AddUser_btn")->clicked.connect(slot(this, &GabberWin::on_AddUser_activate));
     getButton("Gabber_AddUser_btn")->set_sensitive(false);
     // - Group Chat
     getButton("Gabber_GroupChat_btn")->clicked.connect(slot(&GCJoinDlg::execute));
     getButton("Gabber_GroupChat_btn")->set_sensitive(false);
     // - Browse Agents
     getButton("Gabber_BrowseAgents_btn")->clicked.connect(slot(&AgentBrowserDlg::execute));
     getButton("Gabber_BrowseAgents_btn")->set_sensitive(false);
     // - Manual
     getButton("Gabber_Manual_btn")->clicked.connect(slot(this, &GabberWin::on_Manual_activate));

     // Presence menu
     _menuPresence = getPixmapMenuItem("Gabber_Presence_menu");
     // Main Presence Menu
     _mbuildPresence.main_selected.connect(slot(this, &GabberWin::on_Show_selected));
     _menuPresence->set_submenu(*_mbuildPresence.get_menu());
     _menuPresence->set_sensitive(false);
     // Gabber Presence Menu
     _mbuildGabberPresence.main_selected.connect(slot(this, &GabberWin::on_Show_selected));
     _m_Presence->set_submenu(*_mbuildGabberPresence.get_menu());
     _m_Presence->set_sensitive(false);
     // Docklet Presence Menu
     _mbuildDockletPresence.main_selected.connect(slot(this, &GabberWin::on_Show_selected));
     _md_Presence->set_submenu(*_mbuildDockletPresence.get_menu());
     _md_Presence->set_sensitive(false);
}

gint GabberWin::on_window_delete(GdkEventAny* e)
{
     on_Exit_activate();
     return 0;
}

void GabberWin::on_disconnect()
{
/* This was annoying anyway
 *     Gnome::Dialog* d;
 *     string question = _("Are you sure you want to disconnect from Jabber?");
 *     d = manage(Gnome::Dialogs::question_modal(question,
 *						     slot(this, &GabberWin::handle_disconnect_qry)));
 */

     display_status(Presence::stOffline, _("Logged out"), 0, true, Presence::ptUnavailable);

     // If the Session isn't connected, disconnect the transmitter, otherwise disconnect session
     if (!G_App->getSession().disconnect())
	  G_App->getTransmitter().disconnect();
}

void GabberWin::on_logout_reason_clicked()
{
     // Pop up the status dialog
     on_Show_selected(indexShow(Presence::stOffline));
}

void GabberWin::on_session_connected(const Element& t)
{
     connected = true;

     // Play connected sound
     gnome_triggers_do(NULL, NULL, "gabber", "Connected", NULL);

     // Disable login menuitem
     _m_Login->set_sensitive(false);
     _md_Login->set_sensitive(false);
     _m_Logout->set_sensitive(true);
     _m_LogoutR->set_sensitive(true);
     _md_Logout->set_sensitive(true);

     // Enable agents and roster menus
     _m_Roster->set_sensitive(true);
     _m_Services->set_sensitive(true);
     _m_UserInfo->set_sensitive(true);

     // Disable Add User, Browse Agents and Group Chat toolbar buttons
     getButton("Gabber_AddUser_btn")->set_sensitive(true);
     getButton("Gabber_BrowseAgents_btn")->set_sensitive(true);
     getButton("Gabber_GroupChat_btn")->set_sensitive(true);

     // Update status bar
     push_status_bar_msg(_("Logged in"), 0);

     // Update status toolbar
     if (G_App->getCfg().server.ssl)
	  StatusIndicator::display_connection(StatusIndicator::ctConnectedSSL);
     else
	  StatusIndicator::display_connection(StatusIndicator::ctConnected);

     // Load the list of collapsed roster groups
     _roster->load_collapsed_groups();

     // Allow usr to change status
     _menuPresence->set_sensitive(true);
     _m_Presence->set_sensitive(true);
     _md_Presence->set_sensitive(true);

     // If they're in WelcomeDruid, move them to finish
     if (WelcomeDruid::isRunning())
	 	WelcomeDruid::Connected();

     // Grab the Show, Status, and Priority for presence
     _curShow = indexShow(G_App->getCfg().get_show());
     _curStatus = G_App->getCfg().get_status();
     _curPriority = G_App->getCfg().get_priority();
     _curType = indexType(G_App->getCfg().get_type());

     // We want presence sent after roster refreshes
     _initial_connection = G_App->getSession().evtOnRoster.connect(slot(this, &GabberWin::on_roster));

     // Hook up the XMMS timer
#ifdef HAVE_XMMS
     if (G_App->getCfg().xmms.enabled)
     {
	  _xmms_timer = Gtk::Main::timeout.connect(slot(this, &GabberWin::on_xmms_timeout), (G_App->getCfg().xmms.timer * 1000));
     }
#endif // HAVE_XMMS
}

void GabberWin::on_roster()
{
     // Now we send presence since we have roster loaded internally
     display_status(indexShow(_curShow), _curStatus, _curPriority, true, indexType(_curType));
     _initial_connection.disconnect();

     // Disable presence sounds for 10 seconds
     _roster->disable_presence_sounds(10000);
}

void GabberWin::on_transmitter_disconnected()
{
     connected = false;

     // Update status display
     display_status(Presence::stOffline, _("Disconnected"), 0, true, Presence::ptUnavailable);

     // Save the list of collapsed roster groups
     _roster->save_collapsed_groups();

     // Clear roster..
     _roster->clear();

     // Disable logout menuitem
     _m_Login->set_sensitive(true);
     _md_Login->set_sensitive(true);
     _m_Logout->set_sensitive(false);
     _m_LogoutR->set_sensitive(false);
     _md_Logout->set_sensitive(false);

     // Disable agents and roster menus
     _m_Roster->set_sensitive(false);
     _m_Services->set_sensitive(false);
     _m_UserInfo->set_sensitive(false);

     // Disable Add User, Browse Agents and Group Chat toolbar buttons
     getButton("Gabber_AddUser_btn")->set_sensitive(false);
     getButton("Gabber_BrowseAgents_btn")->set_sensitive(false);
     getButton("Gabber_GroupChat_btn")->set_sensitive(false);

     // Update status bar
     push_status_bar_msg(_("Logged out"), 0);

     // Update status toolbar
     StatusIndicator::display_connection(StatusIndicator::ctDisconnected);
     StatusIndicator::display_presence_signed(false);
     StatusIndicator::display_online_contacts(0);

     // Disallow user to change status
     _menuPresence->set_sensitive(false);
     _m_Presence->set_sensitive(false);
     _md_Presence->set_sensitive(false);
}

void GabberWin::on_Export_activate()
{
     RosterExportDlg::execute();
}

void GabberWin::on_Import_activate()
{
     RosterImportDlg::execute();
}

void GabberWin::on_ConnectionSettings_activate()
{
     LoginDlg::execute_as_settings();
}

void GabberWin::on_AddUser_activate()
{
     AddContactDruid::display(AddContactDruid::auChoice);
}

void GabberWin::on_AddGroup_activate()
{
     Gnome::Dialog* dialog = Gnome::Dialogs::request(*_thisWindow, false, 
						     _("Please enter name of the group:"),
						     "", 0, slot(this, &GabberWin::on_AddGroup_string));
}

void GabberWin::on_AddGroup_string(string group)
{
     if (!group.empty())
     {
	  _roster->add_temp_group(group);
	  _roster->refresh();
     }
}

void GabberWin::on_BlankMessage_activate()
{
     manage(new MessageSendDlg(""));
//     G_App->getMessageManager().display("", MessageManager::translateType("normal"));
}

void GabberWin::on_About_activate()
{
     string info = _("Gabber: The GNOME Jabber Client.");
     info += "\nhttp://gabber.sourceforge.net/";

     vector<string> authors(7);
     authors[0] = _("Julian \"x-virge\" Missig <julian@jabber.org>");
     authors[1] = _("Dave \"DizzyD\" Smith <dave@jabber.org>");
     authors[2] = _("Brandon \"brandon2\" Lees <brandon@aspect.net>");
     authors[3] = _("Eliot \"e-t\" Landrum <eliot@landrum.cx>");
     authors[4] = _("Konrad Podloucky <konrad@crunchy-frog.org>");
     authors[5] = _("Dave \"bigdave\" Lee");
     authors[6] = _("Thomas \"temas\" Muldowney <temas@box5.net>");

     string logo = ConfigManager::get_PIXPATH();
     logo += "gabber-about.png";
     Gnome::About* about = new Gnome::About(ConfigManager::get_PACKAGE(), 
					    ConfigManager::get_VERSION(),
			      _("Copyright (c) 1999-2002 "
			      "Julian Missig and Dave Smith"),
			      authors, info, logo);
     about->set_modal(false);
     about->run();

}

void GabberWin::on_Manual_activate()
{
     GnomeHelpMenuEntry help_entry = { "gabber", "index.html" };
     gnome_help_display (NULL, &help_entry);
}

void GabberWin::on_Show_selected(int custom_index)
{
     string status = G_App->getCfg().get_status(); // config text is all UTF-8

     if (custom_index >= 0) // All normal show types
     {
	  // If we have no status message or the user selects another presence
	  if (!status.length() || custom_index != _curShow) {
		  switch (indexShow(custom_index))
		  {
		  case Presence::stOffline:
		       status = toUTF8(_thisWindow, _("Done for the day. I'll get back to you tomorrow."));
		       break;
		  case Presence::stOnline:
		       status = toUTF8(_thisWindow, _("Online"));
		       break;
		  case Presence::stChat:
		       status = toUTF8(_thisWindow, _("Chat with me!"));
		       break;
		  case Presence::stAway:
		       status = toUTF8(_thisWindow, _("I'm away."));
		       break;
		  case Presence::stXA:
		       status = toUTF8(_thisWindow, _("I'll be away for a while."));
		       break;
		  case Presence::stDND:
		       status = toUTF8(_thisWindow, _("Sorry, I'm busy right now."));
		       break;
		  default:
		       display_status(G_App->getCfg().get_show(), 
				      G_App->getCfg().get_status(),
				      G_App->getCfg().get_priority());
		       return;
		  }
	  }
	  manage(new ShowDlg(indexShow(custom_index), status, _curPriority));
     }
     else // Special presence cases
     {
	  switch (custom_index)
	  {
	  case -2:
	       display_status(Presence::stOffline, "", _curPriority, true, Presence::ptInvisible);
	       break;
	  case -1:
//	       manage(new ShowDlg(indexShow(custom_index), status, _curPriority));
	       break;
	  default:
	       display_status(G_App->getCfg().get_show(), 
			      G_App->getCfg().get_status(),
			      G_App->getCfg().get_priority());
	  }
     }
}

void GabberWin::on_HideOfflineUsers_toggled()
{
     if (_m_HideOffline->is_active())
     {
	  if (_m_HideAgents->is_active())
     	       _roster->set_view_filter(RosterView::rvfOnlineOnlyNoAgents);
	  else
	       _roster->set_view_filter(RosterView::rvfOnlineOnly);
     }
     else
     {
	  if (_m_HideAgents->is_active())
	       _roster->set_view_filter(RosterView::rvfAllNoAgents);
	  else
	       _roster->set_view_filter(RosterView::rvfAll);
     }
}

void GabberWin::on_Exit_activate()
{
     display_status(Presence::stOffline, _("Logged out"), 0, true, Presence::ptUnavailable);

     // If the Session isn't connected, disconnect the transmitter otherwise disconnect session
     if (!G_App->getSession().disconnect())
	  G_App->getTransmitter().disconnect();
     // Save the window size
     save_size();
     // Quit the GabberApp
     G_App->quit();
}

void GabberWin::handle_disconnect_qry(int code)
{
     switch (code)
     {
     case 0: // Yes
          // If the Session isn't connected, disconnect the transmitter, otherwise disconnect session
          if (!G_App->getSession().disconnect())
               G_App->getTransmitter().disconnect();

	  // Open Login Dialog
	  //LoginDlg::execute();
	  // Close G_Win
	  //saveSize();
	  //_thisWindow->destroy();
	  //delete G_Win;
	  //G_Win = NULL;
	  break;
     case 1: // No
	  ;
     }
}

void GabberWin::on_ShowHeadlines_toggled()
{
     // Show/hide the ticker
}

int GabberWin::on_button_press(GdkEventButton* e, Gtk::Object* object)
{
     if (e->type == GDK_BUTTON_PRESS && e->button == 3)
     {
	  _menuToolbar->show_all();
	  _menuToolbar->popup(e->button, e->time);

	  gtk_signal_emit_stop_by_name(object->gtkobj(), "button_press_event");

	  return 1;
     }
     return 0;
}

void GabberWin::on_Toolbar_activate()
{
     g_assert(_mtb_Menubar  != NULL);
     g_assert(_mtb_Toolbar  != NULL);
     g_assert(_mtb_Presence != NULL);
     g_assert(_mtb_Status   != NULL);

     // Save the toolbar state
     ConfigManager& c = G_App->getCfg();
     c.toolbars.menubar  = _mtb_Menubar->get_active();
     c.toolbars.toolbar  = _mtb_Toolbar->get_active();
     c.toolbars.status   = _mtb_Status->get_active();
     c.toolbars.presence = _mtb_Presence->get_active();
     
     // Display the toolbars appropriately
     display_toolbars(_mtb_Menubar->get_active(), _mtb_Toolbar->get_active(), _mtb_Status->get_active(), _mtb_Presence->get_active());
}

gint GabberWin::on_xmms_timeout()
{
     _xmms_timer.disconnect();

#ifdef HAVE_XMMS
     // Hook up the timer again
     if (G_App->getCfg().xmms.timer > 60)
     {
	  _xmms_timer = Gtk::Main::timeout.connect(slot(this, &GabberWin::on_xmms_timeout), (G_App->getCfg().xmms.timer * 1000));
     }

     // If they want XMMS to run and if it's running
     if (G_App->getCfg().xmms.enabled && 
	 xmms_remote_is_running(0) && 
	 xmms_remote_get_playlist_length(0) > 0)
     {
	  gchar* song_title_gchar = xmms_remote_get_playlist_title(0, xmms_remote_get_playlist_pos(0));
	  string new_song_title = toUTF8(song_title_gchar);
	  g_free(song_title_gchar);

	  if (new_song_title != _song_title)
	  {
	       display_status(indexShow(_curShow), _curStatus, _curPriority, true, indexType(_curType));
	  }
     }
     else
     {
	  // If we did have XMMS running but do not anymore...
	  if (!_song_title.empty())
	       display_status(indexShow(_curShow), _curStatus, _curPriority, true, indexType(_curType));
     }
#endif // HAVE_XMMS

     return FALSE;
}

// -------------------------------------------------------------------
//
// Public interface to the window
//
// -------------------------------------------------------------------
void GabberWin::refresh_roster()
{
     _roster->refresh(true);
}

void GabberWin::display_status(Presence::Show show, const string& current_status, int priority, bool send_presence, Presence::Type type)
{
     char* cpriority;
     cpriority = g_strdup_printf("%d", priority);
     // We have this here in case if we want to control applet, docklet, etc.
     if (connected)
     {
	  // Create the core presence packet
	  Presence p("", type, show, current_status, string(cpriority));

	  // If GPG is enabled
	  GabberGPG& gpg = G_App->getGPG();
	  if (gpg.enabled())
	  {
	       string sig;

	       if (gpg.sign(GPGInterface::sigDetach, current_status, sig) != GPGInterface::errOK)
		    cerr << "Couldn't sign status" << endl;
	       else
	       {
		    Element* x = p.addX("jabber:x:signed");
		    x->addCDATA(sig.c_str(), sig.length());
		    StatusIndicator::display_presence_signed(true);
	       }
	  }
#ifdef HAVE_XMMS
	  // If XMMS is enabled; xmms is running; and the presence is an available one
	  if (type == Presence::ptAvailable &&
	      G_App->getCfg().xmms.enabled && 
	      xmms_remote_is_running(0) && 
	      xmms_remote_get_playlist_length(0) > 0)
	  {
	       // Add a gabber:x:music:info extension
	       Element* x = p.addX("gabber:x:music:info");
	       // grab the title of the song playing in XMMS
	       gchar* song_title_gchar = xmms_remote_get_playlist_title(0, xmms_remote_get_playlist_pos(0));
	       _song_title = toUTF8(song_title_gchar);
	       g_free(song_title_gchar);
	       if (!_song_title.empty())
	       {
		    // Add the title
		    x->addElement("title", _song_title);
		    // Add an element which states whether it's paused, stopped, or playing
		    if (xmms_remote_is_paused(0))
		    {
			 x->addElement("state", "paused");
		    }
		    else if (!xmms_remote_is_playing(0))
		    {
			 x->addElement("state", "stopped");
		    }
		    else
		    {
			 x->addElement("state", "playing");
		    }
	       }
	  }
	  else
	  {
	       _song_title = "";
	  }
#endif // HAVE_XMMS
	  StatusIndicator::display_presence(p);
	  // If the presence should be sent
	  if (send_presence)
	  {
	       G_App->getSession() << p;
	       // Fire off the event for my presence
	       evtMyPresence(p);

	       // If this is an invisible presence
	       if (p.getType() == Presence::ptInvisible)
	       {
		    // In order for transports to know to show us as invisible,
		    // we must send this presence directly to each of them
		    for (Roster::iterator it = G_App->getSession().roster().begin(); 
			 it != G_App->getSession().roster().end(); 
			 ++it)
		    {
			 // If this is an agent
			 if (G_App->isAgent(it->getJID()))
			 {
			      // Copy the presence
			      Presence ptrans(p.getBaseElement());
			      // Set it to be to the transport
			      ptrans.setTo(it->getJID());
			      // Send it off
			      G_App->getSession() << ptrans;
			 }
		    }
	       }
	  }
     }
     else // We're not connected
     {
	  StatusIndicator::display_presence(Presence("", Presence::ptUnavailable, Presence::stOffline, toUTF8(_thisWindow, _("Disconnected"))));
     }
     g_free(cpriority);

     // Save the stuff
     _curShow = indexShow(show);
     _curStatus = current_status;
     _curPriority = priority;
     _curType = indexType(type);

     // Set the pixmaps of the presence menu items
     _menuPresence->remove(); // Remove the current label
     if (type == Presence::ptInvisible)
     {
	  _menuPresence->add_label(_("Invisible")); // Set the label to invisible
	  change_pixmap(_menuPresence, _pix_path + "invisible.xpm");
	  change_pixmap(_m_Presence, _pix_path + "invisible.xpm");
	  change_pixmap(_md_Presence, _pix_path + "invisible.xpm");
     }
     else
     {
	  _menuPresence->add_label(getShowName(show)); // Set the label to the show name
	  switch(show)
	  {
	  case Presence::stOffline:
	       change_pixmap(_menuPresence, _pix_path + "offline.xpm");
	       change_pixmap(_m_Presence, _pix_path + "offline.xpm");
	       change_pixmap(_md_Presence, _pix_path + "offline.xpm");
	       break;
	  case Presence::stOnline:
	       change_pixmap(_menuPresence, _pix_path + "online.xpm");
	       change_pixmap(_m_Presence, _pix_path + "online.xpm");
	       change_pixmap(_md_Presence, _pix_path + "online.xpm");
	       break;
	  case Presence::stChat:
	       change_pixmap(_menuPresence, _pix_path + "chat.xpm");
	       change_pixmap(_m_Presence, _pix_path + "chat.xpm");
	       change_pixmap(_md_Presence, _pix_path + "chat.xpm");
	       break;
	  case Presence::stAway:
	       change_pixmap(_menuPresence, _pix_path + "away.xpm");
	       change_pixmap(_m_Presence, _pix_path + "away.xpm");
	       change_pixmap(_md_Presence, _pix_path + "away.xpm");
	       break;
	  case Presence::stXA:
	       change_pixmap(_menuPresence, _pix_path + "xa.xpm");
	       change_pixmap(_m_Presence, _pix_path + "xa.xpm");
	       change_pixmap(_md_Presence, _pix_path + "xa.xpm");
	       break;
	  case Presence::stDND:
	       change_pixmap(_menuPresence, _pix_path + "dnd.xpm");
	       change_pixmap(_m_Presence, _pix_path + "dnd.xpm");
	       change_pixmap(_md_Presence, _pix_path + "dnd.xpm");
	       break;
	  default:
	       change_pixmap(_menuPresence, _pix_path + "offline.xpm");
	       change_pixmap(_m_Presence, _pix_path + "offline.xpm");
	       change_pixmap(_md_Presence, _pix_path + "offline.xpm");
	  }
     }
}

void GabberWin::display_toolbars(bool displaymenubar, bool displaytoolbar, bool displaystatus, bool displaypresence)
{
     g_assert(_dockMenubar  != NULL);
     g_assert(_dockToolbar  != NULL);
     g_assert(_dockStatus   != NULL);
     g_assert(_dockPresence != NULL);

     g_assert(_mtb_Menubar  != NULL);
     g_assert(_mtb_Toolbar  != NULL);
     g_assert(_mtb_Presence != NULL);
     g_assert(_mtb_Status   != NULL);

     // Show or hide the menubar
     if (displaymenubar)
     {
// FIXME: Move Gabber and Services menus (back) into Gabber_Menu_bar
	  _dockMenubar->show();
     }
     else
     {
// FIXME: Move Gabber and Services menus into _menuUser
	  _dockMenubar->hide();
     }
     // Show or hide the toolbar
     (displaytoolbar)  ? _dockToolbar->show()  : _dockToolbar->hide();
     // Show or hide the message queue
     (displaystatus)   ? _dockStatus->show()   : _dockStatus->hide();
     // Show or hide the status bar
     (displaypresence) ? _dockPresence->show() : _dockPresence->hide();

     // Set the checkboxes on the context menu appropriately
     // We check to make sure they're different so we
     // don't call on_Toolbar_activate unnecessarily
     ConfigManager& c = G_App->getCfg();
     if (_mtb_Menubar  ->get_active() != c.toolbars.menubar)
	  _mtb_Menubar ->set_active(c.toolbars.menubar);
     if (_mtb_Toolbar  ->get_active() != c.toolbars.toolbar)
	  _mtb_Toolbar ->set_active(c.toolbars.toolbar);
     if (_mtb_Presence ->get_active() != c.toolbars.presence)
	  _mtb_Presence->set_active(c.toolbars.presence);
     if (_mtb_Status   ->get_active() != c.toolbars.status)
	  _mtb_Status  ->set_active(c.toolbars.status);

     // Resize dock
     getWidget<Gnome::Dock>("Gabber_Dock")->queue_resize();
}

void GabberWin::save_size()
{
     // Save the GabberWin size and position
     int width, height, x, y;
     _thisWindow->get_window().get_size(width, height);
     _thisWindow->get_window().get_root_origin(x, y);
     ConfigManager& c = G_App->getCfg();
     c.window.x = x;
     c.window.y = y;
     c.window.width = width;
     c.window.height = height;
     c.roster.hideoffline = _m_HideOffline->is_active();
     c.roster.hideagents = _m_HideAgents->is_active();
     c.headlines.displayticker = _m_ShowHeadlines->is_active();
}

void GabberWin::toggle_visibility()
{
     // If the window is visible
     if (_thisWindow->is_visible())
     {
	  // Save the size
	  _thisWindow->get_window().get_size(_gwin_width, _gwin_height);
	  _thisWindow->get_window().get_root_origin(_gwin_x, _gwin_y);  
	  // Hide it
	  _thisWindow->hide();
	  // Allow user to show the window
	  _md_Show->set_sensitive(true);
	  _md_Hide->set_sensitive(false);
     }
     else
     {
	  //restore the size we saved because it sometimes forgets
	  _thisWindow->set_default_size(_gwin_width, _gwin_height);
	  _thisWindow->set_uposition(_gwin_x, _gwin_y);

	  // Show it
	  _thisWindow->show();
	  // Allow user to hide the window
	  _md_Show->set_sensitive(false);
	  _md_Hide->set_sensitive(true);
     }
}

void GabberWin::reconnecting()
{
     // Update the status bar   
     push_status_bar_msg(_("Reconnecting..."), 0);
}

void GabberWin::logging_in()
{
     // While it's trying to login, do this stuff:

     // Disable login, enable logout
     _m_Login->set_sensitive(false);
     _md_Login->set_sensitive(false);
     _m_Logout->set_sensitive(true);
     _m_LogoutR->set_sensitive(true);
     _md_Logout->set_sensitive(true);

     // Update the status bar   
     push_status_bar_msg(_("Logging in..."), 0);
}

void GabberWin::toggle_roster_popup(bool inlist)
{
     if (inlist)
     {
	  // Hide Menu Items Used only in the NIL popup
	  _baseMU->getMenuItem("ManageUser_AddUsertoRoster_item")->hide();
     }
     else
     {
	  // Hide Menu Items not used in the NIL popup
	  _baseMU->getMenuItem("ManageUser_EditGroups_item")->hide();
	  _baseMU->getMenuItem("ManageUser_DeleteUser_item")->hide();
     }
}

void GabberWin::raise()
{
     // If this window is visible
     if (_thisWindow->is_visible())
	  // Raise the window
	  gdk_window_show(_thisWindow->get_window().gdkobj());
}

void GabberWin::roster_event(const string& jid, MessageManager::MessageType event)
{
     bool on_roster = true;

     // default is the username
     string nickname = JID::getUser(jid);
     try {
          nickname = G_App->getSession().roster()[JID::getUserHost(jid)].getNickname();
     } catch (Roster::XCP_InvalidJID& e)
     {
          // Special handling for a groupchat ID -- use the resource as the nickname
          if (G_App->isGroupChatID(jid))
               nickname = JID::getResource(jid);
          // Clear the onRoster flag
          on_roster = false;
     }

     if (!on_roster)
     {
          Roster::Item t(jid, nickname);
          t.addToGroup(_("Not in Roster"));
          _roster->addNILItem(t);
     }
}

void GabberWin::clear_event(const string& jid)
{
     _roster->clear_event(jid);
}

void GabberWin::push_status_bar_msg(const string& msg, int millisecs)
{
     // pop!
     StatusBar->pop();
     // disconnect an existing timer (we just popped)
     if (_status_bar_pop_timer.connected())
	  _status_bar_pop_timer.disconnect();

     // push the new message
     StatusBar->push(msg);
     
     // If we want to pop in a certain number of milliseconds,
     if (millisecs != 0)
     {
	  // hook up the new timer
	  _status_bar_pop_timer = Gtk::Main::timeout.connect(slot(this, &GabberWin::pop_status_bar), millisecs);
     }
}

gint GabberWin::pop_status_bar()
{
     // pop!
     StatusBar->pop();

     // disconnect an existing timer
     if (_status_bar_pop_timer.connected())
	  _status_bar_pop_timer.disconnect();

     return FALSE;
}
