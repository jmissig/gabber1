/* jabberoo-message.cc
 * Jabber Message
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

const unsigned int Message::numTypes = 5;

Message::Message(const Element& t)
     : Packet(t)
{
     // Determine message type..
     _type = translateType(t.getAttrib("type"));

#ifndef WIN32
     Element *x;
     string date;

     x = findX("jabber:x:delay");
     if (x) 
     {
	  date = x->getAttrib("stamp");
	  if (!date.empty()) 
	  {
	       struct tm ts;
	       memset(&ts, 0, sizeof(struct tm));
	       sscanf(date.c_str(), "%04d%02d%02dT%02d:%02d:%02d", &ts.tm_year, &ts.tm_mon, &ts.tm_mday, 
		      &ts.tm_hour, &ts.tm_min, &ts.tm_sec);
	       ts.tm_year -= 1900;
	       ts.tm_mon -= 1;
	       // ok, lets make this time local
#ifdef HAVE_TIMEGM
	       tzset(); // this should be called in the beginning anyways..
	       ts.tm_hour += timezone;
	       _timestamp = timegm(&ts); 
#else // HAVE_TIMEGM
	       char *tz = getenv("TZ");
	       setenv("TZ", "", 1);
	       tzset();
	       _timestamp = mktime(&ts);
	       if (tz)
		     setenv("TZ", tz, 1);
	       else
		     unsetenv("TZ");
	       tzset();
#endif // HAVE_TIMEGM
	       return;
	  }
     }
     // Otherwise we want to get the current time
     _timestamp = time(0);
     // And now we'll add the XML for it in case the XML of this message
     // is logged or passed on
     x = addX("jabber:x:delay");
     x->putAttrib("from", getFrom());
     x->putAttrib("stamp", getDateTime("%Y%m%dT%H:%M:%S%z"));
#endif     
}

Message::Message(const string& jid, const string& body, Message::Type mtype)
     : Packet("message")
{
     setTo(jid);
     if (!body.empty())
	  _base.addElement("body", body);
     _timestamp = time(0);
     setType(mtype);
}

void Message::setBody(const string& body)
{
     Element* body_tag = _base.findElement("body");
     if (body_tag)
     {
	  Element::iterator it = body_tag->begin();
	  for (; it != body_tag->end(); ++it )
	  {
	       if ((*it)->getType() != Node::ntCDATA)
		    continue;
	       (static_cast<CDATA*>(*it))->setText(body.c_str(), body.length());
          }
     }
     else
	  _base.addElement("body", body);
}

void Message::setSubject(const string& subject)
{
     Element* subject_tag = _base.findElement("subject");
     if (subject_tag)
     {
	  Element::iterator it = subject_tag->begin();
	  for (; it != subject_tag->end(); ++it )
	  {
	       if ((*it)->getType() != Node::ntCDATA)
		    continue;
	       (static_cast<CDATA*>(*it))->setText(subject.c_str(), subject.length());
	  }
     }
     else
	  _base.addElement("subject", subject);
}

void Message::setThread(const string& thread)
{
     Element* thread_tag = _base.findElement("thread");
     if (thread_tag)
     {
	  Element::iterator it = thread_tag->begin();
	  for (; it != thread_tag->end(); ++it )
	  {
	       if ((*it)->getType() != Node::ntCDATA)
		    continue;
	       (static_cast<CDATA*>(*it))->setText(thread.c_str(), thread.length());
          }
     }
     else
	  _base.addElement("thread", thread);
}

void Message::setType(Message::Type mtype)
{
     _base.putAttrib("type", translateType(mtype));
     _type      = mtype;
}

void Message::requestDelivered()
{
     Element* x = findX("jabber:x:event");
     if (x != NULL)
     {
	  x->addElement("delivered");
     }
     else
     {
	  Element* new_x = addX("jabber:x:event");
	  new_x->addElement("delivered");
     }
}

void Message::requestDisplayed()
{
     Element* x = findX("jabber:x:event");
     if (x != NULL)
     {
	  x->addElement("displayed");
     }
     else
     {
	  Element* new_x = addX("jabber:x:event");
	  new_x->addElement("displayed");
     }
}

void Message::requestComposing()
{
     Element* x = findX("jabber:x:event");
     if (x != NULL)
     {
	  x->addElement("composing");
     }
     else
     {
	  Element* new_x = addX("jabber:x:event");
	  new_x->addElement("composing");
     }
}

const string Message::getBody() const
{
     return string(_base.getChildCData("body"));
}

const string Message::getSubject() const
{
     return string(_base.getChildCData("subject"));
}

const string Message::getThread() const
{
     return string(_base.getChildCData("thread"));
}

Message::Type Message::getType() const
{
     return _type;
}

// added format for the future, for custom date formats in the logs
const string Message::getDateTime(const string& format) const
{
#ifndef WIN32
     char timestr[1024];
     struct tm *timestamp;

     timestamp = localtime(&_timestamp);
     if (format.empty())
	  strftime(timestr, 1024, _dtFormat.c_str(), timestamp);
     else
	  strftime(timestr, 1024, format.c_str(), timestamp);
     return string(timestr);
#else
    return "N/A";
#endif   
}

Message::Message(const Message& m, const string& body)
     : Packet("message")
{
     // Setup basic stuff
     setTo(m.getFrom());
     setType(m.getType());
     _base.addElement("body", body);

     // Get thread if available..
     string thread = m.getThread();
     if (!thread.empty())
	  _base.addElement("thread", thread);
}

Message Message::replyTo(const string& body) const
{
     return Message(*this, body);
}

Message Message::delivered() const
{
     Message new_m(getFrom(), "", getType()); // Create an empty message
     string thread = getThread();
     if (!thread.empty())
	  new_m.setThread(thread);
     Element* new_x = new_m.addX("jabber:x:event");
     new_x->addElement("delivered");
     if (!getID().empty())
	  new_x->addElement("id", getID());

     return new_m;
}

Message Message::displayed() const
{
     Message new_m(getFrom(), "", getType()); // Create an empty message
     string thread = getThread();
     if (!thread.empty())
	  new_m.setThread(thread);
     Element* new_x = new_m.addX("jabber:x:event");
     new_x->addElement("displayed");
     if (!getID().empty())
	  new_x->addElement("id", getID());

     return new_m;
}

Message Message::composing() const
{
     Message new_m(getFrom(), "", getType()); // Create an empty message
     string thread = getThread();
     if (!thread.empty())
	  new_m.setThread(thread);
     Element* new_x = new_m.addX("jabber:x:event");
     new_x->addElement("composing");
     if (!getID().empty())
	  new_x->addElement("id", getID());

     return new_m;
}


string Message::translateType(Type mtype)
{
     switch(mtype)
     {
     case mtNormal:
	  return "normal";
     case mtError:
	  return "error";
     case mtChat:
	  return "chat";
     case mtGroupchat:
	  return "groupchat";
     case mtHeadline:
	  return "headline";
     }
     return "normal";
}

Message::Type Message::translateType(const string& mtype)
{
     // Determine what type of message packet this is
     if (mtype == "")
	  return mtNormal;
     else if (mtype == "normal")
	  return mtNormal;
     else if (mtype == "error")
	  return mtError;
     else if (mtype == "chat")
	  return mtChat;
     else if (mtype == "groupchat")
	  return mtGroupchat;
     else if (mtype == "headline")
	  return mtHeadline;
     return mtNormal;
}

// This, unfortunately, is locale-independent
//string Message::_dtFormat  = "%d %b %Y %H:%M:%S";
string Message::_dtFormat  = "%c";

void Message::setDateTimeFormat(const string& format)
{
     _dtFormat = format;
}

const string& Message::getDateTimeFormat()
{
     return _dtFormat;
}
