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


#ifndef INCL_GABBER_APP_HH
#define INCL_GABBER_APP_HH

#include "jabberoo.hh"

#include "ConfigManager.hh"
#include "TCPTransmitter.hh"

#include <fstream>
#include <glade/glade-xml.h>
#include <sigc++/object_slot.h>
#include <gnome--/client.h>
#include <gnome--/main.h>

class AutoAway;
class ErrorManager;
class EventManager;
class GabberGPG;
class GabberLogger;
class GabberWin;
class MessageManager;


class GabberApp
     : public SigC::Object
{
public:
     GabberApp(int argc, char** argv);
     ~GabberApp();
public:
     void init_win();
     void init_spellcheck();
     void stop_spellcheck();
     void run();
     void quit();
     GladeXML* load_resource(const char* name);
     GladeXML* load_resource(const char* name, const char* filename);
public:
     // Accessors
     jabberoo::Session& getSession()       { return *_Session; }
     ConfigManager&     getCfg()           { return *_Cfg; }
     TCPTransmitter&    getTransmitter()   { return *_Transmitter; }
     MessageManager&    getMessageManager() { return *_MessageManager; }
     ErrorManager&      getErrorManager()  { return *_ErrorManager; }
     EventManager&      getEventManager()  { return *_EventManager; }
     GabberGPG& 	getGPG()  	   { return *_GabberGPG; }
     GabberLogger&      getLogger()	   { return *_Logger; }
     void login();
     bool isGroupChatID(const string& jid);
     bool isAgent(const string& jid);
     const char* getLogFile(const string& jid) const;
     void setLogDir(const string& dir);
     void setLogging(bool onoff);
     void setLogHTML(bool html);
     bool manage_groups;
protected:
     GabberWin* _MainWindow;
     // Session events
     void on_session_message(const jabberoo::Message& m);
     void on_session_iq(const judo::Element& t);
     void on_update_reply(const judo::Element& iq);
     void on_session_auth_error(int ErrorCode, const char* ErrorMessage);
     void on_session_presence_request(const jabberoo::Presence& p);
     void on_agent_reply(const judo::Element& iq);
     void on_agents_reply(const judo::Element& iq);
     void on_session_unknown_packet(const judo::Element& t);
     void on_session_roster(const judo::Element& t);
     void on_session_disconnected();
     void on_session_version(string& name, string& version, string& os);
     void GabberApp::on_session_time(string& UTF8Time, string& UTF8TimeZone);
     void on_session_presence(const jabberoo::Presence& p, jabberoo::Presence::Type prev);
     void on_session_my_presence(const jabberoo::Presence& p);

     // Transmitter events
     void on_transmitter_connected();
     void on_auth_type_received(const judo::Element& iq);
     void on_transmitter_disconnected();
     void on_transmitter_reconnect();
     void on_transmitter_error(const string & emsg);
     // Other sundry events 
     void XML_parser_error(int error_code, const string& error_msg);
     void transmit_XML(const char* XML);
     void recv_XML(const char* XML);
     void transmit_packet(const jabberoo::Packet& p);
     // Dialog handlers
     void handle_create_user_dlg(int result);
     // Misc
     void init_log();
     // GNOME Session
     void on_gnome_die();
     gint on_gnome_save_yourself(gint phase, GnomeSaveStyle save_style, gint is_shutdown, GnomeInteractStyle interact_style, gint is_fast, gpointer client_data);
private:
     Gnome::Main        _GnomeApp;
     string             _LOGDIR;
     string             _SourceDir;
     TCPTransmitter*    _Transmitter;
     jabberoo::Session* _Session;
     GabberLogger*      _Logger;
     ConfigManager*     _Cfg;
     ConfigManager*     _DefaultCfg;
     ofstream           _Log;
     string             _share_dir;
     string             _pix_path;
     bool               _logging;
     set<string, jabberoo::JID::Compare> _GroupChatIDs;
     string             _nickname;
     MessageManager*	_MessageManager;
     ErrorManager*      _ErrorManager;
     EventManager*      _EventManager;
     GabberGPG*		_GabberGPG;
     bool               _createUser;
     bool               _spellcheck;
     string             _current_agent_host;
     string             _current_agent_jid;
     AutoAway*	        _AutoAway;
     Gnome::Client*     _gclient;
};

extern GabberApp* G_App;

#endif

