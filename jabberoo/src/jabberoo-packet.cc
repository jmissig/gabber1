/* jabberoo-packet.cc
 * Jabber base packet
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

Packet::Packet(const string& name)
     : _base(name)
{}

Packet::Packet(const Element& t)
     : _base(t)
{}

const string Packet::getFrom() const
{
     return _base.getAttrib("from");
}

const string Packet::getTo() const
{
     return _base.getAttrib("to");
}

const string Packet::getID() const
{
     return _base.getAttrib("id");
}

const string Packet::getError() const
{
     return string(_base.getChildCData("error"));
}

const int Packet::getErrorCode() const
{
     const Element* error = _base.findElement("error");
     if (error)
	  return atoi(error->getAttrib("code").c_str());
     else
	  return 0;
}

const string Packet::toString() const
{
     return _base.toString();
}

const Element& Packet::getBaseElement() const
{
     return _base;
}

void Packet::setFrom(const string& from)
{
     _base.putAttrib("from", from);
}

void Packet::setTo(const string& to)
{
     _base.putAttrib("to", to);
}

void Packet::setID(const string& id)
{
     _base.putAttrib("id", id);
}

Element* Packet::addX()
{
     return _base.addElement("x");
}

Element* Packet::addX(const string& tnamespace)
{
     Element* x = _base.addElement("x");
     x->putAttrib("xmlns", tnamespace);
     return x;
}

Element* Packet::findX(const string& tnamespace) const
{
     Element::const_iterator it = _base.begin();
     for (; it != _base.end(); it++)
     {
	  if (((*it)->getType() == Node::ntElement) && 
	      ((static_cast<Element*>(*it))->cmpAttrib("xmlns", tnamespace)))
	       break;
     }
     if (it != _base.end())
	  return static_cast<Element*>(*it);
     else
	  return NULL;
}

void Packet::eraseX(const string& tnamespace)
{
     Element::iterator it = _base.begin();
     for (; it != _base.end(); it++)
     {
	  if (((*it)->getType() == Node::ntElement) && 
	      ((static_cast<Element*>(*it))->cmpAttrib("xmlns", tnamespace)))
	       break;
     }
     if (it != _base.end())
	  static_cast<Element*>(*it)->erase(it);
}

Element& Packet::getBaseElement()
{
     return _base;
}
