//============================================================================
// Project:       Jabber Universal Document Objects (Judo)
// Filename:      Element.cpp
// Description:   judo::Element
// Created at:    Mon Jul  2 17:12:54 2001
// Modified at:   Wed Oct 17 11:35:31 2001
// 
//   License:
// 
// The contents of this file are subject to the Jabber Open Source License
// Version 1.0 (the "License").  You may not copy or use this file, in either
// source code or executable form, except in compliance with the License.  You
// may obtain a copy of the License at http://www.jabber.com/license/ or at
// http://www.opensource.org/.  
//
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the License
// for the specific language governing rights and limitations under the
// License.
//
//   Copyrights
//
// Portions created by or assigned to Jabber.com, Inc. are 
// Copyright (c) 1999-2002 Jabber.com, Inc.  All Rights Reserved.  
// 
// $Id: Element.cpp,v 1.5 2002/03/07 21:55:52 julian Exp $
//============================================================================

#include "judo.hpp"
using namespace judo;
using namespace std;

// Internal node copier predicate
class P_NodeCopier : 
    public unary_function<Node*, void>
{
public:
    explicit P_NodeCopier(Element& parent) 
	: _parent(parent) 
	{}
    void operator()(const Node* n) const 
	{
	    if (n->getType() == Node::ntElement)
		_parent.appendChild(new Element(*static_cast<const Element*>(n)));
	    else
		_parent.appendChild(new CDATA(*static_cast<const CDATA*>(n)));
	}
private:
    Element& _parent;
};

/**
   Default constructor
   @param name Name of this tag
   @param attribs Expat-style attributes of this element
*/
Element::Element(const string& name, const char** attribs)
    : Node(name, Node::ntElement)
{
    // Process expat attribs
    if (attribs != NULL)
    {
	int i = 0;
	while (attribs[i] != '\0')
	{
	    _attribs.insert(make_pair(string(attribs[i]), string(attribs[i+1])));
	    i += 2;
	}
    }
}

// Copy constructor
Element::Element(const Element& e)
    : Node(e.getName(), Node::ntElement),
      _attribs(e._attribs)
{
    for_each(e._children.begin(), e._children.end(), P_NodeCopier(*this));
}

static void _releaseChild(Node* n)
{
    delete n;
}

Element::~Element()
{
    for_each(_children.begin(), _children.end(), _releaseChild);
}

Element& Element::operator=(const Element& e)
{
    // Check for assignment to self
    if (this == &e)
	  return *this;
    // Copy underlying members
    Node::operator=(e);

    // Release pre-existing children/attribs if necessary
    if (!_children.empty()) {
	 // Release child nodes
	 for_each(_children.begin(), _children.end(), _releaseChild);
	 _children.clear();
    }
    if (!_attribs.empty()) 
	 _attribs.clear();
     // Copy children/attribs from t
     if (!e._children.empty())
	  // Copy each individual element in t._Children
	  for_each(e._children.begin(), e._children.end(), P_NodeCopier(*this));
     if (!e._attribs.empty())
	  _attribs = e._attribs;

     return *this;
}

/**
   Add a new child element to "this" element.
   @param name Name of child element
   @param attribs Expat-style attributes of the new child element
   @returns Pointer to the new child element
*/
Element* Element::addElement(const string& name, const char** attribs)
{
    Element* result = new Element(name, attribs);
    _children.push_back(result);
    return result;
}

/**
   Add a new child element with a solitary section of character data. 
   @param name Name of child element
   @param value Character data which will be stored in new
   child element
   @param escaped Is the text escaped?
   @returns Pointer to the new child element
*/
Element* Element::addElement(const string& name, const string& value, bool escaped)
{
    Element* result = addElement(name);
    result->addCDATA(value.c_str(), value.size(), escaped);
    return result;
}


/**
   Add character data to "this" element. This automatically
   merges adjacent sections of CDATA (if the parser splits
   them up). 
   @param data Character data to add
   @param datasz Length of the character data
   @param escaped Is the text escaped?
   @returns Pointer to the object which holds the data
*/
CDATA* Element::addCDATA(const char* data, int datasz, bool escaped)
{
    CDATA *result;
    if (!_children.empty() && _children.back()->getType() == Node::ntCDATA)
    {
	result = static_cast<CDATA*>(_children.back());
	result->appendText(data, datasz, escaped);
    }
    else
    {
	result = new CDATA(data, datasz, escaped);
	_children.push_back(result);
    }
    return result;
}


/**
   Store an attribute (key/value pair) on this element.
   @param name Attribute name/key
   @value value Attribute value
*/
void Element::putAttrib(const string& name, const string& value)
{
    _attribs[name] = value;
}

/**
   Retrieve an attribute value.
   @param name Attribute name/key to retrieve
   @returns Value of the attribute
*/
string Element::getAttrib(const string& name) const
{
    map<string,string>::const_iterator it = _attribs.find(name);
    if (it != _attribs.end())
	return it->second;
    else
	return "";
}

/**
   Delete an attribute key/value
   @param name Attribute name/key to delete
*/
void Element::delAttrib(const string& name)
{
    _attribs.erase(name);
}

/**
   Compare an attribute value to a string
   @param name Attribute name/key to compare against
   @param value Value to compare against the attribute value
   @returns True if the user-provided value is the same as the
   value of the attribute
*/
bool Element::cmpAttrib(const string& name, const string& value) const
{
    map<string, string>::const_iterator it = _attribs.find(name);
    if (it != _attribs.end())
	return it->second == value;
    else
	return false;
}

/**
   Get a properly escaped XML string representation of this
   object.
   @see Node::toString
*/
string Element::toString() const
{
    string result;
    XMLAccumulator acc(result);

    accumulate(acc);

    return result;
}

/**
   Custom version of Element::toString. Permits non-recursive
   serialization of the Element to a properly escaped XML string
   representation. 
   @param recursive Determines if the Element is recursively
   serialization. Default is false.
   @param closetag Determines if the Element is closed after
   serialization. Default is false. 
   @see Node::toString
*/
string Element::toStringEx(bool recursive, bool closetag) const
{
    string result;

    XMLAccumulator acc(result);
    
    acc << "<" << getName();
    for_each(_attribs.begin(), _attribs.end(), acc);

    if (recursive && !_children.empty())
    {
	acc << ">";
	for_each(_children.begin(), _children.end(), acc);
	if (closetag)
	    acc << "</" << getName() << ">";
    }
    else
    {
	if (closetag)
	    acc << "/>";
	else
	    acc << ">";
    }
    return result;
}

/** 
    Accumulate a properly escaped XML string representation of
    this object
    @see Node::accumulate
*/
void Element::accumulate(XMLAccumulator& acc) const
{
    acc << "<" << getName();
    for_each(_attribs.begin(), _attribs.end(), acc);
    if (!_children.empty())
    {
	acc << ">";
	for_each(_children.begin(), _children.end(), acc);
	acc << "</" << getName() << ">";
    }
    else
    {
	acc << "/>";
    }
}

/** 
    Retrieve the first CDATA child of this element
    @returns a reference to the first CDATA string
*/
string Element::getCDATA() const
{
    list<Node*>::const_iterator it = _children.begin();
    for (; it != _children.end(); ++it)
    {
	if ((*it)->getType() == ntCDATA)
	    break;
    }
    if (it != _children.end())
	return static_cast<const CDATA*>(*it)->getText();
    else
	return string();
}

/**
   Locate the first child node which has the specified name and type.
   @param name Name of the child Node to find.
   @returns Iterator pointing to the child Node, or end() if no such
   child was found
*/
Element::iterator Element::find(const string& name, Node::Type type)
{
    iterator result = begin();
    for (; result != end(); ++result)
    {
	if (((*result)->getType() == Node::ntElement) && 
	    ((*result)->getName() == name))
	    break;
    }
    return result;
}

/**
   Locate the first child node which has the specified name and type.
   @param name Name of the child Node to find.
   @returns const Iterator pointing to the child Node, or end() if no such
   child was found
*/
Element::const_iterator Element::find(const string& name, Node::Type type) const
{
    const_iterator result = begin();
    for (; result != end(); ++result)
    {
        if (((*result)->getType() == Node::ntElement) &&
            ((*result)->getName() == name))
            break;
    }
    return result;
}


/**
   Locate the first child node which has the specified
   name and is an Element.
   @param name Name of the child Element to find.
   @returns Pointer to the matching child Element, or NULL if
   no such child was found
*/
Element* Element::findElement(const string& name)
{
    iterator result = find(name, Node::ntElement);
    if (result != end())
	return static_cast<Element*>(*result);
    else
	return NULL;
}

/**
   Locate the first child node which has the specified
   name and is an Element.
   @param name Name of the child Element to find.
   @returns Pointer to the matching child Element, or NULL if
   no such child was found
*/
const Element* Element::findElement(const string& name) const
{
    const_iterator result = find(name, Node::ntElement);
    if (result != end())
        return static_cast<const Element*>(*result);
    else
        return NULL;
}


/**
   Locate the first child Element which has the specified name and
   erase it. As a result of this, the child Element's sub-children are
   also removed.
*/
void Element::eraseElement(const string& name)
{
    iterator it = find(name, Node::ntElement);
    if (it != end())
	erase(it);
}

/**
   Detach a child node from this element.
   The iterator is invalidated after this call.
   @param it iterator pointing to child node pointer
   @return - node pointer of child node
*/
Node* Element::detachChild(iterator it)
{
    Node* retval = *it;
    _children.erase(it);
    return retval;
}

/**
   Locate the first child node which has the specified name
   and extract the CDATA from it
   @param name Name of the child Element to find.
   @returns String with CDATA value within the child Element,
   or empty string ("") if no such child was found
*/
std::string Element::getChildCData(const std::string& name) const
{
    const Element* child = findElement(name);
    return (child != NULL) ? child->getCDATA() : string();
}

/**
   Locate the first child node which has the specified name
   and extract the CDATA from it, converting the value into an
   integer
   @param name Name of the child Element to find
   @param default Default value of the integer (in case
   conversion fails)
   @returns Integer value of the CDATA value within the child
   Element, or the default value if no child was found or
   integer conversion failed
*/
int Element::getChildCDataAsInt(const std::string& name, int defaultvalue) const
{
    const judo::Element* child = findElement(name);
    if (child == NULL)
	return defaultvalue;

    string svalue = child->getCDATA();
    if (svalue.empty())
	return defaultvalue;

    char* endptr = NULL;
    int result = strtol(svalue.c_str(), &endptr, 10);
    // If endptr is the same as the original string, no valid digits were found
    if (endptr == svalue.c_str())
	return defaultvalue;

    return result;
}
