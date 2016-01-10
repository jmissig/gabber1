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


#ifndef INCL_AUTO_AWAY_HH
#define INCL_AUTO_AWAY_HH

#include "jabberoo.hh"

#include "GabberConfig.hh"

#include <sigc++/object_slot.h>
#include <gdk/gdk.h>

#ifdef HAVE_XSS
#include <gdk/gdkx.h>
#include <X11/extensions/scrnsaver.h>
#endif /* HAVE_XSS */

class AutoAway
     : public SigC::Object
{
public:
     AutoAway();
     ~AutoAway();
protected:
     void on_session_connected(const Element& t);
     void on_session_disconnected();
     void on_session_last(string &idletime);
     gint auto_away_timer();
     void AutoAway::update_statusbar_aatime_msg(unsigned long idle_time);

     void set_away(jabberoo::Presence::Show show);
     void set_back();
     unsigned long get_idle_time();
private:
     string _oldStatus;
     int    _oldPriority;
     jabberoo::Presence::Show _curShow;
     bool   _autoaway;
     bool   _have_xss;

     SigC::Connection _timer;

#ifdef HAVE_XSS
     XScreenSaverInfo* _scrnsaver_info;
#endif /* HAVE_XSS */
};

#endif
