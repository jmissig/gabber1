/* jabberoox-agent.cc
 * Jabber agent support
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

#include "jabberoo.hh"
#include "jabberoox.hh"
using namespace jabberoo;

// ---------------------------------------------------------
//
// Agent methods
//
// ---------------------------------------------------------
Agent::Agent(const Agent& a)
     : _session(a._session), _jid(a._jid), _name(a._name),
       _desc(a._desc), _service(a._service), _transport(a._transport),
       _registerable(a._registerable), _searchable(a._searchable),
       _subagents(a._subagents)
{
     if (_name.empty())
	  _name = "No name available (" + a._jid + ")";
}

Agent::Agent(const string& jid, const Element& baseElement, Session& s)
     : _jid(jid), _session(s)
{
     _name      = baseElement.getChildCData("name");
     _desc      = baseElement.getChildCData("description");
     _service   = baseElement.getChildCData("service");
     _transport = baseElement.getChildCData("transport");
     _registerable   = (baseElement.findElement("register") != NULL);
     _searchable     = (baseElement.findElement("search") != NULL);
     _subagents      = (baseElement.findElement("agents") != NULL);
     _gc_capable     = (baseElement.findElement("groupchat") != NULL);
     _fetchrequested = false;
}

Agent::~Agent()
{}

void Agent::requestRegister(ElementCallbackFunc f)
{
     _session.queryNamespace("jabber:iq:register", f, _jid);
}

void Agent::requestSearch(ElementCallbackFunc f)
{
     _session.queryNamespace("jabber:iq:search", f, _jid);
}

void Agent::fetch()
{
     if ( _subagents && empty() && !_fetchrequested )
     {
	  // Release existing agents
	  clear();
	  // Set the fetch request flag
	  _fetchrequested = true;
	  // Request agents
	  _session.queryNamespace("jabber:iq:agents", slot(this, &Agent::on_fetch), _jid);
     }
}         

void Agent::on_fetch(const Element& iq)
{
     // Reset fetch request flag
     _fetchrequested = false;

     // IQ should contain an <iq> tag with one child tag <query>; within <query>,
     // there should be 1 or more <agent> tags -- these tags must be parsed into
     // agent objects and stored internally to "this" agent
     const Element* query = iq.findElement("query");
     if (query == NULL)
	  return;

     if (iq.cmpAttrib("type", "result"))
     {
	  // Walk the <query> tag's children
	  Element::const_iterator it = query->begin();
	  for (; it != query->end(); it++)
	  {
	       if ((*it)->getType() != Node::ntElement)
		    continue;
	       // Cast the child element into a tag
	       Element& t = *static_cast<Element*>(*it);
	       
	       // Ensure this is an agent tag
	       if (t.getName() == "agent")
		    push_back(Agent(t.getAttrib("jid"), t, _session));
	  }

	  // Notify any one-time observers that this agent has been updated successfully
	  evtFetchComplete(true);
     }
     else
     {
	  // Notify any observer that this agent has not been update successfully
	  evtFetchComplete(false);
     }

     // Release all one-time observers
     evtFetchComplete.clear();
}

AgentList::AgentList(const Element& query, Session& s)
{
     load(query, s);
}

void AgentList::load(const Element& query, Session& s)
{
     // Ensure list is clear
     clear();
     // MAke sure this is the proper namespace
     if (!query.cmpAttrib("xmlns", "jabber:iq:agents"))
	  return;

     // Walk each of the child <agent> tags and build agent objects
     Element::const_iterator it = query.begin();
     for (; it != query.end(); it++)
     {
	  if ((*it)->getType() != Node::ntElement)
	       continue;
	  // Cast the child element to a tag
	  Element& t = *static_cast<Element*>(*it);

	  // Ensure this is an <agent> tag
	  if (t.getName() == "agent")
	       // Construct and insert an agent
	       push_back(Agent(t.getAttrib("jid"), t, s));
     }
}
