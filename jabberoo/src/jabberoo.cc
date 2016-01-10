/* jabberoo.cc
 * Jabber client library
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


#include <jabberoo.hh>

using namespace jabberoo;


string JID::getResource(const string& jid)
{     // Search for resource divider
     string::size_type i = jid.find("/");
     // If it was found, extract the resource and return it..
     if (i != string::npos)
	return jid.substr(i+1, jid.length()-i);
     else
     	return "";
}

string JID::getUserHost(const string& jid)
{
     // Search for resource divider..
     string::size_type i = jid.find("/");
     // If it was found, extract he userhost and return..
     if (i != string::npos)
	  return jid.substr(0,i);
     else
	  return jid;
}

string JID::getHost(const string& jid)
{
     string::size_type d1 = jid.find("@"); // Search for @ divider
     string::size_type d2 = jid.find("/"); // Search for / divider
     if ((d1 != string::npos) && (d2 != string::npos))
	  return jid.substr(d1+1,d2-d1-1);
     else if ((d1 != string::npos) && (d2 == string::npos))
	  return jid.substr(d1+1, jid.length());
     else if ((d1 == string::npos) && (d2 != string::npos))
	  return jid.substr(0,d2);
//     else if ((d1 == string::npos) && (d2 == string::npos))
     else
	  return jid;
}

string JID::getUser(const string& jid)
{
	string::size_type d1 = jid.find("@");
	if (d1 != string::npos)
		return jid.substr(0, d1);
	else
		return jid;
}

bool JID::isValidUser(const string& user)
{
     if ( (user.empty()) ||
	  (user.find(' ') != string::npos) ||
	  (user.find('@') != string::npos) ||
	  (user.find('/') != string::npos) ||
	  (user.find('\'') != string::npos) ||
	  (user.find('\"') != string::npos) ||
	  (user.find(':') != string::npos)
	  // Insert other invalid chars here
	  )
     {
	  return false;
     }
     return true;
}

bool JID::isValidHost(const string& host)
{
     if ( (host.empty()) ||
	  (host.find(' ') != string::npos) ||
	  (host.find('@') != string::npos) ||
	  (host.find('/') != string::npos) ||
	  (host.find('\'') != string::npos) ||
	  (host.find('\"') != string::npos)
	  // Insert other invalid chars here
	  )
     {
	  return false;
     }
     return true;
}

int JID::compare(const string& ljid, const string& rjid)
{
     // User and Host are case insensitive, Resource is case sensitive
     int userhost = strcasecmp(JID::getUserHost(ljid).c_str(), JID::getUserHost(rjid).c_str());
     int resource = JID::getResource(ljid).compare(JID::getResource(rjid));

     // If the user and host of both are equal, return whether the resource is
     if (userhost == 0)
	  return resource;
     return userhost;
}

bool JID::Compare::operator()(const string& lhs, const string& rhs) const
{
     return (JID::compare(lhs, rhs) < 0);
}

