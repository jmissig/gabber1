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

#include "ConfigManager.hh"

#include "GabberConfig.hh"
#include "GabberUtility.hh"
#include "GabberWin.hh"

#include <libgnome/gnome-config.h>
#include <libgnome/gnome-util.h>

ConfigManager::ConfigManager(const char* name)
     : _CfgName(name),
       _CfgProfile("default")
{
     int i = 0;
     char configpath[64];

     gnome_config_push_prefix(_CfgName.c_str());

     // Get the home directory
//FIXME: if I use this, then config doesn't get sync'd!!!!!!!!!!!
//     gchar* g_homedir = g_get_home_dir();
//     string homedir = string(g_homedir);
//     g_free(g_homedir);

     autoaway.awayafter = getIntValue("AutoAway/AwayAfter=5");
     autoaway.xaafter = getIntValue("AutoAway/XAAfter=15");
     autoaway.enabled = getBoolValue("AutoAway/Enabled=true");
     autoaway.status = getStrValue("AutoAway/Status=Automatically away due to being idle");
     autoaway.changepriority = getBoolValue("AutoAway/ChangePriority=false");

     chats.groupheight = getIntValue("Chats/GroupHeight=300");
     chats.grouptime = getBoolValue("Chats/GroupTime=false");
     chats.groupwidth = getIntValue("Chats/GroupWidth=500");
     chats.oooheight = getIntValue("Chats/OOOHeight=300");
     chats.oootime = getBoolValue("Chats/OOOTime=false");
     chats.ooowidth = getIntValue("Chats/OOOWidth=325");

     colors.icons = getBoolValue("Colors/Icons=true");

     // Load presence colors
     const int   ptable_sz = 7;
     const string ptable[ptable_sz] = { "online"   , "chat"   , "away"   , "xa"     , "dnd"    , "offline", "stalker" };
     const string ctable[ptable_sz] = { "#0066CC"  , "#0099CC", "#004D80", "#004D4D", "#666633", "#666666", "#663333" };

     for (i = 0; i < ptable_sz; i++)
          colors.presence_colors.insert(make_pair(ptable[i], Gdk_Color(getStrValue("Colors/" + ptable[i] + "=" + ctable[i]))));

     const int   ntable_sz = 5;
     // load event colors
     const string ntable[ptable_sz] = { "evMessage", "evChat", "evRoster", "evGCI", ""};
     const string ztable[ptable_sz] = { "#669999", "#996699", "#999966", "#999966", "#000000" };

     for (i = 0; i < ntable_sz; i++)
	  colors.event_colors.insert(make_pair(ntable[i], Gdk_Color(getStrValue("Colors/" + ntable[i] + "=" + ztable[i]))));

     dates.chatformat = getStrValue("Dates/ChatFormat=[%T] ");
     dates.format = getStrValue("Dates/Format=%c"); // julian likes %d %b %Y %H:%M:%S
     dates.gcformat = getStrValue("Dates/GCFormat=[%T] ");

     docklet.show = getBoolValue("Docklet/Show=false");

     filetransfer.port = getIntValue("FileTransfer/Port=0");

     gpg.enabled = getBoolValue("GPG/Enabled=false");
     gpg.keyserver = getStrValue("GPG/keyserver=pgpkeys.mit.edu");
     gpg.secretkeyid = getStrValue("GPG/SecretKeyID=");
     for (i = 0; ; i++)
     {
          g_snprintf(configpath, 32, "GPG/keymap-%d=", i);
	  string pair = getStrValue(configpath);
	  if (pair.empty())
	       break;
          gpg.keymap.insert(make_pair(pair.substr(0, pair.rfind("/")), pair.substr(pair.rfind("/") + 1)));
     }

     headlines.displayticker = getBoolValue("Headlines/DisplayTicker=false");

     ignorelist.outsidecontact = getBoolValue("IgnoreList/OutsideContact=true");
     for (i = 0; ; i++)
     {
	  g_snprintf(configpath, 64, "IgnoreList/ignore-%d=", i);
	  string val = getStrValue(configpath);
	  if (val.empty())
	       break;
	  ignorelist.ignore.push_back(configpath);
     }

     // Get the home directory, free it (stupid C strings), then tack on .Gabber
//     string g_fulldir = homedir + "/.Gabber/";
#ifdef MACOSX
     logs.dir = getStrValue("Logs/Dir=~/.Gabber");
#else // MACOSX
     logs.dir = getStrValue("Logs/Dir=~/.Gabber/");
#endif // MACOSX
     logs.encrypted = getBoolValue("Logs/Encrypted=true");
     logs.groupchat = getBoolValue("Logs/Groupchat=true");
     logs.html = getBoolValue("Logs/HTML=true");
     logs.none = getBoolValue("Logs/None=false");
     logs.purge = getBoolValue("Logs/Purge=false");
     logs.save = getBoolValue("Logs/Save=true");
     logs.xml = getBoolValue("Logs/XML=false");
     logs.lastMovedMonth = getIntValue("Logs/LastMovedMonth=0");
     logs.lastMovedYear  = getIntValue("Logs/LastMovedYear=0");

     msgs.height = getIntValue("Messages/Height=300");
     msgs.openmsgs = getBoolValue("Messages/OpenMsgs=false");
     msgs.raise = getBoolValue("Messages/Raise=false");
     msgs.recvmsgs = getBoolValue("Messages/RecvMsgs=false");
     msgs.recvnon = getBoolValue("Messages/RecvNon=true");
     msgs.recvooochats = getBoolValue("Messages/RecvOOOChats=false");
     msgs.sendmsgs = getBoolValue("Messages/SendMsgs=false");
     msgs.sendooochats = getBoolValue("Messages/SendOOOChats=true");
     msgs.showadvancedopts = getBoolValue("Messages/ShowAdvancedOpts=false");
     msgs.width = getIntValue("Messages/Width=350");

     profiles.defaultname = getStrValue("Profiles/Defaultname=gabber");

     proxy.http_type = getStrValue("Proxy/HTTPType=");
     proxy.password = getPrivStrValue("Proxy/Password=");
     proxy.port = getIntValue("Proxy/Port=8080");
     proxy.server = getStrValue("Proxy/Server=");
     proxy.type = getIntValue("Proxy/Type=0");
     proxy.username = getStrValue("Proxy/Username=");

     roster.hideagents = getBoolValue("Roster/HideAgents=false");
     roster.hideoffline = getBoolValue("Roster/HideOffline=false");
     for (i = 0; ; i++)
     {
	  g_snprintf(configpath, 32, "Roster/clpsd_grp-%d=", i);
          string groupname = getStrValue(configpath);
          if (!groupname.empty())
          {
               roster.collapsed_groups.push_back(groupname);
               putValue(configpath, ""); // Empty the list, since they might collapse *less* next time
	  }
          else
	       break;
     }

     server.autologin = getBoolValue("Server/AutoLogin=false");
     server.autoreconnect = getBoolValue("Server/AutoReconnect=false");
     server.plaintext = getBoolValue("Server/Plaintext=false");
     server.lastupdate = getStrValue("Server/LastUpdate=");
     server.password = getPrivStrValue("Server/Password=");
     server.port = getIntValue("Server/Port=5222");
     server.resource = getStrValue("Server/Resource=Gabber");
     server.savepassword = getBoolValue("Server/SavePassword=false");
     server.server = getStrValue("Server/Server=jabber.com");
     server.current_server = server.server; // This is to store the current server, it is not sync'd so we know which server we're connected to
     server.ssl = getBoolValue("Server/SSL=false");
     server.username = getStrValue("Server/Username=");

     speech.usefestival = getBoolValue("Speech/UseFestival=false");

     spelling.check = getBoolValue("Spelling/Check=true");
     spelling.lang  = getStrValue("Spelling/Lang=");

     // Get the home directory, free it (stupid C strings), then tack on .Gabber-spool
//     string gs_fulldir = homedir + "/.Gabber-spool/";
#ifdef MACOSX
     spool.dir = getStrValue("Spool/Dir=~/.Gabber-spool");
#else // MACOSX
     spool.dir = getStrValue("Spool/Dir=~/.Gabber-spool/");
#endif // MACOSX

     status.priority = getIntValue("Status/Priority=9");
     status.save = getBoolValue("Status/Save=true");
     status.show = getIntValue("Status/Show=2");
     status.status = getStrValue("Status/Status=Online");
     status.type = getIntValue("Status/Type=2");

     statusicon.show = getBoolValue("StatusIcon/Show=true");

     toolbars.menubar = getBoolValue("Toolbars/Menubar=true");
     toolbars.presence = getBoolValue("Toolbars/Presence=true");
     toolbars.status = getBoolValue("Toolbars/Status=false");
     toolbars.toolbar = getBoolValue("Toolbars/Toolbar=true");

     user.country = getStrValue("User/Country=");
     user.email = getStrValue("User/Email=");
     user.firstname = getStrValue("User/FirstName=");
     user.fullname = getStrValue("User/FullName=");
     user.lastname = getStrValue("User/LastName=");
     user.nickname = getStrValue("User/Nickname=");
     user.nojud = getBoolValue("User/NoJUD=false");

     window.height = getIntValue("Window/Height=270");
     window.savepos = getBoolValue("Window/SavePos=true");
     window.savesize = getBoolValue("Window/SaveSize=true");
     window.width = getIntValue("Window/Width=185");
     window.x = getIntValue("Window/x=");
     window.y = getIntValue("Window/y=");

     wizards.firsttime = getBoolValue("Wizards/FirstTime=true");
     wizards.welcomehasrun = getBoolValue("Wizards/WelcomeHasRun=false");

     xmms.enabled = getBoolValue("XMMS/Enabled=false");
     xmms.timer = getIntValue("XMMS/Timer=0");
}

void ConfigManager::setProfile(const string& profile)
{
     _CfgProfile = profile;
}

ConfigManager::~ConfigManager()
{
     gnome_config_pop_prefix();
}

const char* ConfigManager::get_PACKAGE()
{
     return GABBER_PACKAGE;
}

const char* ConfigManager::get_VERSION()
{
     return GABBER_VERSION;
}

const char* ConfigManager::get_GLADEDIR()
{
     return GABBER_GLADEDIR;
}

const char* ConfigManager::get_SHAREDIR()
{
     return GABBER_SHAREDIR;
}

const char* ConfigManager::get_CONFIG()
{
     return GABBER_CONFIG;
}

const char* ConfigManager::get_PIXPATH() 
{
     return GABBER_PIXPATH;
}

const string ConfigManager::get_CSSFILE() 
{
     return GABBER_CSSFILE;
}

void ConfigManager::set_nick(const string& nick) 
{
     user.nickname = nick; 
}

void ConfigManager::set_show(jabberoo::Presence::Show show) 
{ 
     status.show = GabberUtil::indexShow(show); 
}

void ConfigManager::set_status(const string& newstatus) 
{ 
     status.status = newstatus; 
}

void ConfigManager::set_priority(int priority) 
{ 
     status.priority = priority; 
}

void ConfigManager::set_type(jabberoo::Presence::Type type) 
{ 
     status.type = GabberUtil::indexType(type); 
}

void ConfigManager::set_server(const string& newserver)
{
     server.server = newserver;
     // If we're not connected then update current server too
     if ((G_Win == NULL) || (G_Win != NULL && !G_Win->is_connected()))
	  server.current_server = newserver;
}

const string ConfigManager::get_nick() const 
{ 
     return user.nickname; 
}

const jabberoo::Presence::Show ConfigManager::get_show() const 
{ 
     return GabberUtil::indexShow(status.show); 
}

const string ConfigManager::get_status() const 
{
     return status.status; 
}

const int ConfigManager::get_priority() const 
{ 
     return status.priority; 
}

const jabberoo::Presence::Type ConfigManager::get_type() const 
{ 
     return GabberUtil::indexType(status.type); 
}

const string ConfigManager::get_server() const 
{
     // If we're connected then report the current server
     if (G_Win != NULL && G_Win->is_connected())
	  return server.current_server; 
     else 
	  return server.server;
}

void ConfigManager::sync()
{
     int i = 0;
     char configpath[64];

     putValue("AutoAway/AwayAfter", autoaway.awayafter);
     putValue("AutoAway/XAAfter", autoaway.xaafter);
     putValue("AutoAway/Enabled", autoaway.enabled);
     putValue("AutoAway/Status", autoaway.status);
     putValue("AutoAway/ChangePriority", autoaway.changepriority);

     putValue("Chats/GroupHeight", chats.groupheight);
     putValue("Chats/GroupTime", chats.grouptime);
     putValue("Chats/GroupWidth", chats.groupwidth);
     putValue("Chats/OOOHeight", chats.oooheight);
     putValue("Chats/OOOTime", chats.oootime);
     putValue("Chats/OOOWidth", chats.ooowidth);

     putValue("Colors/Icons", colors.icons);
     for (map<string, Gdk_Color>::iterator it = colors.presence_colors.begin(); it != colors.presence_colors.end(); it++)
     {
	  char colorhex[8];
	  g_snprintf(colorhex, 8, "#%02x%02x%02x", it->second.get_red() >> 8, 
		     it->second.get_green() >> 8, it->second.get_blue() >> 8);
	  putValue(string("Colors/" + it->first).c_str(), colorhex);
     }

     for (map<string, Gdk_Color>::iterator it = colors.event_colors.begin(); it != colors.event_colors.end(); it++)
     {
          gchar colorhex[8];
          g_snprintf(colorhex, 8, "#%02x%02x%02x", it->second.get_red() >> 8, 
                     it->second.get_green() >> 8, it->second.get_blue() >> 8);
	  putValue(string("Colors/" + it->first).c_str(), colorhex);
     }

     putValue("Dates/ChatFormat", dates.chatformat);
     putValue("Dates/Format", dates.format);
     putValue("Dates/GCFormat", dates.gcformat);

     putValue("Docklet/Show", docklet.show);

     putValue("FileTransfer/Port", filetransfer.port);

     putValue("GPG/Enabled", gpg.enabled);
     putValue("GPG/keyserver", gpg.keyserver);
     putValue("GPG/SecretKeyID", gpg.secretkeyid);
     i = 0;
     for (map<string, string>::iterator it = gpg.keymap.begin(); it != gpg.keymap.end(); it++)
     {
          g_snprintf(configpath, 32, "GPG/keymap-%d", i);
	  putValue(configpath, it->first + "/" + it->second);
	  i++;
     }

     putValue("Headlines/DisplayTicker", headlines.displayticker);

     putValue("IgnoreList/OutsideContact", ignorelist.outsidecontact);
     i = 0;
     for (list<string>::iterator it = ignorelist.ignore.begin(); it != ignorelist.ignore.end(); it++, i++)
     {
	  g_snprintf(configpath, 32, "IgnoreList/ignore-%d", i);
	  putValue(configpath, *it);
     }

     putValue("Logs/Dir", logs.dir);
     putValue("Logs/Encrypted", logs.encrypted);
     putValue("Logs/Groupchat", logs.groupchat);
     putValue("Logs/HTML", logs.html);
     putValue("Logs/None", logs.none);
     putValue("Logs/Purge", logs.purge);
     putValue("Logs/Save", logs.save);
     putValue("Logs/XML", logs.xml);
     putValue("Logs/LastMovedMonth", logs.lastMovedMonth);
     putValue("Logs/LastMovedYear", logs.lastMovedYear);

     putValue("Messages/Height", msgs.height);
     putValue("Messages/OpenMsgs", msgs.openmsgs);
     putValue("Messages/Raise", msgs.raise);
     putValue("Messages/RecvMsgs", msgs.recvmsgs);
     putValue("Messages/RecvNon", msgs.recvnon);

     putValue("Messages/RecvNon", msgs.recvnon);
     putValue("Messages/RecvOOOChats", msgs.recvooochats);
     putValue("Messages/SendMsgs", msgs.sendmsgs);
     putValue("Messages/SendOOOChats", msgs.sendooochats);
     putValue("Messages/ShowAdvancedOpts", msgs.showadvancedopts);
     putValue("Messages/Width", msgs.width);

     putValue("Profiles/Defaultname", profiles.defaultname);

     putValue("Proxy/HTTPType", proxy.http_type);
     putPrivValue("Proxy/Password", proxy.password);
     putValue("Proxy/Port", proxy.port);
     putValue("Proxy/Server", proxy.server);
     putValue("Proxy/Type", proxy.type);
     putValue("Proxy/Username", proxy.username);

     putValue("Roster/HideAgents", roster.hideagents);
     putValue("Roster/HideOffline", roster.hideoffline);
     i = 0;
     for(list<string>::iterator it = roster.collapsed_groups.begin(); it != roster.collapsed_groups.end(); it++, i++)
     {
          g_snprintf(configpath, 32, "Roster/clpsd_grp-%d", i);
          putValue(configpath, *it);
     }

     putValue("Server/AutoLogin", server.autologin);
     putValue("Server/AutoReconnect", server.autoreconnect);
     putValue("Server/Plaintext", server.plaintext);
     putValue("Server/LastUpdate", server.lastupdate);
     if (server.savepassword)
	  putPrivValue("Server/Password", server.password);
     else
	  putPrivValue("Server/Password", "");
     putValue("Server/Port", server.port);
     putValue("Server/Resource", server.resource);
     putValue("Server/SavePassword", server.savepassword);
     putValue("Server/Server", server.server);
     putValue("Server/SSL", server.ssl);
     putValue("Server/Username", server.username);

     putValue("Spelling/Check", spelling.check);
     putValue("Spelling/Lang", spelling.lang);

     putValue("Speech/UseFestival", speech.usefestival);

     putValue("Spool/Dir", spool.dir);

     putValue("Status/Priority", status.priority);
     putValue("Status/Save", status.save);
     putValue("Status/Show", status.show);
     putValue("Status/Status", status.status);
     putValue("Status/Type", status.type);

     putValue("StatusIcon/Show", statusicon.show);

     putValue("Toolbars/Menubar", toolbars.menubar);
     putValue("Toolbars/Presence", toolbars.presence);
     putValue("Toolbars/Status", toolbars.status);
     putValue("Toolbars/Toolbar", toolbars.toolbar);

     putValue("User/Country", user.country);
     putValue("User/Email", user.email);
     putValue("User/FirstName", user.firstname);
     putValue("User/FullName", user.fullname);
     putValue("User/LastName", user.lastname);
     putValue("User/Nickname", user.nickname);
     putValue("User/NoJUD", user.nojud);

     putValue("Window/Height", window.height);
     putValue("Window/SavePos", window.savepos);
     putValue("Window/SaveSize", window.savesize);
     putValue("Window/Width", window.width);
     putValue("Window/x", window.x);
     putValue("Window/y", window.y);

     putValue("Wizards/FirstTime", wizards.firsttime);
     putValue("Wizards/WelcomeHasRun", wizards.welcomehasrun);

     putValue("XMMS/Enabled", xmms.enabled);
     putValue("XMMS/Timer", xmms.timer);

     gnome_config_sync();
}

// Value setters
void ConfigManager::putValue(const char* name, bool value, bool useprofile)
{
//    if (useprofile)
//	  gnome_config_set_bool(string(_CfgProfile + name).c_str(), value);
//     else
	  gnome_config_set_bool(name, value);
}

void ConfigManager::putValue(const char* name, int value, bool useprofile)
{
//     if (useprofile)
//	  gnome_config_set_int(string(_CfgProfile + name).c_str(), value);
//     else
	  gnome_config_set_int(name, value);
}

void ConfigManager::putValue(const char* name, float value, bool useprofile)
{     
//     if (useprofile)
//	  gnome_config_set_float(string(_CfgProfile + name).c_str(), value);
//     else
	  gnome_config_set_float(name, value);
}

void ConfigManager::putValue(const char* name, const char* value, bool useprofile)
{
//     if (useprofile)
//	  gnome_config_set_string(string(_CfgProfile + name).c_str(), value);
//    else
	  gnome_config_set_string(name, value);
}

void ConfigManager::putPrivValue(const char* name, const char* value, bool useprofile)
{
//     if (useprofile)
//	  gnome_config_private_set_string(string(_CfgProfile + name).c_str(), value);
//     else
	  gnome_config_private_set_string(name, value);
}

void ConfigManager::saveEntryHistory(Gnome::Entry* gentry)
{
     gentry->prepend_history(1, static_cast<Gtk::Entry*>(gentry->gtk_entry())->get_text());
     gentry->save_history();
}

// Value retrievers
string ConfigManager::getStrValue(const char* name, bool useprofile)
{
//     if (useprofile)
//	  return string(gnome_config_get_string(string(_CfgProfile + name).c_str()));
//     else
     char* cval = gnome_config_get_string(name);
     string value(cval);
     g_free(cval);

     return value;
}

string ConfigManager::getPrivStrValue(const char* name, bool useprofile)
{
//     if (useprofile)
//	  return string(gnome_config_private_get_string(string(_CfgProfile + name).c_str()));
//     else
     char* cval = gnome_config_private_get_string(name);
     string value(cval);
     g_free(cval);

     return value;
}

bool ConfigManager::getBoolValue(const char* name, bool useprofile)
{
//     if (useprofile)
//	  return gnome_config_get_bool(string(_CfgProfile + name).c_str());
//     else
	  return gnome_config_get_bool(name);
}

int ConfigManager::getIntValue(const char* name, bool useprofile)
{
//     if (useprofile)
//	  return gnome_config_get_int(string(_CfgProfile + name).c_str());
//     else
	  return gnome_config_get_int(name);
}

float ConfigManager::getFloatValue(const char* name, bool useprofile)
{
//     if (useprofile)
//	  return gnome_config_get_float(string(_CfgProfile + name).c_str());
//     else
	  return gnome_config_get_float(name);
}

void ConfigManager::loadEntryHistory(Gnome::Entry* gentry)
{
     gentry->set_max_saved(10); // Trying to get the thing to save more entries
     gentry->load_history();
}

// Presence pixmap/color cache
ConfigManager::PresenceInfo::PresenceInfo(const string& status, const Gdk_Drawable& window, ConfigManager& cfgm)
{
     _color = cfgm.colors.presence_colors.find(status)->second;

     // Only attempt to load the pixmap if status != unavailable
     if (status != "offline")
     {
	  // Build a table of possible image locations
	  const char* pixpath_tbl[4] = { ConfigManager::get_PIXPATH(), "./", "./pixmaps/", "../pixmaps/" };
	  // Look for and load the pixmap for the specified status...if it exists
	  for (int i = 0; i < 4; i++)
	  {
	       string filename = pixpath_tbl[i] + status + ".xpm";
	       // If we find a pixmap by this name, load it and return
	       if (g_file_exists(filename.c_str()))
	       {
		    _pixmap.create_from_xpm(window, _bitmap, Gdk_Color("white"), filename);
		    return;
	       }
	  }
	  // If this point is reached, no suitable file was found
	  cerr << "AHAGH! Missing pixmap: " << status << endl;
     }
}

void ConfigManager::initPresenceCache(const Gdk_Drawable& window)
{
     // Step 1: Make sure the PI cache is clear
     _PICache.clear();

     // Step 2: Create a table of valid presence status & initial colors (HACK)
     const int   ptable_sz = 7;
     const string ptable[ptable_sz] = { "online"   , "chat"   , "away"   , "xa"     , "dnd"    , "offline", "stalker" };


     // Step 3: Load the cache by walking the presence status table
     for (int i = 0; i < ptable_sz; i++)
	  _PICache.insert(make_pair(ptable[i], PresenceInfo(ptable[i], window, *this)));
}

GdkPixmap* ConfigManager::getPresencePixmap(const string& status)
{
     map<string, PresenceInfo>::iterator it = _PICache.find(status);
     if (it != _PICache.end())
	  return it->second._pixmap.gdkobj();
     else
	  return NULL;
}

GdkBitmap* ConfigManager::getPresenceBitmap(const string& status)
{
     map<string, PresenceInfo>::iterator it = _PICache.find(status);
     if (it != _PICache.end())
	  return it->second._bitmap;
     else
	  return NULL;
}

GdkColor* ConfigManager::getPresenceColor(const string& status)
{
     map<string, PresenceInfo>::iterator it = _PICache.find(status);
     if (it != _PICache.end())
	  return it->second._color.gdkobj();
     else
	  return NULL;
}

