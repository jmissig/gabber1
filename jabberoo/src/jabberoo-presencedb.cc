/* jabberoo-presencedb.cc
 * Jabber Presence Database
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
 */

#include "jabberoo.hh"
using namespace jabberoo;

// Helpful predicates
class pred_presence_jid
{
string _jid;
public:
     pred_presence_jid(const Presence& p)
	  : _jid(p.getFrom()) {}
     pred_presence_jid(const string& s) : _jid(s) {}
     // is there a string function to do a case insensitive compare?
     bool operator()(const Presence& p) const
	  { return (JID::compare(_jid, p.getFrom()) == 0); }
};

class pred_presence_priority
{
int _priority;
public:
     pred_presence_priority(const Presence& p)
	  : _priority(p.getPriority()){}
     bool operator()(const Presence& p) const
	  { return p.getPriority() <= _priority; }
};

PresenceDB::PresenceDB(Session& s)
     : _Owner(s)
{}

PresenceDB::db::const_iterator PresenceDB::find_or_throw(const string& jid) const
{
     db::const_iterator it = _DB.find(JID::getUserHost(jid));
     if (it != _DB.end())
	  return it;
     else
	  throw XCP_InvalidJID();
}

void display(const Presence& p)
{
     cerr << "\tItem: " << p.toString() << endl;
}


void PresenceDB::insert(const Presence& p)
{
     // Get a reference to the list (create one if necessary)
     list<Presence>& l = _DB[JID::getUserHost(p.getFrom())];
     
     // If this presence is ptUnavailable, remove it from the cache
     if (p.getType() == Presence::ptUnavailable ||
	 p.getType() == Presence::ptError)
     {
	  // Empty list? Then exit..
	  if (l.empty())
	  {
	       _DB.erase(JID::getUserHost(p.getFrom()));
	       return;
	  }
	  else
	  {
	       // Attempt to find the presence packet and erase
	       iterator it = find_if(l.begin(), l.end(), pred_presence_jid(p));
	       if (it != l.end())
		    l.erase(it);
	       // If the list is now empty, remove this entry from the _DB map
	       if (l.empty())
		    _DB.erase(JID::getUserHost(p.getFrom()));
	}
    }
    // Otherwise, insert/update 
    else 
    {
	// If this list is empty, insert this presence and be done with it
	if (l.empty())
	  {
	       l.push_back(p);
	  }
	  // Otherwise insert into the list
	  else
	  {
	       // Identify any existing items w/ this JID 
	       iterator it = find_if(l.begin(), l.end(), pred_presence_jid(p));
	       if (it != l.end())
	       {
		    // If the item found has the same priority, simply update
		    if (it->getPriority() == p.getPriority())
		    {
			 // Replace and exit
			 *it = p;
			 return;
		    }
		    // Otherwise erase the element 
		    else
			 l.erase(it);
	       }
	       // Now identify the insertion point for this element
	       it = find_if(l.begin(), l.end(), pred_presence_priority(p));
	       l.insert(it, p);
	  }
     }
}

void PresenceDB::remove(const string& jid)
{
     list<Presence>& l = _DB[JID::getUserHost(jid)];
     
     // Empty list? Then exit..
     if (l.empty())
     {
	  _DB.erase(JID::getUserHost(jid));
	  return;
     }
     else
     {
	  // Attempt to find the presence packet and erase
	  iterator it = find_if(l.begin(), l.end(), pred_presence_jid(jid));
	  if (it != l.end())
	       l.erase(it);
	  // If the list is now empty, remove this entry from the _DB map
	  if (l.empty())
	       _DB.erase(JID::getUserHost(jid));
     }
}


PresenceDB::range PresenceDB::equal_range(const string& jid) const
{
    db::const_iterator it = find_or_throw(jid);
    return make_pair(it->second.begin(), it->second.end());
}

Presence PresenceDB::findExact(const string& jid) const
{
     PresenceDB::db::const_iterator it = find_or_throw(jid);

     // If the list is empty, throw an exception
     if (it->second.begin() == it->second.end())
	  throw XCP_InvalidJID();

     // otherwise check for a resource
     string::size_type i = jid.find("/");
     if (i != string::npos)
     {
	  list<Presence> l= it->second;

	  //return matching resource
	  // Identify any existing items w/ this JID 
	  iterator it2 = find_if(l.begin(), l.end(), pred_presence_jid(jid));

	  // if we didn't find an entry, throw an exception
	  if (it2 == l.end())
	       throw XCP_InvalidJID();

	  // return presence matching the full JID
	  return *it2;
     }
     else
     {
	  //return the first item in the list
	  return *find_or_throw(jid)->second.begin();
     }
}

PresenceDB::const_iterator PresenceDB::find(const string& jid) const
{
     PresenceDB::db::const_iterator it = find_or_throw(jid);
     // If the list is empty, throw an exception, otherwise return the first
     // item in the list
     if (it->second.begin() != it->second.end())
	  return find_or_throw(jid)->second.begin();
     else
	  throw XCP_InvalidJID();
}

bool PresenceDB::contains(const string& jid) const
{
     return (_DB.find(JID::getUserHost(jid)) != _DB.end());
}

bool PresenceDB::available(const string& jid) const
{
     db::const_iterator it = _DB.find(JID::getUserHost(jid));
     // it->second is a list that could be empty so we need to check
     // if there is actually an item in the list before we call getType
     // on the first iitem
     if (it != _DB.end() && it->second.begin() != it->second.end())
	  return (it->second.begin()->getType() == Presence::ptAvailable);
     else
	  return false;
}

void PresenceDB::clear()
{
     // Erase all entries from the DB
     _DB.clear();
}
