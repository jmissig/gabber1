/* jabberoo-session.cc
 * Jabber Session management
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
 * Contributor(s): Julian Missig (IBM)
 *
 * This Original Code has been modified by IBM Corporation. Modifications 
 * made by IBM described herein are Copyright (c) International Business 
 * Machines Corporation, 2002.
 *
 * Date             Modified by     Description of modification
 * 01/20/2002       IBM Corp.       Updated to libjudo 1.1.1
 * 2002-03-05       IBM Corp.       Updated to libjudo 1.1.5
 * 2002-03-09       IBM Corp.       Catch libjudo's ParserError
 */


#include <jabberoo.hh>
using namespace jabberoo;

// ---------------------------------------------------------
// Initializers
// ---------------------------------------------------------
Session::Session()
     : ElementStream(this),
       _ID(0),
       _ConnState(csNotConnected),
       _Roster(*this),
       _PDB(*this),
       _StreamStart(false)
{}

Session::~Session()
{
     // If connected, transmit closing sequence of XML stream...
     if (_ConnState != csNotConnected)
	  disconnect();
}

// ---------------------------------------------------------
// Connection setup/teardown ops (inc. authentication)
// ---------------------------------------------------------
void Session::connect(const string& server, AuthType atype, 
		      const string& username, const string& resource, const string& password,
		      bool newuser)
{
     // If already connected, return
     if (_ConnState == csConnected)
	  return;

     // Store properties for future use
     _AuthType = atype;
     _ServerID = server;
     _Username = username;
     _Resource = resource;
     _Password = password;

      // If we haven't connected at all, send the stream header
     if (_ConnState == csNotConnected)
     {
	  this->reset(); // Reset ElementStream for new connection
	  // Transmit opening sequence to establish the XML stream..    
	  *this << "<stream:stream to='" << server.c_str()
		<< "' xmlns='jabber:client' xmlns:stream='http://etherx.jabber.org/streams'>";  
	  // Set state properly..
	  if (!newuser)
	       _ConnState = csAuthReq;
	  else
	       _ConnState = csCreateUser;
     }
     // Otherwise, attempt to authenicate again     
     else
     {
	  if (newuser)
	       _ConnState = csCreateUser;
	  authenticate();
     }
}

bool Session::disconnect()
{
     bool success = false;

     // only disconnect if we have received the starting stream tag to prevent the tagstream from throwing
     // an exception
     if ((_ConnState != csNotConnected) && _StreamStart)
     {
       *this << "</stream:stream>";
       success = true;
     }
     _ConnState = csNotConnected;
     _StreamStart = false;

     return success;
}

// ---------------------------------------------------------
// Accessors
// ---------------------------------------------------------
const Roster& Session::roster() const
{
     return _Roster;
}

Roster& Session::roster()
{
     return _Roster;
}

const PresenceDB& Session::presenceDB() const
{
     return _PDB;
}

PresenceDB&  Session::presenceDB()
{
     return _PDB;
}

Session::AuthType Session::getAuthType() const
{
     return _AuthType;
}

const string& Session::getUserName() const
{
	return _Username;
}

// ---------------------------------------------------------
// Id/auth ops
// ---------------------------------------------------------
string Session::getNextID()
{
     char buf[8];
     snprintf(buf, 8, "j%u", _ID++);
     return string(buf);
}

string Session::getDigest()
{
     string basic = _SessionID + _Password;
     return shahash(basic.c_str()); 
}

// ---------------------------------------------------------
// Misc ops
// ---------------------------------------------------------
void Session::push(const char* data, int datasz)
{
     evtRecvXML(data);
     try {
	  ElementStream::push(data, datasz);
     } catch (const ElementStream::exception::ParserError& error) {
	  // We had a parser error, so nothing's going to be happy
	  evtXMLParserError(error.getCode(), error.getMessage());

	  // Prevent loops :)
	  _ConnState = csNotConnected;
	  _StreamStart = false;

	  disconnect();
     }
}

void Session::registerIQ(const string& id, ElementCallbackFunc f)
{
     _Callbacks.insert(make_pair(id, f));
}

void Session::queryNamespace(const string& nspace, ElementCallbackFunc f, const string& to)
{
     // Get a unique ID
     string id = getNextID();

     // Construct a query for this namespace
     Element iq("iq");
     iq.putAttrib("type", "get");
     iq.putAttrib("id", id);
     if (!to.empty())
	  iq.putAttrib("to", to);
     iq.addElement("query")->putAttrib("xmlns", nspace);

     // Register a callback for this IQ
     _Callbacks.insert(make_pair(id, f));

     // Transmit the IQ
     *this << iq.toString().c_str();
}

// ---------------------------------------------------------
// INTERNAL
//
// Authentication
// ---------------------------------------------------------
void Session::authenticate()
{
     // Request auth types allowed
     string id = getNextID();
     Element iq("iq");
     iq.putAttrib("type", "get");
     iq.putAttrib("id", id);
     Element* query = iq.addElement("query");
     query->putAttrib("xmlns", "jabber:iq:auth");
     query->addElement("username", _Username, false);
     *this << iq.toString().c_str();

     // If they explicitly set auth, that's the type we use
     // otherwise detect the auth types allowed
     if (_AuthType != Session::atAutoAuth)
	  sendLogin(_AuthType, NULL);
     else
	  registerIQ(id, slot(*this, &Session::OnAuthTypeReceived));
}

void Session::OnAuthTypeReceived(const Element& t)
{
     // Grab the supported auth types and login using appropriate one
     Session::AuthType atype = Session::atPlaintextAuth;
     const Element* query = NULL;
     if (!t.empty())
     {
	  query = t.findElement("query");
	  if (query != NULL)
	  {
	       // Plaintext auth
	       const Element* password = query->findElement("password");
	       if (password != NULL)
		    atype = Session::atPlaintextAuth;
	       // Digest auth
	       const Element* digest = query->findElement("digest");
	       if (digest != NULL)
		    atype = Session::atDigestAuth;
	       // Zero-Knowledge
	       const Element* token = query->findElement("token");
	       const Element* sequence = query->findElement("sequence");
	       if (token != NULL && sequence != NULL)
		    atype = Session::at0kAuth;  // Add 0k support
	  }
     }
     sendLogin(atype, query);
}

void Session::sendLogin(Session::AuthType atype, const Element* squery)
{
     // Construct the login
     string id = getNextID();

     // Create the IQ
     Element iq("iq");
     iq.putAttrib("type", "set");
     iq.putAttrib("id", id);

     // Setup the query
     Element* query = iq.addElement("query");

     // Add basic tags
     query->addElement("username", _Username);
     query->addElement("resource", _Resource);
     
     // Handle user creation
     if (_ConnState == csCreateUser)
     {
	  query->putAttrib("xmlns", "jabber:iq:register");
	  query->addElement("password", _Password);
	  // Register the create user callback
 	  registerIQ(id, slot(*this, &Session::IQHandler_CreateUser)); 

     }
     else if (_ConnState == csAuthReq)
     {
	  query->putAttrib("xmlns", "jabber:iq:auth");
	  // Include necessary passphrase/digest
	  switch (atype)
	  {
	  case atDigestAuth: 
	       query->addElement("digest", getDigest());
	       break;
	  case atPlaintextAuth:
	       query->addElement("password", _Password);
	       break;
	  case at0kAuth:
	       string hashA = shahash(_Password.c_str());
	       string token = squery->getChildCData("token");
	       int seq = atoi(squery->getChildCData("sequence").c_str());
	       string hash = shahash(string(hashA + token).c_str());
	       for (int i = 0; i < seq; i++)
		    hash = shahash(hash.c_str());
	       query->addElement("hash", hash);
	       break;
	  }
	  // Register the auth callback
	  registerIQ(id, slot(*this, &Session::IQHandler_Auth));
     }

     // Transmit the buffer
     *this << iq.toString().c_str();

     // Adjust the connection state
     _ConnState = csAwaitingAuth;
}

// ---------------------------------------------------------
// INTERNAL
//
// ElementStreamEventListener handlers
// ---------------------------------------------------------
void Session::onDocumentStart(Element* t)
{
     // Retrieve the SID from the stream header
     _SessionID = t->getAttrib("id");
     // Save stream header
     _StreamElement = t;
     _StreamStart = true;
     // Authenticate
     authenticate();
}

void Session::onElement(Element* t) 
{
     Element& tref = *t;
     // Determine what kind of packet we recv'd and call the 
     // appropriate handler
     if (tref.getName() == "message")
	  handleMessage(tref);
     else if (tref.getName() == "presence")
	  handlePresence(tref);
     else if (tref.getName() == "iq")
	  handleIQ(tref);
     else
	  evtUnknownPacket(tref);
     delete t;
}

void Session::onCDATA(CDATA* c)
{
     // If we ever want to deal with CDATA we receive
     // possibly message sizes and things like that
     // then we do it here.
     delete c;
}

void Session::onDocumentEnd() 
{
     // Reset roster
     _Roster.reset();
     // Reset the presenceDB
     _PDB.clear();

     // Clear connection state
     _ConnState = csNotConnected;
     _StreamStart = false;

      // Transmit a closing stream tag...
     *this << "</stream:stream>";

     // Fire session Disconnect handler
     evtDisconnected();
}

// ---------------------------------------------------------
// INTERNAL
//
// Routing handlers (see also: IQ handlers)
// ---------------------------------------------------------
void Session::handleMessage(Element& t)
{
     // Call the signal handler
     evtMessage(Message(t));
}

void Session::handlePresence(Element& t)
{
     Presence p(t);

     // If no sender, it's about my presence
     if (p.getFrom() == "")
     {
	  evtMyPresence(p);
     }

     // If it's a subscription request, fire proper event
     else if ((p.getType() == Presence::ptSubRequest) ||
	 (p.getType() == Presence::ptUnsubRequest))
	  evtPresenceRequest(p);

     // Otherwise, examine presence more closely
     else
     {
	  // Determine the previous status for this jid
	  Presence::Type prev_type;
	  try {
 	       prev_type = _PDB.find(p.getFrom())->getType();
	  } catch (PresenceDB::XCP_InvalidJID) {
	       prev_type = Presence::ptUnavailable;
	  }

	  // Insert the packet into the presence db
	  _PDB.insert(p);

	  // Check for roster update
	  if (_Roster.containsJID(p.getFrom()))
	       _Roster.update(p, prev_type);	       
	  // pass handling to standard event
	  evtPresence(p, prev_type);
     }
}

void Session::handleIQ(Element& t)
{
     // Check for callback w/ ID
     typedef multimap<string, ElementCallbackFunc>::iterator CIT;
     pair<CIT, CIT> cb = _Callbacks.equal_range(t.getAttrib("id"));
     if (cb.first != cb.second)
     {
	  // Iterate across the range and fire callbacks
	  for (CIT it = cb.first; it != cb.second; )
	  {
               // Adding a callback with an ID one greater than the ID of this tag
               // screws up the iterator so we need to make sure the id's are actually
               // equal.  Is there a way to prevent the iterator from being broken?
               if (it->first == t.getAttrib("id")) {
	            // Fire the requested function
	            ElementCallbackFunc& f = it->second;
	            f(t);
		    // erasing it then incrementing it doesn't work right so increment first
		    // then delete
                    CIT nit = it++;
	            _Callbacks.erase(nit);
	       } else
                    it++;
	  }
     }
     // Proceed with xmlns examination
     else
     {
	  Element* q = t.findElement("query");

	  // Catch the odd case of an IQ not having a <query> tag..
	  if (q == NULL) return;

	  // jabber:iq:roster 
	  if (q->cmpAttrib("xmlns", "jabber:iq:roster"))
	  {
	       evtOnRoster();
	       _Roster.update(*q);
	  }

	  // jabber:iq:version
	  if (t.cmpAttrib("type", "get") && q->cmpAttrib("xmlns", "jabber:iq:version"))
	  {
	       string name, ver, os;
	       evtOnVersion(name, ver, os);
	       ver += " (powered by jabberoo 1.1.2)";
	       Packet iq("iq");
	       iq.getBaseElement().putAttrib("type", "result");
	       iq.setTo(t.getAttrib("from"));
	       iq.setID(t.getAttrib("id"));
	       Element* query = iq.getBaseElement().addElement("query");
	       query->putAttrib("xmlns", "jabber:iq:version");
	       query->addElement("name", name);
	       query->addElement("version", ver);
	       query->addElement("os", os);
	       *this << iq;
	  }
	  // jabber:iq:last
	  else if (t.cmpAttrib("type", "get") && q->cmpAttrib("xmlns", "jabber:iq:last"))
	  {
	       string seconds;
	       evtOnLast(seconds);
	       Packet iq("iq");
	       iq.getBaseElement().putAttrib("type", "result");
	       iq.setTo(t.getAttrib("from"));
	       iq.setID(t.getAttrib("id"));
	       Element* query = iq.getBaseElement().addElement("query");
	       query->putAttrib("xmlns", "jabber:iq:last");
	       query->putAttrib("seconds", seconds);
	       *this << iq;
	  }
	  // jabber:iq:time
	  else if (t.cmpAttrib("type", "get") && q->cmpAttrib("xmlns", "jabber:iq:time"))
	  {
#ifndef WIN32
	       Packet iq("iq");
	       iq.getBaseElement().putAttrib("type", "result");
	       iq.setTo(t.getAttrib("from"));
	       iq.setID(t.getAttrib("id"));
	       Element* query = iq.getBaseElement().addElement("query");
	       query->putAttrib("xmlns", "jabber:iq:time");
	       query->addElement("utc", jutil::getTimeStamp());

	       struct tm *loctime;
               time_t curtime;
	       char timestr[1024];

	       curtime = time(0);
	       loctime = localtime(&curtime);

	       // locale-dependent
	       //strftime(timestr, 1024, "%d %b %Y %H:%M:%S", loctime);
	       strftime(timestr, 1024, "%c", loctime);
	       string local= string(timestr); // Local time string
	       strftime(timestr, 1024, "%Z", loctime);
	       string tz = string(timestr); // Time zone
               if (local.find(tz)!=string::npos) // Don't duplicate timezone
                       local.erase(local.find(tz));
	       evtOnTime(local, tz);

	       query->addElement("display", local);
	       query->addElement("tz", tz);
	       *this << iq;
#endif
	  }
	  // Otherwise pass handling to standard event
	  else
	       evtIQ(t);
     }
}


// ---------------------------------------------------------
// INTERNAL
//
// Internal IQ handlers
// ---------------------------------------------------------
void Session::IQHandler_Auth(const Element& t)
{
     // If successful, update connection state
     // and call _Events.OnConnected
     if (t.cmpAttrib("type", "result"))
     {
	  _ConnState = Session::csConnected;	       // Update connection state
	  evtConnected(*_StreamElement);	       // Notify libuser that connection is available
	  _Roster.fetch();	                       // Request roster
	  delete _StreamElement;		       // Release stream header tag
     }
     else
     {
	  // Handle errors
	  const Element* error = t.findElement("error");
	  if (error != NULL)
	  {
	       _ConnState = Session::csAuthReq;
	       evtAuthError(atoi(error->getAttrib("code").c_str()), error->getCDATA().c_str());
	  }
	  else
	       evtAuthError(-1, t.toString().c_str());
     }
}

void Session::IQHandler_CreateUser(const Element& t)
{
     // If successful, update connection state and
     // attempt to authenticate
     _ConnState = Session::csAuthReq;
     if (t.cmpAttrib("type", "result"))
	  authenticate();
     else
     {
	  const Element* error = t.findElement("error");
	  if (error != NULL)
	  {
	       evtAuthError(atoi(error->getAttrib("code").c_str()), error->getCDATA().c_str());
	  }
     }
}



