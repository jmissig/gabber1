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

#ifndef INCL_CONTACT_INFO_INTERFACE_HH
#define INCL_CONTACT_INFO_INTERFACE_HH

#include "jabberoo.hh"
#include "jabberoox.hh"

#include "BaseGabberWindow.hh"
#include "GabberUtility.hh"
#include "GabberGPG.hh"

#include <gtk--/tooltips.h>

class PrettyJID;

class ContactInfoDlg : 
     public BaseGabberDialog
{
public:
     static void display(const string& jid, Gtk::Window* parentWin = NULL);
     static void display(const string& jid, const jabberoo::Roster::Subscription& type, Gtk::Window* parentWin = NULL);
protected:
     ContactInfoDlg(const string& jid, Gtk::Window* parentWin = NULL);
     ContactInfoDlg(const string& jid, const jabberoo::Roster::Subscription& type, Gtk::Window* parentWin = NULL);
     void init();
private:
     void set_info_label(const string& label_name, const string& frame_name, const string& tab_name, const string& data);
     void send_vcard_request();
     void parse_vcard(const Element& t);
     void send_last_request();
     void parse_last(const Element& t);
     void send_version_request();
     void parse_version(const Element& t);
     void send_time_request();
     void parse_time(const Element& t);    
     void get_status();
     // Event handlers
     void on_ok_clicked();
     void on_cancel_clicked();
     void on_help_clicked();
     void on_default_clicked();
     void on_nickname_changed();
     void on_PrettyJID_changed();
     int on_GPGInfo_press_event(GdkEventButton* e);

     string _pix_path;
     string _share_dir;
     Gtk::Entry* _entNickname;
     Gtk::Label* _lblShow;
     Gtk::Label* _lblStatus;
     Gnome::Pixmap* _pixShow;
     Gtk::EventBox* _evtShow;
     Gnome::Pixmap* _pixS10n;
     Gtk::EventBox* _evtS10n;
     jabberoo::Roster::Item _item;
     bool _onroster;
     bool _last_logout;
     PrettyJID* _pjid;
     string _jid;
     string _oldnick;
     bool   _has_presence;
     Gtk::Tooltips _tips;
     GabberGPG::GPGInfo _gpgInfo;
     Gtk::Window* _parentWin;
};

class MyContactInfoWin
     : public BaseGabberDialog
{
public:
     static void execute();
     MyContactInfoWin();
     ~MyContactInfoWin();
     // Non-static manipulators
     void manage_query(const string& jid);
     void get_key(const Element& t);
     void get_info(const string& jid);
     void get_vCard(const Element& t);
     void set_info(const string& jid);
     void loadconfig();
     void saveconfig();
     void changed();
     void on_name_changed();
     void on_ok_clicked();
     void on_help_clicked();
     void on_cancel_clicked();
     gint on_window_delete(GdkEventAny* e);
public:
     // Signals
     Signal1<void, judo::Element&> evtCompleted;
private:
     static MyContactInfoWin* _Dialog;
     string _key;
     bool _keyUsed;
     // JUD
     Gtk::CheckButton* _chkNoJUD;
     // Basic Info
     Gtk::Entry* _entNickname;
     Gtk::Entry* _entFirstName;
     Gtk::Entry* _entLastName;
     Gtk::Entry* _entFullName;
     Gtk::Entry* _entEMail;
     Gtk::Text*  _txtAbout;
     string _old_email;
     string _old_country;
};

class AgentInfoDlg :
     public BaseGabberDialog
{
public:
     AgentInfoDlg(const jabberoo::Agent& cur_agent);
     AgentInfoDlg(const string& jid);
private:
     string _jid;
     string _pix_path;
     string _share_dir;
     void on_ok_clicked();
     void on_register_clicked();
     void on_search_clicked();
     void on_browse_clicked();
     void on_cancel_clicked();

     jabberoo::Agent* _agent;
     Gtk::Entry*      _entNickname;
     Gnome::Pixmap*   _pixS10n;
};

#endif



