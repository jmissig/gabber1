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

#ifndef INCL_CONFIG_MANAGER_HH
#define INCL_CONFIG_MANAGER_HH

#include "jabberoo.hh"

#include <gdk/gdk.h>
#include <gdk--/drawable.h>
#include <gnome--/entry.h>

using namespace std;

class ConfigManager
{
public:
     ConfigManager(const char* name);
     void setProfile(const string& profile);
     ~ConfigManager();

public:
     enum Proxy {
	  proxyNone, proxySOCKS4, proxySOCKS5, proxyHTTP
     };

     struct {
	  int    awayafter;
	  int    xaafter;
	  bool   enabled;
	  string status;
	  bool   changepriority;
     } autoaway;

     struct {
	  int    groupheight;
	  bool   grouptime;
	  int    groupwidth;
	  int    oooheight;
	  bool   oootime;
	  int    ooowidth;
     } chats;

     struct {
	  bool   icons;
	  map<string, Gdk_Color> presence_colors;
	  map<string, Gdk_Color> event_colors;
     } colors;

     struct {
	  string chatformat;
	  string format;
	  string gcformat;
     } dates;

     struct {
	  int    show;
     } docklet;

     struct {
	  int    port;
     } filetransfer;

     struct {
	  bool   enabled;
	  string keyserver;
	  string secretkeyid;
	  map<string, string> keymap;
     } gpg;

     struct {
	  bool   displayticker;
     } headlines;

     struct {
	  bool   outsidecontact;
	  list<string> ignore;
     } ignorelist;

     struct {
	  string dir;
	  // is this option different than the one above, need to check on that
	  bool   encrypted;
	  bool   groupchat;
	  bool   html;
	  bool   none;
	  bool   purge;
	  bool   save;
	  bool   xml;
	  int lastMovedMonth;
	  int lastMovedYear;
     } logs;

     struct {
	  int    height;
	  bool   openmsgs;
	  bool   raise;
	  bool   recvmsgs;
	  bool   recvnon;
	  bool   recvooochats;
	  bool   sendmsgs;
	  bool   sendooochats;
	  bool   showadvancedopts;
	  int    width;
     } msgs;

     struct {
	  string defaultname;
     } profiles;

     struct {
	  string http_type;
	  string password;
	  int    port;
	  string server;
	  int    type;
	  string username;
     } proxy;

     struct {
	  list<string> collapsed_groups;
	  bool   hideagents;
	  bool   hideoffline;
     } roster;

     struct {
	  bool   autologin;
	  bool   autoreconnect;
	  bool   plaintext;
	  string lastupdate;
	  string password;
	  int    port;
	  string resource;
	  bool   savepassword;
	  string server;
	  string current_server;
	  bool   ssl;
	  string username;
     } server;

     struct {
	  int    usefestival;
     } speech;

     struct {
	  bool   check;
	  string lang;
     } spelling;

     struct {
	  string dir;
     } spool;

     struct {
	  int    priority;
	  bool   save;
	  int    show;
	  string status;
	  int    type;
     } status;

     struct {
          bool   show;
     } statusicon;

     struct {
	  bool   menubar;
	  bool   presence;
	  bool   status;
	  bool   toolbar;
     } toolbars;

     struct {
	  string country;
	  string email;
	  string firstname;
	  string fullname;
	  string lastname;
	  string nickname;
	  bool   nojud;
     } user;

     struct {
	  int    height;
	  bool   savepos;
	  bool   savesize;
	  int    width;
	  int    x;
	  int    y;
     } window;

     struct {
	  bool   firsttime;
	  bool   loggedin;
	  bool   welcomehasrun;
     } wizards;

     struct {
	  bool   enabled;
	  int    timer;
     } xmms;

public:
     // Hard-coded value interfaces
     static const char*  get_PACKAGE();
     static const char*  get_VERSION();
     static const char*  get_GLADEDIR();
     static const char*  get_SHAREDIR();
     static const char*  get_CONFIG();
     static const char*  get_PIXPATH();
     static const string get_CSSFILE();
     // Sync
     void sync();
private:
     // Value setters
     void putValue(const char* name, bool value, bool useprofile=true);
     void putValue(const char* name, const char* value, bool useprofile=true);
     void putPrivValue(const char* name, const char* value, bool useprofile=true);
     void putValue(const char* name, string value, bool useprofile=true) { putValue(name, value.c_str()); }
     void putPrivValue(const char* name, string value, bool useprofile=true) { putPrivValue(name, value.c_str()); }
     void putValue(const char* name, int value, bool useprofile=true);
     void putValue(const char* name, float value, bool useprofile=true);
     // Value retrievers
     string      getStrValue(const char* name, bool useprofile=true);
     string      getStrValue(const string& name, bool useprofile=true) { return getStrValue(name.c_str()); };
     string      getPrivStrValue(const char* name, bool useprofile=true);
     const char* getCStrValue(const char* name, bool useprofile=true) { return getStrValue(name).c_str(); }
     bool        getBoolValue(const char* name, bool useprofile=true);
     int         getIntValue(const char* name, bool useprofile=true);
     float       getFloatValue(const char* name, bool useprofile=true);
public:
     void        saveEntryHistory(Gnome::Entry* gentry);
     void        loadEntryHistory(Gnome::Entry* gentry);
     // Soft-coded value interfaces
     void set_nick(const string& nick);
     void set_show(jabberoo::Presence::Show show);
     void set_status(const string& newstatus);
     void set_priority(int priority);
     void set_type(jabberoo::Presence::Type type);
     void set_server(const string& newserver);
     const string get_nick() const;
     const jabberoo::Presence::Show get_show() const;
     const string get_status() const;
     const int    get_priority() const;
     const jabberoo::Presence::Type get_type() const;
     const string get_server() const;
     // Presence pixmap/color cache
     void        initPresenceCache(const Gdk_Drawable& window);
     GdkPixmap*  getPresencePixmap(const string& status);
     GdkBitmap*  getPresenceBitmap(const string& status);
     GdkColor*   getPresenceColor(const string& status);
private:
     string              _CfgName;
     string              _CfgProfile;

     // Presence pixmap/color cache structures
     struct PresenceInfo {
	  Gdk_Color  _color;
	  Gdk_Pixmap _pixmap;
          Gdk_Bitmap _bitmap;
	  PresenceInfo(const string& status, const Gdk_Drawable& window, ConfigManager& cfgm);
     };
     map<string, PresenceInfo> _PICache; // Presence info cache
};

#endif
