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
 *  Copyright (C) 1999-2003 Dave Smith & Julian Missig
 */

#ifndef INCL_STATUS_INTERFACE_HH
#define INCL_STATUS_INTERFACE_HH

#include "BaseGabberWindow.hh"
#include "GabberWin.hh"

#include <gtk--/spinbutton.h>
#include <gtk--/tooltips.h>

class ShowDlg :
     public BaseGabberDialog
{
public:
     ShowDlg(jabberoo::Presence::Show current_show, const string& current_status, int priority = 0);
private:
     void on_UpdateStatus_clicked();
     void on_Cancel_clicked();
     gint on_key_press_event(GdkEventKey* e);
     void on_txtStatus_changed();
     string _pix_path;
     Gtk::Text*               _txtStatus;
     jabberoo::Presence::Show _curShow;
     Gtk::SpinButton*         _spinPriority;
};

class StatusIndicator
{
public:
     enum ConnectType
     {
	  ctDisconnected, ctConnected, ctConnectedSSL
     };
     
     static void display_online_contacts(int num_online);
     static void display_connection(ConnectType connection);
     static void display_presence_signed(bool is_signed);
     static void display_presence(const jabberoo::Presence& p);
     static Gtk::Window* get_dock_window();

     class StatusIcon
	  : public SigC::Object
     {
     public:
	  StatusIcon(const jabberoo::Presence& p);
	  void update_status(const jabberoo::Presence& p);
	  ~StatusIcon();
	  void on_close();
     protected:
	  gint on_window_delete(GdkEventAny *e);
	  int on_button_press(GdkEventButton* e);
	  gint FlashEvents();
     private:
	  bool _flash_events;
	  bool _need_refresh;
	  string _current_show;
	  string _pix_path;
	  Gtk::Container* _status_icon;
	  Gtk::Frame* _frmQueue;
	  Gdk_Bitmap _icon_mask;
	  Gtk::Tooltips _tips;
     };

     class DockletWin
	  : public SigC::Object
     {
     public:
	  DockletWin(const jabberoo::Presence& p);
	  void update_status(const jabberoo::Presence& p);
	  ~DockletWin();
	  void on_close();
	  Gtk::Window* get_dock_window();
     protected:
	  gint on_window_delete(GdkEventAny *e);
	  int on_button_press(GdkEventButton* e);
	  gint FlashEvents();
     private:
	  bool _flash_events;
	  bool _need_refresh;
	  string _current_show;
	  string _pix_path;
	  Gtk::Window* _status_docklet;
	  Gtk::Frame* _frmQueue;
	  Gdk_Bitmap _icon_mask;
	  Gtk::Tooltips _tips;
     };

     class StatusToolbar
	  : public SigC::Object
     {
     public:
	  StatusToolbar(GabberWin* gwin);
	  ~StatusToolbar();

	  void display_online_contacts(int num_online);
	  void display_connection(ConnectType connection);
	  void display_presence_signed(bool is_signed);
	  void display_presence(const jabberoo::Presence& p);
     protected:
	  int on_button_press(GdkEventButton* e);
	  gint FlashEvents();
     private:
	  GabberWin* _gwin;
	  bool _flash_events;
	  bool _need_refresh;
	  Gtk::EventBox* _evtQueue;
	  Gtk::Frame*    _frmQueue;
	  Gtk::Label*    _lblContacts;
	  Gtk::EventBox* _evtContacts;
	  Gnome::Pixmap* _pixSigned;
	  Gtk::EventBox* _evtSigned;
	  Gnome::Pixmap* _pixConnected;
	  Gtk::EventBox* _evtConnected;
	  Gdk_Bitmap _icon_mask;
	  Gtk::Tooltips _tips;
     };

private:
     static StatusIcon* _statusIcon;
     static DockletWin* _statusDock;
     static StatusToolbar* _statusToolbar;
};

#endif
