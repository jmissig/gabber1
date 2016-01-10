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

#ifndef INCL_PREFS_INTERFACE_HH
#define INCL_PREFS_INTERFACE_HH

#include "BaseGabberWindow.hh"
#include "GabberUtility.hh"

#include <gtk--/spinbutton.h>
#include <gnome--/color-picker.h>

class LoginDlg : 
     public BaseGabberDialog
{
public:
     static void execute();
     static void execute_as_settings();
     // Destructor
     ~LoginDlg();
protected:
     // Internalize default constructor
     LoginDlg(bool is_settings);
     // Non-static manipulators
     void on_OK_clicked();
     void on_Login_clicked();
     void on_Cancel_clicked();
     gint on_window_delete(GdkEventAny* e);
     void on_Help_clicked();
     void loadconfig();
     void saveconfig();
     void changed();
     gint on_Username_key_press(GdkEventKey* e);
     void on_SSL_toggled();
     ConfigManager::Proxy get_proxy_selection();
     void set_proxy_selection(ConfigManager::Proxy proxy_selection);
     void proxy_changed();
private:
     bool _is_settings;
     Gtk::Button* _btnLogin;
     Gtk::Button* _btnOK;
     Gtk::Entry*  _entUsername;
     Gtk::Entry*  _entServer;
     Gtk::SpinButton*  _spinPort;
     Gtk::SpinButton*  _spinPriority;
     Gtk::Entry*  _entPassword;
     Gtk::Entry*  _entResource;
     Gtk::CheckButton* _chkSavePassword;
     Gtk::CheckButton* _chkPlaintext;
     Gtk::CheckButton* _chkSSL;
     Gtk::CheckButton* _chkAutologin;
     Gtk::CheckButton* _chkAutoreconnect;
     static LoginDlg*  _Dialog;
};

class PrefsWin : public BaseGabberDialog
{
public:
     static void execute();
     PrefsWin();
     ~PrefsWin();
     // Non-static manipulators
     void loadconfig();
     void saveconfig();
     void changed();
     void colorchanged(unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4);
     void on_ok_clicked();
     void on_help_clicked();
     void on_cancel_clicked();
     gint on_window_delete(GdkEventAny* e);
     void on_GPGKey_select_row(gint,gint,GdkEvent*);
private:
     static PrefsWin* _Dialog;
     // Log widgets
     Gtk::CheckButton* _chkLogs;
     Gtk::CheckButton* _chkLogsEncrypted;
     Gtk::CheckButton* _chkLogsGroupchat;
     Gtk::CheckButton* _chkLogsSave;
     Gtk::CheckButton* _chkLogsPurge;
     //Gnome::FileEntry* _fileLogsDir;
     Gtk::Entry*       _entLogsDir;
     Gtk::CheckButton* _chkLogsHTML;
     Gtk::CheckButton* _chkLogsXML;
     // Connection widgets
     Gtk::CheckButton* _chkConnAutologin;
     Gtk::CheckButton* _chkConnAutorecon;
     // Messages widgets
     Gtk::CheckButton* _chkMsgsSendMsgs;
     Gtk::CheckButton* _chkMsgsSendOOOChats;
     Gtk::CheckButton* _chkMsgsRecvNon;
     Gtk::CheckButton* _chkMsgsRecvMsgs;
     Gtk::CheckButton* _chkMsgsRecvOOOChats;
     Gtk::CheckButton* _chkMsgsOpenMsgs;
     Gtk::CheckButton* _chkMsgsRaise;
     // Color widgets
     Gnome::ColorPicker* _colorAvailable;
     Gnome::ColorPicker* _colorChat;
     Gnome::ColorPicker* _colorAway;
     Gnome::ColorPicker* _colorXa;
     Gnome::ColorPicker* _colorDnd;
     Gnome::ColorPicker* _colorUnavailable;
     Gnome::ColorPicker* _colorNotInList;
     Gnome::ColorPicker* _colorStalker;
     Gnome::ColorPicker* _colorMessage;
     Gnome::ColorPicker* _colorOOOChat;
     // Icons
     Gtk::CheckButton* _chkIcons;
     // Size and Pos
     Gtk::CheckButton* _chkSize;
     Gtk::CheckButton* _chkPos;
     // Toolbar widgets
     Gtk::CheckButton* _chkMenubarShow;
     Gtk::CheckButton* _chkToolbarShow;
     Gtk::CheckButton* _chkStatusShow;
     Gtk::CheckButton* _chkPresenceShow;
     // Spelling
     Gtk::CheckButton* _chkSpellCheck;
     // Docklet
     Gtk::CheckButton* _chkDockletShow;
     // Chats
     Gtk::CheckButton* _chkChatsOOOTime;
     Gtk::CheckButton* _chkChatsGroupTime;
     //Encryption
     Gtk::CheckButton* _chkGPGEnable;
     Gtk::Entry*       _entGPGKeyserver;
     Gtk::CList*       _clistGPGKey;
     bool	       _gotKeys;
     // Auto-away
     Gtk::CheckButton*   _chkAutoAway;
     Gtk::SpinButton*    _spinAwayAfter;
     Gtk::SpinButton*    _spinXAAfter;
     Gtk::Text*          _txtStatus;
     Gtk::CheckButton*   _chkChangePriority;
};

#endif

