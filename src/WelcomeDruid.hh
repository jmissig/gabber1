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


#ifndef INCL_WELCOME_DRUID_HH
#define INCL_WELCOME_DRUID_HH

#include "jabberoo.hh"

#include "BaseGabberWindow.hh"

#include <gtk--/progressbar.h>
#include <gnome--/druid.h>

class WelcomeDruid : 
     public BaseGabberWindow
{
public:
     static void execute();
     static bool isRunning();
     static bool hasTriedConnecting();
     static void Connected();
     // Destructor
     ~WelcomeDruid();
protected:
     // Non-static manipulators
     void OnCancel();
     void OnAccountChanged();
     void OnServerChanged();
     void OnResourceChanged();
     void OnAccountPrepare();
     void OnPasswordPrepare();
     void OnPasswordChanged();
     void OnConfirmPrepare();
     void OnLoggingInPrepare();
     void get_jud_key(const string& jid);
     void get_jud(const Element& t);
     void jud_finished(const Element& t);
     void finished();
     void OnFinish();
     gint on_refresh();
     // Internalize default constructor
     WelcomeDruid();
private:
     Gnome::Druid* _druid;
     static WelcomeDruid* _Dialog;
     Gtk::Entry* _entFirstName;
     Gtk::Entry* _entLastName;
     Gtk::Entry* _entUsername;
     Gtk::Entry* _entServer;
     Gtk::Entry* _entResource;
     Gtk::Entry* _entPassword;
     Gtk::Entry* _entConfirmPassword;
     Gtk::CheckButton* _chkSavePassword;
     Gtk::Label* _lblProgress;
     Gtk::ProgressBar* _barProgress;
     SigC::Connection _refresh_timer;
     string _first, _last, _nick, _email;
     bool _tried_connecting;
};

#endif


