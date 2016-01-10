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

#include "MessageManager.hh"

#include "ContactInterface.hh"
#include "GabberApp.hh"
#include "GabberLogger.hh"
#include "GabberUtility.hh"
#include "GabberWin.hh"
#include "GCInterface.hh"
#include "MessageViews.hh"
#include "EventManager.hh"

#include <fcntl.h>
#include <string>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <libgnome/gnome-util.h>

using namespace jabberoo;
using namespace GabberUtil;

const string MessageManager::XML_FOOTER = "</spool>";
MessageManager::MessageType MessageManager::MessageViewInfo::_typeCounter = 1;
const MessageManager::MessageType MessageManager::error_mtype = 0;
map<MessageManager::MessageType, MessageManager::MessageViewInfo*> MessageManager::_message_view_info;

MessageManager::MessageManager(const string& spooldir)
     : _spool(spooldir)
{
     // Make sure message type 0 is for Error/Unknown
     MessageViewInfo *mvi = new MessageViewInfo("error", (MessageViewInfo::ViewInfoFlags) 0, 
						(MessageViewInfo::NewViewMsgFunc) NULL, 
						(MessageViewInfo::NewViewJidFunc) NULL, 
						"glade-blank.xpm", "#000000");
     mvi->mtype = 0;
     MessageViewInfo::_typeCounter = 1;
     _message_view_info[0] = mvi;

     // call message view init functions so they can register themselves as views
     MessageRecvView::init_view(*this);
     ChatMessageView::init_view(*this);
     GCMessageView::init_view(*this);
     ContactRecvView::init_view(*this);
     GCIRecvView::init_view(*this);
     AutoupdateDlg::init_view(*this);
     MessageRecvView::init_oob_view(*this); // Need a separate init because of type counter

     initdir();
     
     // G_Win will run load_spool() when it starts
}

MessageManager::~MessageManager()
{
     map<MessageType, MSpoolMap>::iterator it = _mspools.begin();

     for ( ; it != _mspools.end(); ++it)
     {
          MSpoolMap::iterator mit = it->second.begin();

          // clean up the ofstreams
          while (mit != it->second.end())
          {
	       mit->second->close();
               delete mit->second;
	       ++mit;
          }
     }

}

void MessageManager::load_spool()
{
     // Check spool dir for existing messages; if there are
     // message, load them up
     DIR* spool = opendir(_spool.c_str()); 
     if (spool != NULL)
     {
	  struct dirent* entry = readdir(spool);
	  while (entry != NULL)
	  {
               string filename = entry->d_name;
               if ((filename != ".") && (filename != ".."))
               {
	            string fullname = g_concat_dir_and_file(_spool.c_str(), entry->d_name);
	            MessageLoader(fullname, *this);
               }
	       entry = readdir(spool);
	  }
	  closedir(spool);
     }
}

void MessageManager::add(const jabberoo::Message& m, bool spool)
{
     ConfigManager& cfgm = G_App->getCfg();
     // Figure out if the user has selected to only received OOOChats Or Messages
     MessageType mtype = translateType(m);
     string user;
     bool popup = false;

     if (_message_view_info[mtype]->flags & MessageViewInfo::vfPerResource)
	  user = m.getFrom();
     else
	  user = JID::getUserHost(m.getFrom());

     // if the mtype is error and the body is empty then drop the message
     if (mtype == error_mtype && m.getBody().empty())
	  return;
     else if (mtype == error_mtype)
     {
	      mtype = translateType("normal"); // It was passed on to message manager, so it's probably a normal message
     }
     else if (cfgm.msgs.recvmsgs && (mtype == translateType("chat")))
          mtype = translateType("normal");
     else if (cfgm.msgs.recvooochats && (mtype == translateType("normal")))
          mtype = translateType("chat");

     //XXX: This code ripped straight from GabberWidgets ... not good :(
     //XXX: it's used for message_received below
     // Lookup nickname - default is the username
     string jid = m.getFrom();
     string resource = JID::getResource(jid);
     string nickname = JID::getUser(jid);
     try {
	  nickname = G_App->getSession().roster()[JID::getUserHost(jid)].getNickname();
     } catch (Roster::XCP_InvalidJID& e) {
	  // Special handling for a groupchat ID -- use the resource as the nickname
	  if (G_App->isGroupChatID(jid) && !resource.empty())
	       nickname = resource;
     }

     // Select a view based on the message type
     ViewMap& vm = _mviews[mtype];

     // Attempt to locate a view for this JID
     ViewMap::iterator it = vm.find(user);
     // If a view is found... and it doesn't want messages to be queued
     if (it != vm.end() && !(_message_view_info[mtype]->flags & MessageViewInfo::vfQueueMessage))
     {
	  // Call the message received event
	  G_App->getEventManager().message_received(m, nickname, mtype, false);

	  // Pass the message on to the view
	  it->second->render(m);
     }
     // No view was found, so stick this message
     // in a message list
     else
     {
	  // check to see if message should be popped up
	  if (cfgm.msgs.openmsgs)
	       popup = true;
	  // if the view type asks for popup then popup and don't spool
	  else if (_message_view_info[mtype]->flags & MessageViewInfo::vfPopup)
	       popup = true;

	  // Or a view was found and we just want this queued
	  if (it != vm.end()
	      // Wanting this queued is implied from the other if statement which checks
	      // for it != vm.end()
	      // && (_message_view_info[mtype]->flags & MessageViewInfo::vfQueueMessage)
	       )
	  {
	       // So we notify the view that we have messages waiting now
	       it->second->has_messages_waiting(true);

	       // And we do not want this to automatically popup
	       popup = false;
	  }

	  // Lookup the message list for this JID and Message Type
	  MList& mlist = _mlistmap[mtype][user];
	  // Add the message
	  mlist.push_back(m);
	  // If this message should be spooled, do so
	  // If the message is being displayed, don't spool
	  if (spool && !popup)
	  {
               streampos pos;
	       MSpoolMap::iterator it = _mspools[mtype].find(user);

	       if (it == _mspools[mtype].end())
           {
                string file = user + "-" + translateType(mtype);
                if (file.find("/") != string::npos)
                    file.replace(file.find("/"), 1, "-");
                char *filename = g_concat_dir_and_file(_spool.c_str(), file.c_str());
		if (!g_file_exists(filename))
		{
		     open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		}
                fstream* spool = new fstream(filename, ios::out | ios::ate);
                pair<MSpoolMap::iterator, bool> np = _mspools[mtype].insert(make_pair(user, spool));
                it = np.first;

                spool->seekp(0, ios::end);
                // If file is empty output proper XML header
                if (spool->tellp() == (streampos) 0)
                {
                     *spool << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
                     *spool << "<spool xmlns=\"http://gabber.sourceforge.net/namespace/spool\">" << endl;
                     *spool << XML_FOOTER;
                }
           }
	       Message logm(m);

	       // If the message has no timestamp add one so the correct time is displayed
	       Element* x = logm.findX("jabber:x:delay");
	       if (!x)
	       {
               time_t curtime = time(0);
               struct tm* t = gmtime(&curtime);
               t->tm_year += 1900;
               t->tm_mon += 1;
               char* time = g_strdup_printf("%04d%02d%02dT%02d:%02d:%02d", t->tm_year, t->tm_mon, t->tm_mday,
		 	       		         t->tm_hour, t->tm_min, t->tm_sec);

               Element* rx = logm.getBaseElement().addElement("x");
               rx->putAttrib("xmlns", "jabber:x:delay");
               rx->putAttrib("stamp", time);

               g_free(time);
	       }
            
           it->second->seekp( - (int)XML_FOOTER.size(), ios::end);
	       *it->second << logm.toString() << endl;
           *it->second << XML_FOOTER;
           it->second->flush();
	  }

	  if (popup)
	  {
	       // Call the message received event
	       G_App->getEventManager().message_received(m, nickname, mtype, false);

	       // Display the message
	       display(m.getFrom(), mtype);
	  }
	  else
	  {
	       // Call the message received event
	       G_App->getEventManager().message_received(m, nickname, mtype, true);

	       if (!(_message_view_info[mtype]->flags & MessageViewInfo::vfNoFlash))
	       {
            cout << "adding event " << mtype << " for " << user << endl;
		    _events.push_back(make_pair(user, mtype));
		    // The G_Win may not exist if this message is being
		    // loaded from a spool file.  In that case, the
		    // stuff that's done in roster_event is done elsewhere.  
		    if (G_Win)
			 G_Win->roster_event(user, mtype);
	       }

	       // Raise the main window if they so desire but only if the window exists.  
	       if (G_Win && G_App->getCfg().msgs.raise)
		    G_Win->raise();
	  }
     }
}

MessageView* MessageManager::display(const string jid, const MessageType type)
{
     bool view_displayed = false;
     MessageView *view = NULL;
     string user;

     if (_message_view_info[type]->flags & MessageViewInfo::vfPerResource)
	  user = jid;
     else
	  user = JID::getUserHost(jid);

     // Locate the message list for this JID
     MListMap::iterator it = _mlistmap[type].find(user);
     // If we found a message list
     if (it != _mlistmap[type].end())
     {
	  // walk the list, rendering each message
	  if ((_message_view_info[get_render_type(type)]->flags & MessageViewInfo::vfMultiMessage))
	  {
	       MList& mlist = it->second;
	       for (MList::iterator mit = mlist.begin(); mit != mlist.end(); ++mit)
		    view = render(*mit);
	       
	       if (mlist.begin() != mlist.end())
		    view_displayed = true;
	       
	       _mlistmap[type].erase(it);

	       clearSpool(jid, type);
	       clearEvents(jid, type);
	  }
	  // or just render one if the view doesn't support multimessage
	  else
	  {
	       MList& mlist = it->second;
	       view = render(*(mlist.begin())); // render the message

	       if (mlist.begin() != mlist.end())
		    view_displayed = true;
	       
	       mlist.erase(mlist.begin());

	       bool msgswaiting = true;
	       // If there are no more messages, erase the message list
	       if (mlist.begin() == mlist.end())
	       {
		    _mlistmap[type].erase(it);
		    msgswaiting = false;
	       }

	       // Notify the view if there are more messages are waiting

		    // All this code should probably go in its own function somewhere, 
		    // render(Message m) uses it too
		    {
			 // Get a view map for the message type we are using to render the message
			 ViewMap& vm = _mviews[get_render_type(type)];
			 string user;
			 
			 if (_message_view_info[get_render_type(type)]->flags & MessageViewInfo::vfPerResource)
			      user = jid;
			 else
			      user = JID::getUserHost(jid);
			 
			 // Attempt to locate an existing view..
			 ViewMap::iterator it = vm.find(user);
			 
			 // If an existing view was found, tell it there are more messages where that came from
			 if (it != vm.end())
			 {
			      view = it->second;
			      it->second->has_messages_waiting(msgswaiting);
			 }
		    }

	       clearSpool(jid, type); // FIXME: implement a popSpool
	       popEvent(jid, type);
	  }
     }

     // make sure a View of the requested type is displayed. 
     if (!view_displayed)
          view = render(jid, type);
     if (view)
	     view->raise();
     return view;
}

MessageView* MessageManager::render(const string& jid, const MessageType type)
{
     ViewMap& vm = _mviews[type];
     string user;
     MessageView* view = NULL;

     if (_message_view_info[type]->flags & MessageViewInfo::vfPerResource)
	  user = jid;
     else
	  user = JID::getUserHost(jid);

     ViewMap::iterator it = vm.find(user);
     if (it == vm.end())
     {
	  if (_message_view_info[type]->new_view_jid)
	       view = _message_view_info[type]->new_view_jid(jid, vm);
     }
     else
     {
	  view = it->second;
     }

     return view;
}

MessageView* MessageManager::render(const jabberoo::Message& m)
{
     MessageView *view = NULL;
     // Figure out if the user has selected to only received OOOChats Or Messages
     MessageType render_type = get_render_type(translateType(m));

     // Get a view map for the message type we are using to render the message
     ViewMap& vm = _mviews[render_type];
     string user;

     if (_message_view_info[render_type]->flags & MessageViewInfo::vfPerResource)
	  user = m.getFrom();
     else
	  user = JID::getUserHost(m.getFrom());

     // Attempt to locate an existing view..
     ViewMap::iterator it = vm.find(user);

     // If an existing view was found, use it to render the message...
     // But only use it if the view type can render another message
     if (it != vm.end() && ((_message_view_info[render_type]->flags & MessageViewInfo::vfMultiMessage) ||
	                    (_message_view_info[render_type]->flags & MessageViewInfo::vfQueueMessage)))
     {
	  view = it->second;
	  it->second->render(m);
     }
     else
     // If no view was found, create a new based on the message type
     // - Also, the MessageView is given a reference to the ViewMap
     // - so that it can register it self with it and unregister itself
     // - on destruction
     {
	  if (_message_view_info[render_type]->new_view_msg)
	       view = _message_view_info[render_type]->new_view_msg(m, vm);
     }
     return view;
}

bool MessageManager::hasView(const string& jid, MessageType type)
{
     ViewMap& vm = _mviews[type];
     string user;

     if (_message_view_info[type]->flags & MessageViewInfo::vfPerResource)
	  user = jid;
     else
	  user = JID::getUserHost(jid);

     return (vm.find(user) != vm.end());
}

void MessageManager::initdir()
{
     // Ensure _logdir is terminated with a /
     if (_spool[_spool.length()-1] != '/')
          _spool += "/";
     // Replace ~ with $HOME
     if (_spool[0] == '~')
          _spool.replace(0, 1, g_get_home_dir());
     // See if directory already exists..
     if (!g_file_test(_spool.c_str(), G_FILE_TEST_ISDIR) &&
         (mkdir(_spool.c_str(), 0700) == -1))
     {
          g_error(("Unable to create logging directory: " + _spool).c_str());
          G_App->quit();
     }
}

void MessageManager::clearSpool(const string& jid, MessageType type)
{
     if (jid.empty())
	  return;

     string user;
     if (_message_view_info[type]->flags & MessageViewInfo::vfPerResource)
	  user = jid;
     else
	  user = JID::getUserHost(jid);

     string fileend = user + "-" + translateType(type);
     if (fileend.find("/") != string::npos)
	  fileend.replace(fileend.find("/"), 1, "-");
     string filename = _spool + fileend;
     unlink(filename.c_str());
     // Remove file from the spool map so it is properly re-created if
     // a new message comes in from the user
     MSpoolMap::iterator it = _mspools[type].find(user);
     if (it != _mspools[type].end())
     {
          delete it->second;
          _mspools[type].erase(it);
     }
}

void MessageManager::clearEvents(const string& jid, MessageType type)
{
     if (jid.empty())
	  return;

     string user;
     if (_message_view_info[type]->flags & MessageViewInfo::vfPerResource)
          user = jid;
     else
          user = JID::getUserHost(jid);

     EventList::iterator it = _events.begin();
     for ( ; it != _events.end(); )
     {
          if (it->first == user && it->second == type)
               _events.erase(it++);
	  else
	       ++it;
     }
     // If the user has no more events, clear the events on the roster
     if (getEvent(user) == _events.end())
	  G_Win->clear_event(user);
}

void MessageManager::popEvent(const string& jid, MessageType type)
{
     if (jid.empty())
	  return;

     string user;
     if (_message_view_info[type]->flags & MessageViewInfo::vfPerResource)
          user = jid;
     else
          user = JID::getUserHost(jid);

     EventList::iterator it = _events.begin();
     for ( ; it != _events.end(); )
     {
          if (it->first == user && it->second == type)
	  {
               _events.erase(it);
	       break;
	  }
	  else
	       ++it;
     }
     // If the user has no more events, clear the events on the roster
     if (getEvent(user) == _events.end())
	  G_Win->clear_event(user);
}

// Find the first event in the list (the oldest event) for a user
MessageManager::EventList::iterator MessageManager::getEvent(const string& jid)
{
     EventList::iterator it = _events.begin();
     string user = JID::getUserHost(jid);
     string it_user = "";
     bool offline = !(G_App->getSession().presenceDB().available(jid));
     // The comparison has to be of jid without resource, otherwise if the default
     // resource is different that the one that sent the message or
     int ret;
     while (it != _events.end())
     {
      if (offline)
        it_user = JID::getUserHost(it->first);
      else 
        it_user = it->first;
      if(_message_view_info[it->second]->flags & MessageViewInfo::vfPerResource)
        ret = strcasecmp(it_user.c_str(), jid.c_str());
      else
        ret = strcasecmp(it_user.c_str(), user.c_str());
      if (ret == 0)
        return it;
      ++it;
     }

     return _events.end();
}

// Send a message, properly logging it and encrypting or signing it
// on return encryption is the actual level of encryption
bool MessageManager::send_message(Message m, Encryption& encryption)
{
     bool encrypted = false;
     GabberGPG& gpg = G_App->getGPG();
     if (encryption == encSign && gpg.enabled())
     {
	  // sign the message
          string message;
          GPGInterface::Error err;

          if ((err = gpg.sign(GPGInterface::sigDetach, m.getBody(), message)) != GPGInterface::errOK)
          {
	       // Couldn't sign message.  Verify the message should still be
	       // sent
               Gnome::Dialog* d;
               string question = _("An error occurred trying to sign your message.\nSend the message unsigned?");
               d = manage(Gnome::Dialogs::question_modal(question));
	       main_dialog(d);

               gint button = d->run_and_close();
               // They pressed No so don't send the message
               if (button == 1)
                    return false;
	       // no encryption was used
	       encryption = encNone;
          }
          else
          {
               // Add the signature to the message
               Element* x = m.addX("jabber:x:signed");
               x->addCDATA(message.c_str(), message.length());
          }
     }
     else if (encryption == encEncrypt && gpg.enabled())
     {
	  // encrypt the message
	  string message;
	  string keyid;
	  encrypted = true;
	  G_App->getLogger().tryToLog(m.getTo(), m, encrypted);

          try {
               // Lookup the recipient's jid in the keymap to get their keyid
               keyid = gpg.find_jid_key(m.getTo());
               GPGInterface::Error err;

               if ((err = gpg.encrypt(keyid, m.getBody(), message, true)) != GPGInterface::errOK)
               {
		    // couldn't encrypt message.  ask if it should be sent 
		    // unencrypted 
                    Gnome::Dialog* d;
                    string question = _("An error occurred trying to encrypt your message.\nSend the message unencrypted?");
                    d = manage(Gnome::Dialogs::question_modal(question));
		    main_dialog(d);

                    gint button = d->run_and_close();
                    // They pressed No so don't send the message. 
                    if (button == 1)
                         return false;
		    // message us being sent without encryption
		    encryption = encNone;
               }
	       else
	       {
	            // Change the body of the message to indicate the message is encrypted and 
	            // add the encrypted part to the message
                    m.setBody("This message is encrypted.");
                    Element* x = m.addX("jabber:x:encrypted");
                    x->addCDATA(message.c_str(), message.length());
               }
	  } catch(GabberGPG::GPG_InvalidJID& e) {
               cerr << "FIXME: don't have Public Key.  Need a way to get it." << endl;

               Gnome::Dialog* d;
               string question = _("An error occurred trying to encrypt your message.\nSend the message unencrypted?");
               d = manage(Gnome::Dialogs::question_modal(question));
	       main_dialog(d);

               gint button = d->run_and_close();
               // They pressed No so don't send the message
               if (button == 1)
                    return false;
	       // message is being sent without encryption
	       encryption = encNone;
          }
     }
     else
     {
	  // the message wasn't encrypted
	  encryption = encNone;
     }
     if(!encrypted)
	  G_App->getLogger().tryToLog(m.getTo(), m, encrypted);

     // Send the message
     G_App->getSession() << m;

     return true;
}

MessageManager::MessageType MessageManager::translateType(const Message& m)
{
     bool extension;
     string type;
     const Element *x = m.getBaseElement().findElement("x");
     MessageType mtype = 0;

     if (x)
     {
	  type = x->getAttrib("xmlns");
	  extension = true;
     }
     else
     {
	  type = m.getBaseElement().getAttrib("type");
	  extension = false;
     }
     // If it's an extension, try to translate the namespace.  If we don't
     // recognize it, translate the message type
     if (extension)
     {
	  mtype = translateType(type, extension);
	  // if the mtype is error, the x namespace was unrecognized,
	  // otherwise, return the message type.  
	  if (mtype != error_mtype)
	       return mtype;

	  // If the body is empty then there's no point in translating it to a standard
	  // message type so return unknown
	  if (m.getBody().empty())
	       return error_mtype;
	  // unrecognized message typ, change type to message type so the 
	  // translateType later will translate that instead.
	  type = m.getBaseElement().getAttrib("type");
     }
     // pass false for isextension since if we get here we don't want to
     // match an extension even if there is an x in the message.  
     if (type.empty())
     {
       type = "normal";
     }
     return translateType(type, false);
}

MessageManager::MessageType MessageManager::translateType(const string& type, bool isextension)
{
     map<MessageType, MessageViewInfo*>::iterator it = _message_view_info.begin();
     for (; it != _message_view_info.end(); ++it)
     {
	  if (isextension && (it->second->flags & MessageViewInfo::vfExtension))
	  {
               if (strcasecmp(type.c_str(), it->second->stype.c_str()) == 0)
                    return it->second->mtype;
          }
          else if (!isextension && !(it->second->flags & MessageViewInfo::vfExtension))
          {
               if (strcasecmp(type.c_str(), it->second->stype.c_str()) == 0)
                    return it->second->mtype;
          }

     }
     return error_mtype;
}

string MessageManager::translateType(const MessageType mtype)
{
     if (mtype < 0 || mtype >= MessageViewInfo::_typeCounter)
	  return "";

     return _message_view_info[mtype]->stype;
}

MessageManager::MessageType MessageManager::get_render_type(const MessageType type)
{
     ConfigManager& cfgm = G_App->getCfg();
     // Figure out if the user has selected to only received OOOChats Or Messages
     MessageType render_type = type;

     if (render_type == translateType("error"))
	  render_type = translateType("normal"); // It was passed on to message manager, so it's probably a normal message
     if (cfgm.msgs.recvmsgs && (render_type == translateType("chat")) )
          render_type = translateType("normal");
     else if (cfgm.msgs.recvooochats && (render_type == translateType("normal")))
          render_type = translateType("chat");

     return render_type;
}

MessageManager::MessageType MessageManager::register_view_type(const string& stype, enum MessageManager::MessageViewInfo::ViewInfoFlags flags, MessageManager::MessageViewInfo::NewViewMsgFunc new_msg, MessageManager::MessageViewInfo::NewViewJidFunc new_jid, const string& pixmap, const string& color)
{
     MessageViewInfo *mv;

     mv = new MessageViewInfo(stype, flags, new_msg, new_jid, pixmap, color);
     _message_view_info.insert(make_pair(mv->mtype, mv));
     return mv->mtype;
}

MessageManager::MessageViewInfo::MessageViewInfo(const string& type, enum ViewInfoFlags f, NewViewMsgFunc new_msg, NewViewJidFunc new_jid, const string& pixfile, const string& colorstr)
{
     stype = type;
     mtype = _typeCounter++;
     flags = f;
     new_view_msg = new_msg;
     new_view_jid = new_jid;

     color = Gdk_Color(colorstr);
     pixmap_file = pixfile;
     pixmap_loaded = false;
}

void MessageManager::MessageViewInfo::load_pixmap()
{
     if (pixmap_loaded || !G_Win || !G_Win->get_MainWin())
          return;
     
     // Build a table of possible image locations
     const char* pixpath_tbl[5] = { ConfigManager::get_PIXPATH(), ConfigManager::get_SHAREDIR(), "./", "./pixmaps/", "../pixmaps/" };
     // Look for and load the pixmap
     for (int i = 0; i < 5; i++)
     {
          string filename = pixpath_tbl[i] + pixmap_file;
          // If we find a pixmap by this name, load it and return
          if (g_file_exists(filename.c_str()))
          {
               pixmap.create_from_xpm(G_Win->get_MainWin()->get_window(), bitmap, Gdk_Color("white"), filename);
	       break;
          }
     }
     pixmap_loaded = true;
}

GdkColor* MessageManager::getEventColor(MessageType type)
{
     if (type < 0 || type >= MessageViewInfo::_typeCounter)
	  return NULL;

     return _message_view_info[type]->color.gdkobj();
}

GdkPixmap* MessageManager::getEventPixmap(MessageType type)
{
     if (type < 0 || type >= MessageViewInfo::_typeCounter)
          return NULL;

     if (!_message_view_info[type]->pixmap_loaded)
          _message_view_info[type]->load_pixmap();

     return _message_view_info[type]->pixmap.gdkobj();
}

GdkBitmap* MessageManager::getEventBitmap(MessageType type)
{
     if (type < 0 || type >= MessageViewInfo::_typeCounter)
          return NULL;

     if (!_message_view_info[type]->pixmap_loaded)
	  _message_view_info[type]->load_pixmap();

     return _message_view_info[type]->bitmap.gdkobj();
}

// --------------------------------------
//
// MessageLoader
//
// --------------------------------------
MessageLoader::MessageLoader(const string& filename, MessageManager& mm)
     : judo::ElementStream(this),
       _manager(mm)
{
     // Open an istream 
     ifstream spoolfile(filename.c_str());
     if (spoolfile)
     { 
	  // Feed all istream data into the parser
	  char buf[4096];
	  spoolfile.getline(buf, 4096);
	  while (spoolfile.gcount() != 0)
	  {
	       push(buf, strlen(buf));
	       spoolfile.getline(buf, 4096);
	  }
     }
}

MessageLoader::~MessageLoader()
{
}

void MessageLoader::onDocumentStart(Element* e)
{
}

void MessageLoader::onElement(Element* t)
{
     Element& tref = *t;
     // Check for messages
     if (tref.getName() == "message")
     {
	  Message m(tref);
	  _manager.add(m, false);
     }
     delete t;
}

void MessageLoader::onDocumentEnd()
{
}
