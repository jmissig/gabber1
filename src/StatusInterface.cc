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

#include "StatusInterface.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "GabberWin.hh"
#include "ConfigManager.hh"
#include "MessageManager.hh"
#include <gdk/gdkx.h>
#include <X11/Xatom.h>

#include "gtkspell.h"
#include "eggtrayicon.h"

using namespace jabberoo;
using namespace GabberUtil;

// ---------------------------------------------------------
//
// Show Dialog
//
// ---------------------------------------------------------

ShowDlg::ShowDlg(jabberoo::Presence::Show current_show, const string& current_status, int priority)
     : BaseGabberDialog("Status_dlg"),
       _pix_path(ConfigManager::get_PIXPATH()),
       _curShow(current_show)
{
     main_dialog(_thisDialog);

     // Setup buttons
     Gtk::Button* b1 = getButton("Status_UpdateStatus_btn");
     b1->clicked.connect(slot(this, &ShowDlg::on_UpdateStatus_clicked));
     b1 = getButton("Status_Cancel_btn");
     b1->clicked.connect(slot(this, &ShowDlg::on_Cancel_clicked));
     _spinPriority = getWidget<Gtk::SpinButton>("Status_Priority_spin");

     // Enter closes
     _thisWindow->key_press_event.connect(slot(this, &ShowDlg::on_key_press_event));

     // Show pixmap
     string show_pix = _pix_path + getShowFilename(_curShow) + ".xpm";
     getWidget<Gnome::Pixmap>("Status_Show_pix")->load(show_pix);

     // Display the show
     getLabel("Status_Show_txt")->set_text(getShowName(current_show));

     // Setup status text widget
     _txtStatus = getWidget<Gtk::Text>("Status_Status_txt");
     _txtStatus->set_word_wrap(true);
     _txtStatus->set_editable(true);
     _txtStatus->changed.connect(slot(this, &ShowDlg::on_txtStatus_changed));
     if (gtkspell_running())
	  gtkspell_attach(_txtStatus->gtkobj()); // Attach gtkspell for spell checking

     if (current_status.length() > 0)
	  _txtStatus->insert(fromUTF8(_thisWindow, current_status)); // text encoding is local if displaying/inputting, UTF-8 otherwise.
     else
	  _txtStatus->insert(getShowName(current_show));

     // Display the priority
     _spinPriority->set_value(priority);

     show();
}

void ShowDlg::on_UpdateStatus_clicked()
{
     string msg = toUTF8(_txtStatus, _txtStatus->get_chars(0, -1));
     int pri = _spinPriority->get_value_as_int();

     // We want to "show" as offline. Disconnect.
     if (_curShow == Presence::stOffline)
     {
	  G_Win->display_status(_curShow, msg, pri, true, Presence::ptUnavailable);

	  // If the Session isn't connected, disconnect the transmitter, otherwise disconnect session
	  if (!G_App->getSession().disconnect())
	       G_App->getTransmitter().disconnect();
     }
     else
     {
	  // Save the status to the config file
	  G_App->getCfg().set_show(_curShow);
	  G_App->getCfg().set_status(msg);
	  G_App->getCfg().set_priority(pri);

	  G_Win->display_status(_curShow, msg, pri);
     }

     close();
}

void ShowDlg::on_Cancel_clicked()
{
     close();
}

gint ShowDlg::on_key_press_event(GdkEventKey* e)
{
     // If they pressed the Keypad enter, make it act like a normal enter
     if (e->keyval == GDK_KP_Enter)
	  e->keyval = GDK_Return;

     // If Shift-Enter is pressed, insert a newline
     if ( (e->keyval == GDK_Return) && (e->state & GDK_SHIFT_MASK) )
	  e->state ^= GDK_SHIFT_MASK;
     // If enter is pressed, act as if they clicked Update Status
     else if (e->keyval == GDK_Return)
	  on_UpdateStatus_clicked();
     // If they pressed space, run spellcheck
     else if (e->keyval == GDK_space && gtkspell_running())
	  gtkspell_check_all(_txtStatus->gtkobj());

     return 0;
}

void ShowDlg::on_txtStatus_changed()
{
     getButton("Status_UpdateStatus_btn")->set_sensitive(_txtStatus->get_length() != 0);
}

// ---------------------------------------------------------
//
// Status Indicator
//
// ---------------------------------------------------------
StatusIndicator::StatusIcon* StatusIndicator::_statusIcon = NULL;
StatusIndicator::DockletWin* StatusIndicator::_statusDock = NULL;
StatusIndicator::StatusToolbar* StatusIndicator::_statusToolbar = NULL;

void StatusIndicator::display_online_contacts(int num_online)
{
     if (_statusToolbar == NULL)
	  _statusToolbar = manage(new StatusToolbar(G_Win));
     _statusToolbar->display_online_contacts(num_online);
}

void StatusIndicator::display_connection(ConnectType connection)
{
     if (_statusToolbar == NULL)
	  _statusToolbar = manage(new StatusToolbar(G_Win));
     _statusToolbar->display_connection(connection);
}

void StatusIndicator::display_presence_signed(bool is_signed)
{
     if (_statusToolbar == NULL)
	  _statusToolbar = manage(new StatusToolbar(G_Win));
     _statusToolbar->display_presence_signed(is_signed);
}

// This seems to be the most intelligent way to handle presence display
// The rest of the StatusIndicator stuff should probably be converted to this
void StatusIndicator::display_presence(const jabberoo::Presence& p)
{
     // Toolbar
     if (_statusToolbar == NULL)
	  _statusToolbar = manage(new StatusToolbar(G_Win));
     _statusToolbar->display_presence(p);

     // Status Icon
     if (G_App->getCfg().statusicon.show)
     {
	  if (_statusIcon != NULL)
	       _statusIcon->update_status(p);
	  else
	       _statusIcon = manage(new StatusIcon(p));
     }
     else if (_statusIcon != NULL)
     {
	  _statusIcon->on_close();
     }       

     // Docklet
     if (G_App->getCfg().docklet.show)
     {
	  if (_statusDock != NULL)
	       _statusDock->update_status(p);
	  else
	       _statusDock = manage(new DockletWin(p));
     }
     else if (_statusDock != NULL)
     {
	  _statusDock->on_close();
     }  
}

Gtk::Window* StatusIndicator::get_dock_window()
{
     if (_statusDock != NULL)
	  return _statusDock->get_dock_window();
     else
	  return NULL;
}

// ---------------------------------------------------------
// Status Icon (freedesktop.org protocol)
// ---------------------------------------------------------
StatusIndicator::StatusIcon::StatusIcon(const jabberoo::Presence& p)
     : _flash_events(false), _need_refresh(false), 
       _current_show(getShowFilename(p.getShow())), 
       _pix_path(ConfigManager::get_PIXPATH())
{
     if (p.getType() == jabberoo::Presence::ptInvisible)
	  _current_show = "invisible";

     // Freedesktop.org protocol. Woo!!

     // make status docklet a new window with a fixed size
     _status_icon = Gtk::wrap(GTK_CONTAINER(egg_tray_icon_new(_("gabber"))));
     _status_icon->add_events(GDK_BUTTON_PRESS_MASK);
     _status_icon->button_press_event.connect(slot(this, &StatusIcon::on_button_press));
     _status_icon->realize();
     _status_icon->delete_event.connect(slot(this, &StatusIcon::on_window_delete));
     
     // Add the frame
     _frmQueue = manage(new Gtk::Frame());
     _frmQueue ->set_shadow_type(GTK_SHADOW_NONE);
     _status_icon->add(*_frmQueue);
     
     // Give it the proper pixmap
     update_status(p);

     Gtk::Main::timeout.connect(slot(this, &StatusIcon::FlashEvents), 500);

     _frmQueue->show();
     _status_icon->show();
}

StatusIndicator::StatusIcon::~StatusIcon()
{
}

gint StatusIndicator::StatusIcon::on_window_delete(GdkEventAny *e)
{
     on_close();

     return 0;
}

void StatusIndicator::StatusIcon::on_close()
{
     // Kill the status docklet
     if (_status_icon != NULL && _statusIcon != NULL)
     {
	  _status_icon->destroy();
	  delete _statusIcon;
	  _statusIcon = NULL;
     }
}

void StatusIndicator::StatusIcon::update_status(const jabberoo::Presence& p)
{
     _current_show = getShowFilename(p.getShow());
     if (p.getType() == jabberoo::Presence::ptInvisible)
	  _current_show = "invisible";

     // Grab the pixmap for current show
     string status_file = _pix_path + _current_show + ".xpm";
     Gdk_Pixmap icon_pixmap(_frmQueue->get_window(), 
			    _icon_mask,
			    Gdk_Color("white"),
			    status_file);
     Gtk::Widget* icon = manage(new Gtk::Pixmap(icon_pixmap, _icon_mask));
     _frmQueue->remove();
     _frmQueue->add(*icon);
     icon->show();

     _tips.set_tip(*_status_icon, getShowName(p.getShow()) + ": " + fromUTF8(_status_icon, p.getStatus()));
}

int StatusIndicator::StatusIcon::on_button_press(GdkEventButton* e)
{
     // Determine if we should show/hide the main window or popup menu
     if (e->type == GDK_BUTTON_PRESS)
     {
          MessageManager::EventList& events = G_App->getMessageManager().getEvents();
          if (!events.empty() && e->button == 1)
          {
	       _frmQueue->set_shadow_type(GTK_SHADOW_IN);
               G_App->getMessageManager().display(events.begin()->first, events.begin()->second);
               G_Win->refresh_roster();
          }
	  else if (e->button == 1)
	       G_Win->toggle_visibility();
	  else if (e->button == 3)
	  {
	       G_Win->get_menuDocklet()->show_all();
	       G_Win->get_menuDocklet()->popup(e->button, e->time);
	  }
     }

     return 0;
}

gint StatusIndicator::StatusIcon::FlashEvents()
{
     MessageManager& mm = G_App->getMessageManager();
     MessageManager::EventList& events = mm.getEvents();
     MessageManager::EventList::iterator it = events.begin();
     ConfigManager& cfgm = G_App->getCfg();

     if (it == events.end() && _need_refresh)
     {
          // Grab the pixmap for current show
          string status_file = _pix_path + _current_show + ".xpm";
          Gdk_Pixmap icon_pixmap(_frmQueue->get_window(),
                                 _icon_mask,
                                 Gdk_Color("white"),
                                 status_file);
          Gtk::Widget* icon = manage(new Gtk::Pixmap(icon_pixmap, _icon_mask));
          _frmQueue->remove();
          _frmQueue->add(*icon);
	  _frmQueue->set_shadow_type(GTK_SHADOW_NONE);
          icon->show();
	  _need_refresh = false;
     }
     else if (it != events.end())
     {
	  _need_refresh = true;
          if (_flash_events)
          {
               Gtk::Widget* icon = manage(new Gtk::Pixmap(mm.getEventPixmap(it->second), mm.getEventBitmap(it->second)));
               _frmQueue->remove();
               _frmQueue->add(*icon);
	       _frmQueue->set_shadow_type(GTK_SHADOW_OUT);
               icon->show();
          }
          else
          {
               Gtk::Widget* icon = manage(new Gtk::Pixmap(mm.getEventPixmap(0), mm.getEventBitmap(0)));
               _frmQueue->remove();
               _frmQueue->add(*icon);
               icon->show();
          }
     }
     _flash_events = !_flash_events;
     return TRUE;
}

// ---------------------------------------------------------
// Docklet Window
// ---------------------------------------------------------
StatusIndicator::DockletWin::DockletWin(const jabberoo::Presence& p)
     : _flash_events(false), _need_refresh(false), 
       _current_show(getShowFilename(p.getShow())), 
       _pix_path(ConfigManager::get_PIXPATH())
{
     if (p.getType() == jabberoo::Presence::ptInvisible)
	  _current_show = "invisible";

     // Yeah, this is the old way of doing things
     // But it works without gnome-libs 1.2.x ;)
     // This also allows us to work with KDE 1.x *and* KDE 2.x... it's nice to leave the desktop up to the user
     GdkAtom atom;
     gint data = 1;

     // make status docklet a new window with a fixed size
     _status_docklet = manage(new Gtk::Window(GTK_WINDOW_TOPLEVEL));
     _status_docklet->set_usize(22, 22);
     _status_docklet->add_events(GDK_BUTTON_PRESS_MASK);
     _status_docklet->button_press_event.connect(slot(this, &DockletWin::on_button_press));
     _status_docklet->realize();
     _status_docklet->delete_event.connect(slot(this, &DockletWin::on_window_delete));
     
     // Add the frame
     _frmQueue = manage(new Gtk::Frame());
     _frmQueue ->set_shadow_type(GTK_SHADOW_NONE);
     _status_docklet->add(*_frmQueue);
     
     // Give it the proper pixmap
     update_status(p);

     // Create the Status Docklet atom
     atom = gdk_atom_intern("KWM_DOCKWINDOW", false);
     gdk_property_change(_status_docklet->get_window().gdkobj(), atom, atom, 32,
			 GDK_PROP_MODE_REPLACE, (guchar *)&data, 1);

     // Do it again, this time for KDE2
     atom = gdk_atom_intern("_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", false);
     gdk_property_change(_status_docklet->get_window().gdkobj(), atom, XA_WINDOW, 32,
			 GDK_PROP_MODE_REPLACE, (guchar *)&data, 1);

     Gtk::Main::timeout.connect(slot(this, &DockletWin::FlashEvents), 500);

     _frmQueue->show();
     _status_docklet->show();
}

StatusIndicator::DockletWin::~DockletWin()
{
}

gint StatusIndicator::DockletWin::on_window_delete(GdkEventAny *e)
{
     on_close();

     return 0;
}

void StatusIndicator::DockletWin::on_close()
{
     // Kill the status docklet
     if (_status_docklet != NULL && _statusDock != NULL)
     {
	  _status_docklet->destroy();
	  delete _statusDock;
	  _statusDock = NULL;
     }
}

Gtk::Window* StatusIndicator::DockletWin::get_dock_window()
{
     if (_status_docklet)
	  return _status_docklet;
     else
	  return NULL;
}

void StatusIndicator::DockletWin::update_status(const jabberoo::Presence& p)
{
     _current_show = getShowFilename(p.getShow());
     if (p.getType() == jabberoo::Presence::ptInvisible)
	  _current_show = "invisible";

     // Grab the pixmap for current show
     string status_file = _pix_path + _current_show + ".xpm";
     Gdk_Pixmap icon_pixmap(_frmQueue->get_window(), 
			    _icon_mask,
			    Gdk_Color("white"),
			    status_file);
     Gtk::Widget* icon = manage(new Gtk::Pixmap(icon_pixmap, _icon_mask));
     _frmQueue->remove();
     _frmQueue->add(*icon);
     icon->show();

     _tips.set_tip(*_status_docklet, getShowName(p.getShow()) + ": " + fromUTF8(_status_docklet, p.getStatus()));
}

int StatusIndicator::DockletWin::on_button_press(GdkEventButton* e)
{
     // Determine if we should show/hide the main window or popup menu
     if (e->type == GDK_BUTTON_PRESS)
     {
          MessageManager::EventList& events = G_App->getMessageManager().getEvents();
          if (!events.empty() && e->button == 1)
          {
	       _frmQueue->set_shadow_type(GTK_SHADOW_IN);
               G_App->getMessageManager().display(events.begin()->first, events.begin()->second);
               G_Win->refresh_roster();
          }
	  else if (e->button == 1)
	       G_Win->toggle_visibility();
	  else if (e->button == 3)
	  {
	       G_Win->get_menuDocklet()->show_all();
	       G_Win->get_menuDocklet()->popup(e->button, e->time);
	  }
     }

     return 0;
}

gint StatusIndicator::DockletWin::FlashEvents()
{
     MessageManager& mm = G_App->getMessageManager();
     MessageManager::EventList& events = mm.getEvents();
     MessageManager::EventList::iterator it = events.begin();
     ConfigManager& cfgm = G_App->getCfg();

     if (it == events.end() && _need_refresh)
     {
          // Grab the pixmap for current show
          string status_file = _pix_path + _current_show + ".xpm";
          Gdk_Pixmap icon_pixmap(_frmQueue->get_window(),
                                 _icon_mask,
                                 Gdk_Color("white"),
                                 status_file);
          Gtk::Widget* icon = manage(new Gtk::Pixmap(icon_pixmap, _icon_mask));
          _frmQueue->remove();
          _frmQueue->add(*icon);
	  _frmQueue->set_shadow_type(GTK_SHADOW_NONE);
          icon->show();
	  _need_refresh = false;
     }
     else if (it != events.end())
     {
	  _need_refresh = true;
          if (_flash_events)
          {
               Gtk::Widget* icon = manage(new Gtk::Pixmap(mm.getEventPixmap(it->second), mm.getEventBitmap(it->second)));
               _frmQueue->remove();
               _frmQueue->add(*icon);
	       _frmQueue->set_shadow_type(GTK_SHADOW_OUT);
               icon->show();
          }
          else
          {
               Gtk::Widget* icon = manage(new Gtk::Pixmap(mm.getEventPixmap(0), mm.getEventBitmap(0)));
               _frmQueue->remove();
               _frmQueue->add(*icon);
               icon->show();
          }
     }
     _flash_events = !_flash_events;
     return TRUE;
}

// ---------------------------------------------------------
// Status Toolbar
// ---------------------------------------------------------
StatusIndicator::StatusToolbar::StatusToolbar(GabberWin* gwin)
     : _gwin(gwin), _flash_events(false), _need_refresh(false)
{
     // Grab the widgets
     _evtQueue     = _gwin->getEventBox("Gabber_Status_queue_evt");
     _evtQueue     ->button_press_event.connect(slot(this, &StatusToolbar::on_button_press));
     _frmQueue     = _gwin->getFrame("Gabber_Status_queue_frame");
     _frmQueue     ->set_shadow_type(GTK_SHADOW_NONE);
     _lblContacts  = _gwin->getLabel("Gabber_Status_contacts_lbl");
     _evtContacts  = _gwin->getEventBox("Gabber_Status_contacts_evt");
     _pixSigned    = _gwin->getPixmap("Gabber_Status_signed_pix");
     _evtSigned    = _gwin->getEventBox("Gabber_Status_signed_evt");
     _pixConnected = _gwin->getPixmap("Gabber_Status_connected_pix");
     _evtConnected = _gwin->getEventBox("Gabber_Status_connected_evt");

     _gwin->getPixmap("Gabber_Status_xmms_pix")->load(string(ConfigManager::get_PIXPATH()) + string("xmms.xpm"));

     Gtk::Main::timeout.connect(slot(this, &StatusToolbar::FlashEvents), 500);
}

StatusIndicator::StatusToolbar::~StatusToolbar()
{
}

void StatusIndicator::StatusToolbar::display_online_contacts(int num_online)
{
     char* txt_online;
     txt_online = g_strdup_printf("(%d)", num_online);
     // Set the label to the number online
     _lblContacts->set_text(txt_online);
     g_free(txt_online);
}

void StatusIndicator::StatusToolbar::display_connection(ConnectType connection)
{
     switch (connection)
     {
     case ctDisconnected:
	  _pixConnected->load(string(ConfigManager::get_SHAREDIR()) + string("disconnected.xpm"));
	  _tips.set_tip(*_evtConnected, _("Disconnected"));
	  break;
     case ctConnected:
	  _pixConnected->load(string(ConfigManager::get_SHAREDIR()) + string("connected.xpm"));
	  _tips.set_tip(*_evtConnected, _("Connected"));
	  break;
     case ctConnectedSSL:
	  _pixConnected->load(string(ConfigManager::get_SHAREDIR()) + string("connected-ssl.xpm"));
	  _tips.set_tip(*_evtConnected, _("Connected using SSL"));
	  break;
     }
}

void StatusIndicator::StatusToolbar::display_presence_signed(bool is_signed)
{
     if (is_signed)
     {
	  _pixSigned->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-signed.xpm"));
	  _tips.set_tip(*_evtSigned, _("Presence is signed"));
     }
     else
     {
	  _pixSigned->load(string(ConfigManager::get_SHAREDIR()) + string("gpg-unsigned.xpm"));  
	  _tips.set_tip(*_evtSigned, _("Presence is not signed"));
     }
}

void StatusIndicator::StatusToolbar::display_presence(const jabberoo::Presence& p)
{
     // It would be more intelligent to do more presence display info here, too

     Element* x = p.findX("gabber:x:music:info");
     if (!x)
     {
	  x = p.findX("jabber:x:music:info");
     }
     if (x)
     {
	  string song_title = x->getChildCData("title");
	  string state = x->getChildCData("state");
	  if (state == "stopped")
	  {
	       song_title += _(" [stopped]");
	       _gwin->getPixmap("Gabber_Status_xmms_pix")->load(string(ConfigManager::get_PIXPATH()) + string("xmms_stopped.xpm"));
	  }
	  else if (state == "paused")
	  {
	       song_title += _(" [paused]");
	       _gwin->getPixmap("Gabber_Status_xmms_pix")->load(string(ConfigManager::get_PIXPATH()) + string("xmms_paused.xpm"));
	  }
	  else
	  {
	       _gwin->getPixmap("Gabber_Status_xmms_pix")->load(string(ConfigManager::get_PIXPATH()) + string("xmms.xpm"));
	  }
	  _tips.set_tip(*_gwin->getEventBox("Gabber_Status_xmms_evt"), fromUTF8(_gwin->getEventBox("Gabber_Status_xmms_evt"), song_title));
	  _gwin->getEventBox("Gabber_Status_xmms_evt")->show();
     }
     else
     {
	  _gwin->getEventBox("Gabber_Status_xmms_evt")->hide();
     }
}

int StatusIndicator::StatusToolbar::on_button_press(GdkEventButton* e)
{
     if (e->type == GDK_BUTTON_PRESS && e->button == 1)
     {
          MessageManager::EventList& events = G_App->getMessageManager().getEvents();
          if (!events.empty())
          {
	       _frmQueue->set_shadow_type(GTK_SHADOW_IN);
               G_App->getMessageManager().display(events.begin()->first, events.begin()->second);
               G_Win->refresh_roster();
          }
     }

     return 0;
}

gint StatusIndicator::StatusToolbar::FlashEvents()
{
     MessageManager& mm = G_App->getMessageManager();
     MessageManager::EventList& events = mm.getEvents();
     MessageManager::EventList::iterator it = events.begin();
     ConfigManager& cfgm = G_App->getCfg();

     if (it == events.end() && _need_refresh)
     {
          // Grab the blank pixmap
          string blank_file = string(ConfigManager::get_SHAREDIR()) + "glade-blank.xpm";
          Gdk_Pixmap icon_pixmap(_frmQueue->get_window(),
                                 _icon_mask,
                                 Gdk_Color("white"),
                                 blank_file);
          Gtk::Widget* icon = manage(new Gtk::Pixmap(icon_pixmap, _icon_mask));
          _frmQueue->remove();
          _frmQueue->add(*icon);
	  _frmQueue->set_shadow_type(GTK_SHADOW_NONE);
          icon->show();
	  _need_refresh = false;
     }
     else if (it != events.end())
     {
	  _need_refresh = true;
          if (_flash_events)
          {
               Gtk::Widget* icon = manage(new Gtk::Pixmap(mm.getEventPixmap(it->second), mm.getEventBitmap(it->second)));
               _frmQueue->remove();
               _frmQueue->add(*icon);
	       _frmQueue->set_shadow_type(GTK_SHADOW_OUT);
               icon->show();
          }
          else
          {
               Gtk::Widget* icon = manage(new Gtk::Pixmap(mm.getEventPixmap(0), mm.getEventBitmap(0)));
               _frmQueue->remove();
               _frmQueue->add(*icon);
               icon->show();
          }
     }
     _flash_events = !_flash_events;
     return TRUE;
}
