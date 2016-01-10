/* jabberoo-presence.cc
 * Jabber Presence
 *
 * Original Code Copyright (C) 1999-2001 Dave Smith (dave@jabber.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contributor(s): Julian Missig
 *
 * This Original Code has been modified by IBM Corporation. Modifications 
 * made by IBM described herein are Copyright (c) International Business 
 * Machines Corporation, 2002.
 *
 * Date             Modified by     Description of modification
 * 01/20/2002       IBM Corp.       Updated to libjudo 1.1.1
 * 2002-03-05       IBM Corp.       Updated to libjudo 1.1.5
 */


#include <jabberoo.hh>

using namespace jabberoo;

Presence::Presence(const Element& t)
     : Packet(t)
{
     // Determine the presence type
     _type = translateType(t.getAttrib("type"));
     // Determine the show value
     _show = translateShow(_type, t.getChildCData("show"));
}

Presence::Presence(const string& jid, Presence::Type ptype, Presence::Show stype, const string& status, const string& priority)
     : Packet("presence")
{
     if (!jid.empty())
	  setTo(jid);
     setType(ptype);
     setStatus(status);
     setShow(stype);
     setPriority(priority);
}

void Presence::setType(Presence::Type ptype)
{
     if (ptype != Presence::ptAvailable)
	  _base.putAttrib("type", translateType(ptype));
     _type = ptype;
}

void Presence::setStatus(const string& status)
{
     if (!status.empty())
	  _base.addElement("status", status);
}

void Presence::setShow(Presence::Show stype)
{
     if (stype > stOnline)
     {
	  _base.addElement("show", translateShow(stype));
     }
     _show = stype;
}

void Presence::setPriority(const string& priority)
{
     if (priority != "0")
     {
	  _base.addElement("priority", priority);
     }
     _priority = atoi(priority.c_str());
}

Presence::Type Presence::getType() const
{
     return _type;
}

const string Presence::getStatus() const
{
     if (_base.getChildCData("status").empty())
	  return _base.getChildCData("error");
     else
	  return _base.getChildCData("status");
}

Presence::Show Presence::getShow() const
{
     return _show;
}

const string Presence::getShow_str() const
{
     return translateShow(_show);
}

int Presence::getPriority() const
{
     if (_type == Presence::ptUnavailable)
	  return -1;
     else
	  return atoi(_base.getChildCData("priority").c_str());
}

string Presence::translateType(Type ptype)
{
     switch(ptype)
     {
     case ptAvailable: 
	  return string("");
     case ptUnavailable: 
	  return string("unavailable");
     case ptSubRequest: 
	  return string("subscribe");
     case ptUnsubRequest: 
	  return string("unsubscribe");
     case ptSubscribed : 
	  return string("subscribed");
     case ptUnsubscribed:
	  return string("unsubscribed");
     case ptError:
	  return string("error");
     case ptInvisible:
	  return string("invisible");
     }
     return string("");
}

string Presence::translateShow(Show stype)
{
     switch(stype)
     {
     case stInvalid:
	  return string("");
     case stOnline:
	  return string("online");
     case stOffline:
	  return string("offline");
     case stChat:
	  return string("chat");
     case stAway:
	  return string("away");
     case stXA:
	  return string("xa");
     case stDND:
	  return string("dnd");
     }
     return string("");
}

Presence::Type Presence::translateType(const string& ptype)
{
     // Determine what type of presence packet this is
     if (ptype.empty())
	  return ptAvailable;
     else if (ptype == "subscribe")
	  return ptSubRequest;
     else if (ptype == "unsubscribe")
	  return ptUnsubRequest;
     else if (ptype == "subscribed")
	  return ptSubscribed;
     else if (ptype == "unsubscribed")
	  return ptUnsubscribed;
     else if (ptype == "error")
	  return ptError;
     else if (ptype == "invisible")
	  return ptInvisible;
     else
	  return ptUnavailable;
}

Presence::Show Presence::translateShow(Type ptype, const string& show)
{
     // Determine what value show should return
     if (ptype == ptError)
	  return stOffline;
     else if ((ptype == ptAvailable) && (show.empty()))
	  return stOnline;
     else if ((ptype == ptAvailable) && (show == "online"))
	  return stOnline;
     else if ((ptype == ptUnavailable) && (show.empty()))
	  return stOffline;
     else if ((ptype == ptUnavailable) && (show == "offline"))
	  return stOffline;
     else if (show == "chat")
	  return stChat;
     else if (show == "away")
	  return stAway;
     else if (show == "xa")
	  return stXA;
     else if (show == "dnd")
	  return stDND;
     else if (ptype == ptAvailable) // Maybe we should return stOnline?
	  return stInvalid;         // Only way this could occur is if it's and unknown <show>
     else
	  return stOffline;
}


