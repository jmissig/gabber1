/* jabberoox.hh
 * Jabber client library extensions
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

#ifndef INCL_JABBEROOX_HH
#define INCL_JABBEROOX_HH

namespace jabberoo
{
     class Filter
     {
     public:
	  class Action
	  {
	  public:
	       enum Value
	       {
		    Invalid = -1, SetType, ForwardTo, ReplyWith, StoreOffline, Continue
	       };
	       Action(Value v = SetType, const string& param = "")
		    : _value(v), _param(param) {}
	       // Accessors
	       bool requires_param()
		    { return Action::ParamReq[_value]; }
	       const string& param() const 
		    { return _param; }
	       Value value() const 
		    { return _value; }
	       string toXML() const;
	       // Mutators
	       Action& operator<<(Value v) 
		    { _value = v; return *this;}
	       Action& operator<<(const string& param) {
		    _param = param; return *this; }
	       // Translator
	       static Value translate(const string& s);
	  private:
	       static const bool ParamReq[5];
	       Value  _value;
	       string _param;
	  };

	  class Condition
	  {
	  public:
	       enum Value
	       {
		    Invalid = -1, Unavailable, From, MyResourceEquals, SubjectEquals, BodyEquals, ShowEquals, TypeEquals
	       };
	       Condition(Value v = Unavailable, const string& param = "")
		    : _value(v), _param(param) {}
	       // Accessors
	       bool  requires_param() 
		    { return Condition::ParamReq[_value]; }
	       const string& param() const 
		    { return _param; }
	       Value value() const 
		    { return _value; }
	       string toXML() const;
	       // Mutators
	       Condition& operator<<(Value v) 
		    { _value = v; return *this;}
	       Condition& operator<<(const string& param) 
		    { _param = param; return *this; }
	       // Translator
	       static Value translate(const string& s);
	  private:
	       static const bool ParamReq[7];
	       Value  _value;
	       string _param;
	  };

	  typedef list<Action> ActionList;
	  typedef list<Condition> ConditionList;

     public:
	  // Constructors
	  Filter(const string& name);
	  Filter(const Element& rule);
	  Filter(const Filter& f);
 	  bool operator==(const Filter& f) const { return &f == this; }
 	  bool operator==(const Filter& f) { return &f == this; }
	  // XML converter
	  string toXML() const;
	  // Accessors
	  ActionList&    Actions()    { return _actions; }
	  ConditionList& Conditions() { return _conditions; }
	  const string&  Name() const { return _name; }
	  void           setName(const string& newname) { _name = newname; }
     private:
	  ActionList    _actions;
	  ConditionList _conditions;
	  string _name;
     };

     class FilterList
	  : public list<Filter>
     {
     public:
	  FilterList(const Element& query);
	  string toXML() const;
     };

     class Agent;
     class Agent
	  : public list<Agent>, public SigC::Object
     {
     public:
	  Agent(const Agent& a);
	  Agent(const string& jid, const Element& baseElement, Session& s);
	  ~Agent();
     public:
	  // Events
	  Signal1<void, bool, Marshal<void> > evtFetchComplete;
	  // Accessors
	  const string& JID()         const { return _jid; };
	  const string& name()        const { return _name; };
	  const string& description() const { return _desc; };
	  const string& service()     const { return _service; };
	  const string& transport()   const { return _transport; };
	  // Info ops
	  bool isRegisterable() const { return _registerable; };
	  bool isSearchable()   const { return _searchable; };
	  bool isGCCapable()    const { return _gc_capable; };
	  bool hasAgents()      const { return _subagents; };
	  // Action ops
	  void requestRegister(ElementCallbackFunc f);
	  void requestSearch(ElementCallbackFunc f);
	  void fetch();
     protected:
	  void on_fetch(const Element& iq);
     private:
	  string   _jid;
	  string   _name;
	  string   _desc;
	  string   _service;
	  string   _transport;
	  bool     _registerable;
	  bool     _searchable;
	  bool     _subagents;
	  bool     _gc_capable;
	  bool     _fetchrequested;
	  Session& _session;
     };

     class AgentList
	  : public list<Agent>
     {
     public:
	  AgentList(const Element& query, Session& s);
	  void load(const Element& query, Session& s);
     };


};

#endif
