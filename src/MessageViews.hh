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

#ifndef INCL_MESSAGE_VIEWS_HH
#define INCL_MESSAGE_VIEWS_HH

#include "jabberoo.hh"

#include "BaseGabberWindow.hh"
#include "GabberGPG.hh"
#include "GabberUtility.hh"
#include "MessageManager.hh"

#include <gtk--/paned.h>
#include <gtk--/tooltips.h>

class PrettyJID;

class MessageView
{
public:
     MessageView(const jabberoo::Message& m, ViewMap& vm, bool userhost = false);
     MessageView(const string& jid, ViewMap& vm, bool userhost = false);
     virtual ~MessageView();
     virtual void render(const jabberoo::Message& m) = 0;
     virtual void has_messages_waiting(bool msgswaiting);
     virtual void raise();
protected:
     string _jid;
private:
     bool   _only_userhost;
     ViewMap& _registrar;
};

namespace MessageUtil
{
     bool gpg_toggle(Gtk::ToggleButton* tb, const string& jid, const string& resource);
     bool gpg_toggle(Gtk::ToggleButton* tb, const jabberoo::Presence& p);
};

typedef list<jabberoo::Message> MessageQueue;

class MessageRecvView;

class MessageSendDlg
     : public BaseGabberWindow
{
public:
     MessageSendDlg(const string& jid);
     MessageSendDlg(const jabberoo::Message& m, MessageRecvView* mrv);
     ~MessageSendDlg();
protected:
     void init();
     // Handlers
     void on_Send_clicked();
     void on_Cancel_clicked();
     gint on_window_delete(GdkEventAny* e);
     gint on_window_key_press(GdkEventKey* e);
     void on_SendTo_changed();
     void on_AddContact_clicked();
     void on_ContactInfo_clicked();
     void on_History_clicked();
     gint on_Event_clicked(GdkEventButton* button);
private:
     MessageRecvView* _msgRecvView;
     string _jid;
     string _nickname;

     string _thread;

     Gtk::Button* _btnAddContact;
     Gtk::Text*   _txtBody;
     PrettyJID*   _pjid;
     Gtk::Entry*  _entSubject;
     bool   _useGPG;
     bool   _onRoster;
};

class MessageRecvView
     : public MessageView, public BaseGabberWindow
{
public:
     // Basic MessageView ops
     MessageRecvView(const jabberoo::Message& m, ViewMap& vm);
     MessageRecvView(const string& jid, ViewMap& vm);
     ~MessageRecvView();
     void render(const jabberoo::Message& m);
     void has_messages_waiting(bool msgswaiting);

     static void init_view(MessageManager& mm);
     static void init_oob_view(MessageManager& mm);
     static MessageView* new_view_msg(const jabberoo::Message& m, ViewMap& vm);
     static MessageView* new_view_jid(const string& jid, ViewMap& vm);
protected:
     void init();
     // Handlers
     void on_Reply_clicked();
     void on_ReadPrev_clicked();
     void on_ReadNext_clicked();
     void on_Close_clicked();
     gint on_window_delete(GdkEventAny* e);
     gint on_window_key_press(GdkEventKey* e);
     void on_AddContact_clicked();
     void on_ContactInfo_clicked();
     void on_History_clicked();
     int  on_GPGInfo_button_press(GdkEventButton* e);
private:
     jabberoo::Message _message;
     Gtk::Button* _btnAddContact;
     Gtk::Button* _btnReadPrev;
     Gtk::Button* _btnReadNext;
     Gtk::Text*   _txtBody;
     Gnome::Pixmap* _pixGPG;
     Gtk::EventBox* _evtGPG;

     bool   _useGPG;
     bool   _onRoster;
     string _nickname;

     MessageQueue       _Messages;
     MessageQueue::iterator _Messages_current;
     bool               _msgswaiting;
     bool               _Messages_should_queue;
     GabberGPG::GPGInfo _gpgInfo;
     Gtk::Tooltips      _tips;
     jabberoo::Message* _composing_msg;
};


class ChatView;

class ChatMessageView
     : public MessageView, public BaseGabberWindow
{
public:
     // Basic MessageView ops
     ChatMessageView(const jabberoo::Message& m, ViewMap& vm);
     ChatMessageView(const string& jid, ViewMap& vm);
     ~ChatMessageView();
     void render(const jabberoo::Message& m);
     void raise();

     static void init_view(MessageManager& mm);
     static MessageView* new_view_msg(const jabberoo::Message& m, ViewMap& vm);
     static MessageView* new_view_jid(const string& jid, ViewMap& vm);
protected:
     void init();
     void get_status();
     // Handlers
     void on_Send_clicked();
     void on_Close_clicked();
     void on_CloseChat_Close_clicked();
     void on_CloseChat_Keep_clicked();
     gint on_new_message_timer();
     gint on_window_delete(GdkEventAny* e);
     void on_memMsg_changed();
     void on_AddUser_clicked();
     void on_EditUser_clicked();
     void on_History_clicked();
     gint on_window_key_press(GdkEventKey* e);
     int  on_GPGInfo_button_press(GdkEventButton* e);
     // Presence handler
     void on_presence(const jabberoo::Presence& p, const jabberoo::Presence::Type prev);
     // Error handler
     void on_error(const string& concerning, int errorcode, const string& errormsg, const string& body);
     void on_Chatview_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time);
private:
     string         _pix_path;
     Gtk::HBox*     _hboxChatview;
     Gtk::Text*     _memMsg;
     Gtk::CheckButton* _chkMessage;
     Gtk::Button*   _btnAddUser;
     Gnome::Pixmap* _pixGPG;
     Gtk::EventBox* _evtGPG;
     Gdk_Pixmap*    _pix_drag_pixmap;
     Gdk_Bitmap     _pix_drag_bitmap;
     BaseGabberWidget* _baseCloseChat;
     Gnome::Dialog* _dlgCloseChat;
     string         _nickname;
     string         _resource;
     string         _subject;
     string         _thread;
     bool           _onRoster;
     GabberGPG::GPGInfo _gpgInfo;
     bool	    _useGPG;
     Gtk::Tooltips  _tips;
     jabberoo::Message* _composing_msg;
     SigC::Connection _composing_event;
     SigC::Connection _new_message_timer;

     // Chat view
     ChatView*    _chatview;
};

class GCMessageView
     : public MessageView, public BaseGabberWindow
{
public:
     // Basic MessageView ops
     GCMessageView(const jabberoo::Message& m, ViewMap& vm);
     GCMessageView(const string& jid, ViewMap& vm);
     ~GCMessageView();
     void render(const jabberoo::Message& m);

     static void join(const string& jid);

     static void init_view(MessageManager& mm);
     static MessageView* new_view_msg(const jabberoo::Message& m, ViewMap& vm);
     static MessageView* new_view_jid(const string& jid, ViewMap& vm);
protected:
     bool process_command(const string& command);
     void init();

     // Events
     void on_session_disconnected();
     void on_session_presence(const jabberoo::Presence& p, jabberoo::Presence::Type prev);
     void on_presence_changed(const jabberoo::Presence& p);
     gint on_Subject_key_press(GdkEventKey* e);
     gint on_Message_key_press(GdkEventKey* e);
     int  on_Roster_button_press(GdkEventButton* e);
     void on_Message_activate();
     void on_OOOChat_activate();
     void on_SendContacts_activate();
     void on_ViewInfo_activate();
     void on_History_clicked();
     void on_Close_clicked();
     void on_Users_toggled();
     jabberoo::Presence::Show translate_show(int show_index);
     void on_Show_selected(int show_index);
     gint on_window_delete(GdkEventAny* e);
     void on_Invite_clicked();
     // Error handler
     void on_error(const string& concerning, int errorcode, const string& errormsg, const string& body);
     void on_drag_data_received(GdkDragContext* drag_ctx, gint x, gint y, GtkSelectionData* data, guint info, guint time);
private:
     string        _room;
     string        _nick;
     string        _oldnick;
     int           _row, _col;
     GCompletion*  _completer;
     jabberoo::Presence::Show _current_show;
     // Widget ptrs
     Gtk::Label*   _lblUserCount;
     Gtk::Text*   _memMessage;
     Gtk::HBox*   _hboxChatview;
     Gtk::Entry*  _entSubject;
     Gtk::CList*  _lstUsers;
     Gtk::ScrolledWindow* _lscroll;
     Gtk::Button* _btnHistory;
     Gtk::Button* _btnClose;
     Gtk::ToggleButton* _tglUsers;
     Gtk::Paned* _hpane;
     int _posHpane;
     Gtk::OptionMenu* _optShow;
     MenuBuilder  _menuShow;
     Gtk::Menu*   _menuGCUser;

     BaseGabberWidget* _baseGCU;
     // Chat view ptr
     ChatView* _chatview;
};

class GCJoinDlg
     : public BaseGabberDialog
{
public:
     static void execute();
     GCJoinDlg();
protected:
     // Handlers
     void on_Server_activate();
     void on_ok_clicked();
     void on_cancel_clicked();
     gint on_key_pressed(GdkEventKey* e);
     void on_protocol_selected(int protocol);
     void changed();
     void on_agents_reply(const Element& iq);
private:
     int _protocol;
     Gtk::OptionMenu* _optProtocol;
     MenuBuilder    _menuProtocol;
     Gtk::Entry*    _entNick;
     Gtk::Entry*    _entRoom;
     Gtk::Entry*    _entServer;
     Gtk::Entry*    _entIRCServer;
     Gnome::Entry*  _gentNick;
     Gnome::Entry*  _gentRoom;
     Gnome::Entry*  _gentServer;
};

class AutoupdateDlg
     : public BaseGabberDialog, public MessageView
{
public:
     AutoupdateDlg(const jabberoo::Message& m, ViewMap& vm);
     void render(const jabberoo::Message& m);

     static MessageView* new_view(const jabberoo::Message& m, ViewMap& vm);
     static void init_view(MessageManager& mm);
     //AutoupdateDlg(const string& version, const string& desc, const string& url);
protected:
     void on_OK_clicked();
     gint on_window_delete(GdkEventAny* e);
     void on_update_reply(const Element& t);

     const string _version;
};

#endif
