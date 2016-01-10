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

#ifndef INCL_MESSAGE_MANAGER_HH
#define INCL_MESSAGE_MANAGER_HH

#include "jabberoo.hh"

#include <fstream>
#include <sigc++/object_slot.h>
#include <gdk--/bitmap.h>
#include <gdk--/color.h>
#include <gdk--/pixmap.h>

class MessageView;
typedef map<string, MessageView*, jabberoo::JID::Compare> ViewMap;

class MessageManager
     : public SigC::Object
{
public:
     MessageManager(const string& spooldir);
     ~MessageManager();
     void load_spool();

     static const unsigned int numTypes = 9;

     enum Encryption
     {
	  encNone, encSign, encEncrypt
     };

     typedef int MessageType;

     typedef pair<string, MessageType> EventPair;
     typedef list<EventPair> EventList;

     struct MessageViewInfo {
	  // string value for type
	  string      stype;

	  // int value for type, assigned by messagemanager
	  MessageType mtype;

	  // View Flags
	  enum ViewInfoFlags {
	       // the stype is a jabber:x extension rather than a <message type=''>
	       // message.  
	       vfExtension    = 1 << 0,
	       // view can handle multiple messages
	       vfMultiMessage = 1 << 1,
	       // One view per resource.  This only has meaning if vfMultiMessage is set as well
	       vfPerResource  = 1 << 2,
	       // don't flash events for this type
	       vfNoFlash      = 1 << 3,
	       // popup the message when it's received, never spool messages for this type
	       vfPopup	      = 1 << 4,
	       // view wants multiple messages to wait, but can display them
	       vfQueueMessage = 1 << 5,
	  } flags;

	  typedef MessageView* (*NewViewMsgFunc)(const jabberoo::Message& m, ViewMap& vm);
	  typedef MessageView* (*NewViewJidFunc)(const string& jid, ViewMap& vm);
	  // create a new messageview of this type and return a pointer to it
	  NewViewMsgFunc new_view_msg;
	  NewViewJidFunc new_view_jid;

	  // Icon to flash for this event
	  bool       pixmap_loaded;
	  string     pixmap_file; 
	  Gdk_Pixmap pixmap;
	  Gdk_Bitmap bitmap;
	  void       load_pixmap();

	  // color to flash for this event
	  Gdk_Color  color;
	  
	  MessageViewInfo(const string&, ViewInfoFlags flags, NewViewMsgFunc new_msg, NewViewJidFunc new_jid, const string& pixmap, const string& color);

	  // a message type of 0 is reserved for unknown message type
	  static MessageType _typeCounter;
     };
     static const MessageType error_mtype;

     // Public operations
     void add(const jabberoo::Message& m, bool spool = true);
     MessageView* display(const string jid, const MessageType type);
     bool hasQueue(const string& jid);
     bool hasView(const string& jid, MessageType type = 0);

     EventList& getEvents() { return _events; }
     EventList::iterator getEvent(const string& jid);

     // send a message
     bool send_message(jabberoo::Message m, Encryption& encryption);

     // Type translations
     static MessageType translateType(const jabberoo::Message& m);
     static string translateType(const MessageType type);
     static MessageType translateType(const string& type, bool xtype=false);
     static MessageType get_render_type(const MessageType type);

     // Register a new view type, the type# is returned
     MessageType register_view_type(const string& stype, enum MessageViewInfo::ViewInfoFlags flags, MessageViewInfo::NewViewMsgFunc new_msg, MessageViewInfo::NewViewJidFunc new_jid, const string& pixmap, const string& color);
     // XXX: Not urgent, might want to add a way to unregister views
     // if a plugin interface or something is implemented in the future.  

     GdkColor* getEventColor(MessageType type);
     GdkPixmap* getEventPixmap(MessageType type);
     GdkBitmap* getEventBitmap(MessageType type);
private:
     // Internal ops
     MessageView* render(const jabberoo::Message& m);
     MessageView* render(const string& jid, MessageType type);
     void initdir();
     void clearSpool(const string& jid, MessageType type);
     void clearEvents(const string& jid, MessageType type);
     void popEvent(const string& jid, MessageType type);

     // Message list structures
     typedef list<jabberoo::Message> MList;
     typedef map<string, MList, jabberoo::JID::Compare> MListMap;
     map<MessageType, MListMap> _mlistmap;

     // Spooling info
     string _spool;

     // Message spooling map
     typedef map<string, fstream*, jabberoo::JID::Compare> MSpoolMap;
     map<MessageType, MSpoolMap> _mspools;

     // Message display structures
     map<MessageType, ViewMap> _mviews;

     // Use a list to keep track of the order that the messages arrived
     EventList _events;

     static const string XML_FOOTER;

     static map<MessageType, MessageViewInfo*> _message_view_info;
};

class MessageLoader
     : private judo::ElementStreamEventListener, private judo::ElementStream
{
public:
     MessageLoader(const string& filename, MessageManager& mm);
     ~MessageLoader();
private:
     MessageManager& _manager;
     virtual void onDocumentStart(Element* e);
     virtual void onElement(Element* t);
     virtual void onDocumentEnd();
};

#endif
