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

#include "GabberApp.hh"

#include "jutil.hh"

// Windows
#include "GabberWin.hh"
#include "WelcomeDruid.hh"
// Dialogs
#include "ErrorManager.hh"
#include "EventManager.hh"
#include "MessageManager.hh"
#include "MessageViews.hh"
#include "PrefsInterface.hh"
#include "S10nInterface.hh"
#include "FTInterface.hh"
// Misc
#include "AutoAway.hh"
#include "GabberGPG.hh"
#include "GabberLogger.hh"
#include "GabberUtility.hh"
#include "TCPTransmitter.hh"

// Spell checking
#include "gtkspell.h"

#include <sys/utsname.h>
#include <glade/glade.h>
#include <libgnome/gnome-triggers.h>
#include <libgnome/gnome-util.h>
#include <libgnomeui/gnome-window-icon.h>
#include <gnome--/client.h>

using namespace jabberoo;
using namespace GabberUtil;

GabberApp* G_App;


GabberApp::GabberApp(int argc, char** argv)
     : manage_groups(true),
       _GnomeApp(ConfigManager::get_PACKAGE(), ConfigManager::get_VERSION(), 
		 argc, argv),
       _SourceDir(ConfigManager::get_GLADEDIR()),
       _Cfg(new ConfigManager(ConfigManager::get_CONFIG())),
       _share_dir(ConfigManager::get_SHAREDIR()),
       _pix_path(ConfigManager::get_PIXPATH()),
       _logging(true),
       _createUser(false),
       _gclient(Gnome::Client::master_client())
{
     cerr << ConfigManager::get_PACKAGE() << " " << ConfigManager::get_VERSION() << endl;

     // Assign this pointer to the global application pointer
     G_App = this;

     // Initialize glade for gnome
     glade_gnome_init();

     // Set the default window icon
     string window_icon = _pix_path + "gnome-gabber.xpm";
     gnome_window_icon_set_default_from_file(window_icon.c_str());
     gnome_window_icon_init();

     // Locate UI/Glade file
     if (!g_file_exists(string(_SourceDir + "Login_dlg.glade").c_str()))
          _SourceDir = "./";
     if (!g_file_exists(string(_SourceDir + "Login_dlg.glade").c_str()))
          _SourceDir = "./ui/";
     if (!g_file_exists(string(_SourceDir + "Login_dlg.glade").c_str()))
          _SourceDir = "./src/";
     if (!g_file_exists(string(_SourceDir + "Login_dlg.glade").c_str()))
     {
	  g_error("Could not find Login_dlg.glade!");
          exit(-1);
     }

     // Set the default profile
     _Cfg->setProfile(_Cfg->profiles.defaultname);

     // Instantiate objects
     _Session     = new Session;
     _Transmitter = new TCPTransmitter;
     setLogging(!_Cfg->logs.none);
     _Logger      = new GabberLogger(_Cfg->logs.dir,
				     _Cfg->get_nick(),
				     _Cfg->logs.html);
     _MessageManager = new MessageManager(_Cfg->spool.dir);
     _ErrorManager = new ErrorManager();
     _EventManager = new EventManager();
     _GabberGPG = new GabberGPG;
     _GabberGPG->set_armor(true);
    
     _AutoAway = new AutoAway;

     // Instantiate classes
     Message::setDateTimeFormat(_Cfg->dates.format);

     // Debugging signals     
     _Session->evtXMLParserError.connect(slot(this, &GabberApp::XML_parser_error));
     _Session->evtTransmitXML.connect(slot(this, &GabberApp::transmit_XML));
     _Session->evtRecvXML.connect(slot(this, &GabberApp::recv_XML));
     _Session->evtTransmitPacket.connect(slot(this, &GabberApp::transmit_packet));

     // Connect up with session signals
     _Session->evtTransmitXML.connect(slot(_Transmitter, &TCPTransmitter::send));
     _Session->evtMessage.connect(slot(this, &GabberApp::on_session_message));
     _Session->evtIQ.connect(slot(this, &GabberApp::on_session_iq));
     _Session->evtAuthError.connect(slot(this, &GabberApp::on_session_auth_error));
     _Session->evtPresenceRequest.connect(slot(this, &GabberApp::on_session_presence_request));
     _Session->evtUnknownPacket.connect(slot(this, &GabberApp::on_session_unknown_packet));
     _Session->evtDisconnected.connect(slot(this, &GabberApp::on_session_disconnected));
     _Session->evtOnVersion.connect(slot(this, &GabberApp::on_session_version));
     _Session->evtOnTime.connect(slot(this, &GabberApp::on_session_time));
     _Session->evtPresence.connect(slot(this, &GabberApp::on_session_presence));
     _Session->evtMyPresence.connect(slot(this, &GabberApp::on_session_my_presence));

     // Connect up transmitter signal
     _Transmitter->evtConnected.connect(slot(this, &GabberApp::on_transmitter_connected));
     _Transmitter->evtDisconnected.connect(slot(this, &GabberApp::on_transmitter_disconnected));
     _Transmitter->evtReconnect.connect(slot(this, &GabberApp::on_transmitter_reconnect));
     _Transmitter->evtError.connect(slot(this, &GabberApp::on_transmitter_error));
     _Transmitter->evtDataAvailable.connect(slot(_Session, &Session::push));

     // Connect GNOME Session management signals
     _gclient->die.connect(slot(this, &GabberApp::on_gnome_die));

     // Startup sound
     gnome_triggers_do(NULL, NULL, "gabber", "Startup", NULL);

     // Startup spell checking
     if (_Cfg->spelling.check)
	  init_spellcheck();

     // Run the wizard if necessary
     if (!_Cfg->wizards.welcomehasrun)
     {
	  WelcomeDruid::execute();
	  _Cfg->wizards.welcomehasrun = true;
     }
     else {
	  // Check config and auto-connect if necessary
	  if (_Cfg->server.autologin)
	       if (_Cfg->server.savepassword)
	       {
		    init_win();

                    // FIXME: Attempt to sign something here so it loops to get the correct passphrase.  It
                    // would be better to do this when the passphrase is first needed but there is some wierd problem
	            // with Gnome::Dialog->run() and receiving the roster at the same time causing the roster to be ignored.
	            GabberGPG& gpg = G_App->getGPG();
	            string dest;
	            if (gpg.enabled() && (gpg.sign(GPGInterface::sigClear, string(""), dest) == GPGInterface::errPass))
	            {
	                 // If we couldn't get the right passphrase, disable gpg
	                 gpg.disable();
	            }

		    login();
	       }
	       else
		    // For *now* we just give them login dialog
		    // Soon this should be a simple password dialog
		    LoginDlg::execute();
	  else
	       LoginDlg::execute();
     }
}

void GabberApp::init_win()
{
     // If we don't have an existing main window
     if (G_Win == NULL)
     {
	  // Create one
	  G_Win  = new GabberWin;
     }
}

void GabberApp::init_spellcheck()
{
     if (!gtkspell_running())
     {
	  // Start up gtkspell for spell checking
	  char* spellcmd[] = { "aspell", "--sug-mode=fast",
			       "pipe", NULL };
// 	  if (!_Cfg->spelling.lang.empty())
// 	  {
// 	       char* langtag = g_strdup_printf("--language-tag=%s", _Cfg->spelling.lang.c_str());
// 	       realloc(spellcmd, sizeof(langtag)*4);
// 	       spellcmd[2] = langtag;
// 	       spellcmd[3] = "pipe";
// 	       spellcmd[4] = NULL;
// 	       g_free(langtag);
// 	  }
	  if (gtkspell_start(NULL, spellcmd) < 0)
	  {
	       char* spellcmd2[] = { "ispell", "-a", NULL };
	       if (gtkspell_start(NULL, spellcmd2) < 0)
	       {
		    cerr << "Neither ispell nor aspell could run, spell-checking disabled" << endl;
	       }
	  }
     }
}

void GabberApp::stop_spellcheck()
{
     if (gtkspell_running())
     {
	  // Stop gtkspell processes
	  gtkspell_stop();
     }
}

void GabberApp::XML_parser_error(int error_code, const string& error_msg)
{
     // Give them cake
     Gnome::Dialog* d;
     string errmsg = _("An error occurred while trying to parse the received XML.\n"
		       "You have been disconnected.");
     if (_Cfg->server.autoreconnect)
	  errmsg += _("\nAttempting to reconnect...");
     errmsg += string("\nError: ") + error_msg;

     d = manage(Gnome::Dialogs::warning(errmsg));
     d->set_modal(true);
     main_dialog(d);

     // Let them eat cake (try to disconnect as gracefully as possible)
     if (G_Win)
	  G_Win->display_status(Presence::stOffline, _("Logged out"), 0, true, Presence::ptUnavailable);
     if (!_Session->disconnect())
	  _Transmitter->disconnect();

     if (_Cfg->server.autoreconnect)
     {
	  // Try to give them more cake (reconnect)
	  init_win();
	  login();
     }
}

void GabberApp::transmit_XML(const char* XML)
{
     cerr << jutil::getTimeStamp() << "<<< " << XML << endl;
}

void GabberApp::recv_XML(const char* XML)
{
     cerr << jutil::getTimeStamp() << ">>> " << XML << endl;
}

void GabberApp::transmit_packet(const Packet& p)
{
#if 0
     if (!_logging)
	  return;
     // Shouldn't we be able to use RTTI to determine if p is Message ???
     if (p.getBaseElement().getName() != "message")
          return;
     Message m = Message(p.getBaseElement());
     _Logger->log(m.getTo(), m);
#endif
}

GabberApp::~GabberApp()
{
     // Release
     delete _Session;
     // set session ponter to null so we know it was deleted if the delete of _Transmitter
     // sends out a signal
     _Session = NULL;
     delete _Transmitter;
     delete _Cfg;
     delete _Logger;
     delete _MessageManager;
     delete _ErrorManager;
     delete _EventManager;
     delete _GabberGPG;
     delete _AutoAway;

     // Delete the main window
     delete G_Win;
}

void GabberApp::run()
{
     // Let the gnome app start processing messages
     Gnome::Main::run();
}

void GabberApp::quit()
{
     // Sync the config
     _Cfg->sync();

     // Stop gtk/gnome message loop
     Gnome::Main::quit();

     // Stop spell checking
     stop_spellcheck();

     delete G_App;
}

GladeXML* GabberApp::load_resource(const char* name)
{
     return glade_xml_new(string(_SourceDir + name + ".glade").c_str(), name);
}

GladeXML* GabberApp::load_resource(const char* name, const char* filename)
{
     return glade_xml_new(string(_SourceDir + filename + ".glade").c_str(), name);
}

void GabberApp::login()
{
     // Tell the main window we're logging in
     G_Win->logging_in();
     // Set the proxy type
     switch (_Cfg->proxy.type)
     {
     case ConfigManager::proxyNone: // no proxy
	  _Transmitter->setProxy("none", "", 0, "", "");
	  break;
     case ConfigManager::proxyHTTP: // http proxy
	  if (_Cfg->proxy.http_type=="")
	  {
	       _Transmitter->setProxy(_Cfg->server.port==443 ? "CONNECT"
				      : "PUT", _Cfg->proxy.server,
				      _Cfg->proxy.port,
				      _Cfg->proxy.username,
				      _Cfg->proxy.password);
	  }
	  else
	  {
	       _Transmitter->setProxy(_Cfg->proxy.http_type,
				      _Cfg->proxy.server,
				      _Cfg->proxy.port,
				      _Cfg->proxy.username,
				      _Cfg->proxy.password);
	  }
	  break;
     case ConfigManager::proxySOCKS4:
	  _Transmitter->setProxy("SOCKS4",
				 _Cfg->proxy.server,
				 _Cfg->proxy.port, "", "");
	  break;
     case ConfigManager::proxySOCKS5:
	  _Transmitter->setProxy("SOCKS5",
				 _Cfg->proxy.server,
				 _Cfg->proxy.port,
				 _Cfg->proxy.username,
				 _Cfg->proxy.password);
	  break;
     default:
	  cout << "unsupported proxy type" << endl;
     }
     // Connect the transmitter
     _Transmitter->connect(_Cfg->get_server(),
			   _Cfg->server.port,
			   _Cfg->server.ssl,
			   _Cfg->server.autoreconnect);
     // Start polling to attempt to keep the connection open
     _Transmitter->startPoll();
}

bool GabberApp::isGroupChatID(const string& jid)
{
     // Simply look for the JID in the list of groupchat ids
     return (_GroupChatIDs.find(JID::getUserHost(jid)) != _GroupChatIDs.end());
}

bool GabberApp::isAgent(const string& jid)
{
     // FIXME: Should browse to the agents and get their info and such
     //        But how do we determine which JIDs to browse to?
     // CHEAP HACK
     return ((jid.find("@") == string::npos));
}

const char* GabberApp::getLogFile(const string& jid) const
{
     // Right now we simply won't support logs of resourced jids
     // In the future, we should replace / with some other char
     // and then check JID with resource against the roster
     // or something.
     return _Logger->getLogFile(JID::getUserHost(jid)).c_str();
}

void GabberApp::setLogDir(const string& dir)
{
     _Logger->setLogDir(dir);
}

void GabberApp::setLogging(bool onoff)
{
     _logging = onoff;
}

void GabberApp::setLogHTML(bool html)
{
     _Logger->setLogHTML(html);
}

// -------------------------------------
//
// Session event handlers
//
// -------------------------------------
void GabberApp::on_session_message(const Message& m)
{
     bool encrypted = false;
     // Update group chat IDs if necessary
     if (m.getType() == Message::mtGroupchat)
	  _GroupChatIDs.insert(JID::getUserHost(m.getFrom()));

     // Route messages properly
     if (m.getType() == Message::mtError)
     {
	  _ErrorManager->add(m);
	  return;
     }

     if ( (m.findX("jabber:x:encrypted")) != NULL)
	     encrypted = true;
     // If logging is enabled and the message body isn't empty
     // FIXME: MessageView should have a log function
     //        which each class should implement
     //        - which then passes the data to be logged to GabberLogger
     _Logger->tryToLog(m.getFrom(), m, encrypted);

     // Pass message on to the MessageManager
     _MessageManager->add(m);
}

void GabberApp::on_session_iq(const Element& t)
{
    const Element* q = t.findElement("query");

    // Catch the odd case of an IQ not having a <query> tag..
    // It's not really so odd, temas, there is browsing ;)
    if (q == NULL) return;

    // jabber:iq:oob
    if (t.cmpAttrib("type", "set") && q->cmpAttrib("xmlns", "jabber:iq:oob"))
    {
        manage(new FTRecvView(t));
    }
}

void GabberApp::on_session_auth_error(int ErrorCode, const char* ErrorMsg)
{
     Gnome::Dialog* d;
  
     // Check for the password
     string password = _Cfg->server.password;
     if (password.empty())
     {
	  d = manage(Gnome::Dialogs::warning(_("Error, unable to load the password.\nPlease try logging in again.")));
	  d->set_modal(true);
	  main_dialog(d);
	  LoginDlg::execute();
     }

     switch (ErrorCode)
     {
     case 401:
  	  // If they have yet to log in with the wizard, then give them a special dialog
	  if (!WelcomeDruid::hasTriedConnecting())
	  {
	       d = manage(Gnome::Dialogs::question_modal(_("Are you sure you would like Gabber to try\n"
								 "to register ") + 
							       _Cfg->server.username + "@" +
							       _Cfg->get_server() + _("?"),
							       slot(this, &GabberApp::handle_create_user_dlg)));
	  } 
	  else
	  {
	       d = manage(Gnome::Dialogs::question_modal(_("Gabber could not log you in.\n"
								 "Would you like Gabber to try to create a new\n"
								 "account on the selected Jabber server?"),
							       slot(this, &GabberApp::handle_create_user_dlg)));
	  }
	  break;
     case 409:
	  LoginDlg::execute();		     
	  d = manage(Gnome::Dialogs::warning(_("Attempt to create a new user failed: \n")
					     + string(ErrorMsg) 
					     + _("\n. Please select a different user name or verify your \n"
					     "login information.")));
	  d->set_modal(true);
	  break;
     case 500:
	  // If user is trying to auth using Digest, it's possible the server doesn't support it.
	  LoginDlg::execute();
	  d = manage(Gnome::Dialogs::warning(_("Login failed. It is possible this server does not\n"
					       "support digest authentication.\n"
					       "Try connecting again with digest disabled.")));
	  d->set_modal(true);
	  main_dialog(d);
     }

      
}

void GabberApp::on_session_presence_request(const Presence& p)
{
     if (p.getType() == Presence::ptSubRequest)
     {
	  // Play SubRequest sound
	  gnome_triggers_do(NULL, NULL, "gabber", "SubRequest", NULL);

	  if (isAgent(p.getFrom()))
	  {
	       string host = JID::getHost(p.getFrom());
	       // Send jabber:iq:agent to the agent to get its name
	       string id = _Session->getNextID();
	       Packet iq("iq");
	       iq.setID(id);
	       iq.setTo(host);
	       iq.getBaseElement().putAttrib("type", "get");
	       Element* query = iq.getBaseElement().addElement("query");
	       query->putAttrib("xmlns", "jabber:iq:agent");
	       *_Session << iq;
	       _Session->registerIQ(id, slot(this, &GabberApp::on_agent_reply));
	       // Also send a jabber:iq:agents query to server to try to get some of the transports
	       // since very few of them actually reply to jabber:iq:agent queries
	       // Set the _current_agent_jid for on_agents_reply to find the right name
	       _current_agent_host = host;
	       _current_agent_jid = p.getFrom();
	       _Session->queryNamespace("jabber:iq:agents", slot(this, &GabberApp::on_agents_reply), _Cfg->get_server());

	       // Add a part of the jid in case the agent doesn't reply
	       *_Session << Presence(p.getFrom(), Presence::ptSubscribed);
	       string transName = host.substr(0, host.find('.'));
	       _Session->roster() << Roster::Item(p.getFrom(), transName);
	  }
	  else
	       S10nReceiveDlg::execute(p);
     }
}

void GabberApp::on_agent_reply(const Element& iq)
{
     const Element* query = iq.findElement("query");
     if (iq.cmpAttrib("type", "result")) 
     {
          string jid = iq.getAttrib("jid");
	  if (query != NULL)
	  {
	       string agentname = query->getChildCData("name");
	       // Rename the agent to its actual name
	       if (!agentname.empty())
		    _Session->roster() << Roster::Item(_current_agent_jid, agentname);
	  }
     }
}

void GabberApp::on_agents_reply(const Element& iq)
{
     const Element* query = iq.findElement("query");
     if (iq.cmpAttrib("type", "result") && query != NULL)
     {
	  // Walk the children
	  Element::const_iterator it = query->begin();
	  for (; it != query->end(); ++it)
          {
	       if ((*it)->getType() != Node::ntElement)
		    continue;
	       Element& agent = *static_cast<Element*>(*it);
	       
               if (agent.getName() != "agent")
                    continue;
	       // Find the agent which matches the current agent jid
               string jid = agent.getAttrib("jid");
	       if (jid != _current_agent_host)
		    continue;
	       string agentname = agent.getChildCData("name");
	       // Rename the agent to its actual name
	       if (!agentname.empty())
		    _Session->roster() << Roster::Item(_current_agent_jid, agentname);
	  }
     }
}
	       

void GabberApp::on_session_unknown_packet(const Element& t)
{
     _Log << "Unknown packet: " << t.toString() << endl;
}

void GabberApp::on_session_disconnected()
{
     // Disconnect the socket...
     _Transmitter->disconnect();
}

void GabberApp::on_session_version(string& name, string& version, string& os)
{
     name = "Gabber";
     version = ConfigManager::get_VERSION();
     // Run uname
     struct utsname osinfo;
     uname(&osinfo);
     os = string(osinfo.sysname) + " " + string(osinfo.machine);
}

void GabberApp::on_session_time(string& UTF8Time, string& UTF8TimeZone)
{
     UTF8Time = toUTF8(UTF8Time);
     UTF8TimeZone = toUTF8 (UTF8TimeZone);
}

// -------------------------------------
//
// Transmitter event handlers
//
// -------------------------------------
void GabberApp::on_transmitter_connected()
{
     // Check for the password
     string password = _Cfg->server.password;
     if (password.empty())
     {
	  Gnome::Dialog* d = manage(Gnome::Dialogs::warning(_("Error, unable to load the password.\n"
							      "Please try logging in again.")));
	  d->set_modal(true);
	  main_dialog(d);
	  LoginDlg::execute();
     }
     else
     {
	  // Default to auto-select 0k, digest, plaintext
	  Session::AuthType atype = Session::atAutoAuth;
	  // If they want to force usage of plaintext, do so
	  if (_Cfg->server.plaintext)
	       atype = Session::atPlaintextAuth;

	  // Tell the session to connect with whatever type was selected
	  _Session->connect(_Cfg->get_server(),
			    atype,
			    _Cfg->server.username,
			    _Cfg->server.resource,
			    password,
			    _createUser);
	  // We've attempted to create the user, set it to false
	  _createUser = false;
     }
}

void GabberApp::on_transmitter_disconnected()
{
     // only do this if the session exists
     if (_Session)
     {
          // Tell the session to disconnect
          cerr << "DISCONNECTED" << endl;
          _Session->disconnect();
     }
}

void GabberApp::on_transmitter_reconnect()
{
     if (G_Win)
	  G_Win->reconnecting();
}

/*void GabberApp::on_transmitter_error(TCPTransmitter::Error e)*/
void GabberApp::on_transmitter_error(const string & emsg)
{
     string errormessage = _("Transmitter error. Disconnected: ");
     errormessage += emsg;

     Gnome::Dialog* d = manage(Gnome::Dialogs::warning(errormessage));
     d->set_modal(true);
     main_dialog(d);

     cerr << "Transmitter error. Disconnected: " << emsg << endl;
     _Transmitter->disconnect();
}

void GabberApp::handle_create_user_dlg(int code)
{
     // Default to not creating new user
     _createUser = false;
     switch (code)
     {
     case 0:
	  // Set the "create new user" flag
	  _createUser = true;
	  // Tell the session to attempt to create a user and connect
	  on_transmitter_connected();
	  break;
     case 1:
	  // Disconnect Transmitter
	  _Transmitter->disconnect();
	  break;
     }
}

void GabberApp::on_session_presence(const jabberoo::Presence& p, const jabberoo::Presence::Type prev_type)
{
     // If the presence was unavailable from a transport make all other items on the roster
     // that are from that transport unavailable as well
     if ((p.getType() == Presence::ptUnavailable) && isAgent(p.getFrom()))
     {
	  cerr << "got unavailable presence from transport" << endl;
          Roster::iterator it = _Session->roster().begin();
          string transport = JID::getHost(p.getFrom());
          for ( ; it != _Session->roster().end(); ++it)
          {
               // Check if the jid is from the transport but skip over the transport itself
               if ((transport == JID::getHost(it->getJID())) && (p.getFrom() != it->getJID()))
               {
                    // Check to see if we have an available presence from this jid
                    if (_Session->presenceDB().available(it->getJID()))
                    {
                         // Setup unavailable presence for JID
                         Presence p("", Presence::ptUnavailable);
                         p.getBaseElement().putAttrib("from", it->getJID());
                         // send the unavail presence to the presenceDB, roster and whoever else wants it
                         _Session->presenceDB().insert(p);
                         _Session->roster().update(p, Presence::ptAvailable);
		         _Session->evtPresence(p, Presence::ptAvailable);
                    }
               }
          }
     }
     // Jer doesn't like this code.  Transports are supposed to handle this for us.
     // Naturally, they don't really.
#if 0
     else if ((p.getType() == Presence::ptAvailable) && isAgent(p.getFrom()))
     {
	  Roster::iterator it = _Session->roster().begin();
	  string transport = JID::getHost(p.getFrom());

	  // if the presence needs to be signed, sign it here so it is only
	  // signed once
	  char* cpriority;
	  cpriority = g_strdup_printf("%d", _Cfg->get_priority());
	  string current_status = _Cfg->get_status();
	  Presence::Show current_show = indexShow(_Cfg->get_show());
	  string signature;

          if (_GabberGPG->enabled())
          {
              if (_GabberGPG->sign(GPGInterface::sigDetach, current_status, signature) != GPGInterface::errOK)
                    cerr << "Couldn't sign status" << endl;
	  }

          for ( ; it != _Session->roster().end(); ++it)
	  {
	       if ((transport == JID::getHost(it->getJID())) && (p.getFrom() != it->getJID()))
	       {
		     Presence p(it->getJID(), Presence::ptAvailable, current_show, current_status, cpriority);
		     if (_GabberGPG->enabled())
		     {
		          Element* x = p.addX("jabber:x:signed");
		          x->addCDATA(signature.c_str(), signature.length());
		     }

		     *_Session << p;
	       }
	  }
     }
#endif
}

void GabberApp::on_session_my_presence(const jabberoo::Presence& p)
{
     // Presence error handler
     if ((p.getType() == Presence::ptError))
     {
	  // If we ever handle more errors here, use a switch()
	  if (p.getErrorCode() == 400)
	  {
	       Gnome::Dialog* d = manage(Gnome::Dialogs::warning(substitute(_("Unfortunately, %s does not support Invisible.\nSetting to Available."), fromUTF8(_Cfg->get_server()))));
	       d->set_modal(true);
	       main_dialog(d);
	       if (G_Win)
	       {
		    G_Win->display_status(Presence::stOnline, p.getStatus(), p.getPriority(), true, Presence::ptAvailable);
	       }
	  }
     }
}

// -------------------------------------
//
// GNOME Session
//
// -------------------------------------
void GabberApp::on_gnome_die()
{
     quit();
}

gint GabberApp::on_gnome_save_yourself(gint phase, 
				       GnomeSaveStyle save_style, 
				       gint is_shutdown, 
				       GnomeInteractStyle interact_style, 
				       gint is_fast, 
				       gpointer client_data)
{
     // Wooo... go whacky session management settings
     vector<string> argv(4);

     argv.push_back(static_cast<char*>(client_data)); // yuck - you got that right

     // argv.push_back("--somearg");
     // argv.push_back("somevalue");
     
     _gclient->set_clone_command(argv);
     _gclient->set_restart_command(argv);

     return TRUE;
}
