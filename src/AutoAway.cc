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

#include "GabberConfig.hh"

#include "AutoAway.hh"

#include "GabberApp.hh"
#include "GabberWin.hh"

AutoAway::AutoAway()
     : _autoaway(false), _have_xss(false)
{
#ifdef HAVE_XSS
     int event_base, error_base;

     if (XScreenSaverQueryExtension(GDK_DISPLAY(), &event_base, &error_base))
     {
          _scrnsaver_info = XScreenSaverAllocInfo();
	  _have_xss = true;
     }
     else
#endif /* HAVE_XSS */
	  cerr << "X Screen Saver Extension not found, auto-away disabled" << endl;

     G_App->getSession().evtConnected.connect(slot(this, &AutoAway::on_session_connected));
     G_App->getSession().evtDisconnected.connect(slot(this, &AutoAway::on_session_disconnected));
     G_App->getSession().evtOnLast.connect(slot(this, &AutoAway::on_session_last));
}

AutoAway::~AutoAway()
{
}

void AutoAway::on_session_connected(const Element& t)
{
     _autoaway = false;
     _timer = Gtk::Main::timeout.connect(slot(this, &AutoAway::auto_away_timer), 1000);
}

void AutoAway::on_session_disconnected()
{
     _timer.disconnect();
}

gint AutoAway::auto_away_timer()
{
     unsigned long idle_time = 0;
     ConfigManager& cf = G_App->getCfg();

     if (!cf.autoaway.enabled)
	  return TRUE;

     idle_time = get_idle_time();
   
     Presence::Show show = (Presence::Show)cf.get_show();

     unsigned int aatime = cf.autoaway.awayafter;
     unsigned int natime = cf.autoaway.xaafter;
     signed long away_time = (idle_time - (aatime * 60000));

     if (!aatime && !natime)
          return TRUE;

     // idle_time is in milliseconds, assume that natime will be longer than away time
     if (natime && (idle_time > natime * 60000))
     {
	  if (_autoaway || (show == Presence::stOnline || show == Presence::stChat))
	  {
	       set_away(Presence::stXA);
	       G_Win->push_status_bar_msg(_("Moving to extended away"), 1000);
	  }
	  
          // Only display the timer if we're automatically away, not away set by the user
	  if (_autoaway && show == Presence::stXA)
	       update_statusbar_aatime_msg(away_time);  // Update the status bar
     }

     else if (aatime && (idle_time > aatime * 60000))
     {
	  if (show == Presence::stOnline || show == Presence::stChat)
	  {
	       set_away(Presence::stAway);
	       G_Win->push_status_bar_msg(_("Starting auto-away"), 1000);
	  }

	  else if (_autoaway)
	       update_statusbar_aatime_msg(away_time);  // Update the status bar
     }
     else if (_autoaway)
     {
          set_back();

	  // Update the status bar
	  G_Win->push_status_bar_msg(_("Returning from auto-away"), 3000);
     }
     return TRUE;
}

void AutoAway::update_statusbar_aatime_msg(unsigned long idle_time)
{
     // Manipulate the milliseconds of idle_time into 'normal' units
     int days, hrs, mins, secs, seconds;
     seconds = (idle_time / 1000);       // Let's not worry about rounding here
     days = (seconds / (3600 * 24));
     hrs  = ((seconds / 3600) - (days * 24));
     mins = ((seconds / 60) - (days * 24 * 60) - (hrs * 60));
     secs = (seconds - (days * 24 * 60 * 60) - (hrs * 60 * 60) - (mins * 60));
     char *tdays, *thrs, *tmins, *tsecs;
     tdays = g_strdup_printf("%d", days);
     thrs  = g_strdup_printf("%d", hrs);
     tmins = g_strdup_printf("%d", mins);
     tsecs = g_strdup_printf("%d", secs);
     
     // Format the string to be displayed
     string aamsg = _("Auto away for ");
     if (days > 0)
	  aamsg += string(tdays) + " " + _("day") + " ";
     else
	  g_free(tdays);
     
     if (hrs > 0)
	  aamsg += string(thrs) + ":" + string(tmins) + ":" + string(tsecs);
     else
     {
	  aamsg += string(tmins);

	  if (secs < 10)
	       aamsg += ":0" + string(tsecs);
	  else
	       aamsg += ":" + string(tsecs);

	  g_free(thrs);
	  g_free(tmins);
	  g_free(tsecs);
     }
     
     // Update the status bar with the Fancy Style (like ketchup in packets) message
     G_Win->push_status_bar_msg(aamsg, 1000);
}

void AutoAway::set_away(Presence::Show show)
{
     ConfigManager& cf = G_App->getCfg();

     // Only send presence if it's a change
     if (cf.get_show() == show)
	  return;

     // Only set old value if we are changing from online to autoaway
     if (!_autoaway)
     {
	  _oldStatus = cf.get_status();
	  _oldPriority = cf.get_priority();
     }

     cf.set_show(show);
     cf.set_status(cf.autoaway.status);
     // If they want priority lowered when autoaway
     if (cf.autoaway.changepriority)
	  cf.set_priority(0);

     G_Win->display_status(show, cf.autoaway.status, cf.get_priority());

     _autoaway = true;
}

void AutoAway::set_back()
{
     ConfigManager& cf = G_App->getCfg();

     cf.set_show(Presence::stOnline);
     cf.set_status(_oldStatus);
     cf.set_priority(_oldPriority);

     G_Win->display_status(Presence::stOnline, _oldStatus, cf.get_priority());
     _autoaway = false;
}

unsigned long AutoAway::get_idle_time()
{
#ifdef HAVE_XSS
     if (_have_xss)
     {
          XScreenSaverQueryInfo(GDK_DISPLAY(), GDK_ROOT_WINDOW(), _scrnsaver_info);
          return _scrnsaver_info->idle;
     }
#endif /* HAVE_XSS */
     // Is there any other way we could try to get idle time?

     return 0;
}

void AutoAway::on_session_last(string& idletime)
{
     unsigned long idle;
     char idlestr[1024];

     // get_idle_time returns time in milliseconds so we have to convert to secs
     idle = get_idle_time() / 1000;
     g_snprintf(idlestr, 1024, "%ld", idle);
     cerr << "IIIII****III***III**II Idle Time is: " << idlestr << endl;
     idletime = idlestr;
}
