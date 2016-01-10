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


#ifndef INCL_GABBER_WIN_H
#define INCL_GABBER_WIN_H

#include "BaseGabberWindow.hh"
#include "GabberUtility.hh"
#include "MessageManager.hh"

#include <gnome--/app.h>
#include <gnome--/appbar.h>

using namespace jabberoo;

class RosterView;

class GabberWin : public BaseGabberWindow
{
public:
     GabberWin();
     void         display_status(jabberoo::Presence::Show show, 
				 const string& current_status, 
				 int priority, 
				 bool send_presence = true,
				 jabberoo::Presence::Type type = jabberoo::Presence::ptAvailable);
     void         display_toolbars(bool displaymenubar, 
				   bool displaytoolbar, 
				   bool displaystatus, 
				   bool displaypresence);
     void         save_size();
     void         toggle_visibility();
     void         refresh_roster();
     void         reconnecting();
     void         logging_in();
     // show and clear message events
     void         roster_event(const string& jid, MessageManager::MessageType event);
     void	  clear_event(const string& jid);
     Presence::Show translate_show(int show_index);
     // Needed to show/hide appropriate Menu Items for User Popup
     void         toggle_roster_popup(bool inlist);
     void         raise();
     void         push_status_bar_msg(const string& msg, int millisecs);
     gint         pop_status_bar();
     // Accessors
     Gtk::Menu*   get_menuUser() { return _menuUser; }
     Gtk::Menu*   get_menuAgent() { return _menuAgent; }
     Gtk::Menu*   get_menuDocklet() { return _menuDocklet; }
     Gnome::App*  get_MainWin() { return MainWin; }
     Gnome::AppBar* get_StatusBar() { return StatusBar; }
     bool         is_connected() { return connected; }
     Signal1<void, const Presence&> evtMyPresence;
protected:
     void         init_menus();
     virtual gint on_window_delete(GdkEventAny* e);
     void         on_Export_activate();
     void         on_Import_activate();
     void         on_ConnectionSettings_activate();
     void         on_AddUser_activate();
     void         on_AddGroup_activate();
     void         on_AddGroup_string(string group);
     void         on_BlankMessage_activate();
     void         on_About_activate();
     void         on_Manual_activate();
     void         on_Show_selected(int custom_index);
     void         on_HideOfflineUsers_toggled();
     void         on_ShowHeadlines_toggled();
     void         on_Exit_activate();
     int          on_button_press(GdkEventButton* e, Gtk::Object* object);
     void         on_Toolbar_activate();
     gint         on_xmms_timeout();

     // Session event handlers
     void         on_session_connected(const Element& t);
     void         on_roster();
     gint         enable_presence_sounds();
     void         on_transmitter_disconnected();
     void         on_disconnect();
     void         on_logout_reason_clicked();
     void         handle_disconnect_qry(int code);
private:
     Gnome::App*         MainWin;
     BaseGabberWidget*   _baseMU;
     BaseGabberWidget*   _baseMA;
     BaseGabberWidget*   _baseD;
     BaseGabberWidget*   _baseTB;
     Gtk::Menu*          _menuUser;
     Gtk::Menu*          _menuAgent;
     Gtk::Menu*          _menuDocklet;
     Gtk::Menu*          _menuToolbar;
     Gnome::AppBar*      StatusBar;
     bool                connected;
     string              _pix_path;
     RosterView*         _roster;
     Gtk::PixmapMenuItem* _menuPresence;
     PresenceMenuBuilder _mbuildPresence;
     PresenceMenuBuilder _mbuildGabberPresence;
     PresenceMenuBuilder _mbuildDockletPresence;
     Gnome::DockItem*    _dockMenubar;
     Gnome::DockItem*    _dockToolbar;
     Gnome::DockItem*    _dockStatus;
     Gnome::DockItem*    _dockPresence;
     int                 _curShow;
     string              _curStatus;
     int                 _curPriority;
     int                 _curType;
     string              _song_title;
     Gtk::PixmapMenuItem* _m_Presence;
     Gtk::MenuItem*      _m_Login;
     Gtk::MenuItem*      _m_Logout;
     Gtk::MenuItem*      _m_LogoutR;
     Gtk::MenuItem*      _m_Agents;
     Gtk::MenuItem*      _m_UserInfo;
     Gtk::MenuItem*      _m_Roster;
     Gtk::MenuItem*      _m_Services;
     Gtk::CheckMenuItem* _m_HideOffline;
     Gtk::CheckMenuItem* _m_HideAgents;
     Gtk::CheckMenuItem* _m_ShowHeadlines;
     Gtk::PixmapMenuItem* _md_Presence;
     Gtk::MenuItem*      _md_Login;
     Gtk::MenuItem*      _md_Logout;
     Gtk::MenuItem*      _md_Show;
     Gtk::MenuItem*      _md_Hide;
     Gtk::CheckMenuItem* _mtb_Menubar;
     Gtk::CheckMenuItem* _mtb_Toolbar;
     Gtk::CheckMenuItem* _mtb_Status;
     Gtk::CheckMenuItem* _mtb_Presence;
     SigC::Connection    _status_bar_pop_timer;
     SigC::Connection    _initial_connection;
     SigC::Connection    _xmms_timer;
     int                 _gwin_width;
     int                 _gwin_height;
     int                 _gwin_x;
     int                 _gwin_y;
};

extern GabberWin* G_Win;

#endif
