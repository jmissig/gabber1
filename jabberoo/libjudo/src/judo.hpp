//============================================================================
// Project:       Jabber Universal Document Objects (Judo)
// Filename:      judo.h
// Description:   Primary header
// Created at:    Mon Jul  2 17:27:04 2001
// Modified at:   Tue Oct 16 10:28:40 2001
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
// Copyright (c) 1999-2001 Jabber.com, Inc.  All Rights Reserved.  
// 
// $Id: judo.hpp,v 1.2 2002/03/07 21:55:52 julian Exp $
//============================================================================

#ifndef INCL_JUDO_H
#define INCL_JUDO_H

#ifdef WIN32
#pragma warning (disable:4786)
#include <windows.h>
#endif

#include <assert.h>
#include "expat/expat.h"

#include <cstdio>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <set>
#include <algorithm>

#ifdef TEST
#define TESTER(s) friend class s;
#else
#define TESTER(s)
#endif

/**
   Core namespace for Jabber Universal Document Objects
*/
namespace judo
{
    class XMLAccumulator;

    /**
       Parent class for all XML objects
    */
    class Node
    {
    public:
        enum Type
        {
            ntElement,
            ntCDATA
        };

        /**
           Primary constructor
           @param name Name of this element
           @param type Node type classifier
        */
        Node(const std::string& name, Type ntype)
	    : _name(name), _type(ntype)
	    {}
        virtual ~Node() {}
    public:
        /**
           Accessor for the nodes name
           @return Reference to the name of the node
        */
        const std::string& getName() const
	    { return _name; }

        /**
           Accessor for the Nodes type
           @return The type of the Node (Element CDATA)
        */
        Node::Type getType() const
	    { return _type; }

        /**
           Accessor for the XML string representation of the
           Node. This is a virtual abstract method that is
           implemented by the child classes. 
           @return The XML representation of an Node
        */
        virtual std::string toString() const = 0;

	/**
	   Accumulator entry point for recursive XML
	   serialization. This is a virtual abstract method that is
	   implemented by subclasses.
	   @param acc An instance of an XMLAccumulator
	*/
	virtual void accumulate(XMLAccumulator& acc) const = 0;

    protected:
        std::string        _name;
        Node::Type _type;    

        // Ensure that no one can initialize this variable without
        // using the proper constructor
        Node();
    };

    // Utility routines
    void   unescape(const char* src, unsigned int srcLen, std::string& dest, bool append = false);  
    std::string escape(const std::string& src);

    class XMLAccumulator
    {
    public:
	XMLAccumulator(std::string& s)
	    : _result(s) {}

	void operator()(const Node* n)
	    { n->accumulate(*this); }
	void operator()(const std::pair<const std::string, std::string> p)
	    { _result += " " + p.first + "='" + 
		  escape(p.second) + "'";
	    }
	template <class T>
	XMLAccumulator& operator<<(T data)
	    { _result += data; return *this; }
    private:
	std::string& _result;
    };

    /**
       Character data storage class
    */
    class CDATA : 
        public Node
    {
    public:
        /**
           Default constructor.
        */
        CDATA(const char* text, unsigned int textsz, bool escaped = false)
            : Node("#CDATA", Node::ntCDATA)
	    {
		if (escaped)
		{
		    unescape(text, textsz, _text);
		}
		else
		{
		    _text.assign(text, textsz);
		}
	    }
	
        /**
           Overwrite all existing character data in this object with
           specified text
           @param text New character data to store
           @param textsz Length of new character data
	   @param escaped Is the text escaped?
        */
        void setText(const char* text, unsigned int textsz, bool escaped = false)
	    { 
		if (escaped)
		{
		    unescape(text, textsz, _text); 
		}
		else
		{
		    _text.assign(text, textsz);
		}
	    }

        /**
           Append new character data to the existing text in this
           object
           @param text New character data to store
           @param textsz Length of new character data
	   @param escaped Is the text escaped?
        */
        void appendText(const char* text, unsigned int textsz, bool escaped = false)
	    { 
		if (escaped)
		{
		    unescape(text, textsz, _text, true); 
		}
		else
		{
		    _text.append(text, textsz);
		}
	    }

        /**
           Get a read-only reference to the character data in
           this object. Note that this data is not properly escaped
           XML and is not suitable for passing to an XML parser.
        */
        const std::string& getText() const
            { return _text; }

        /**
           Get a properly escaped XML string representation of this
           object.
           @see Node::toString
        */
        std::string toString() const
            { return escape(_text); }

	void accumulate(XMLAccumulator& acc) const
	    { acc << escape(_text); }

    private:
	TESTER(CDATATest)

        std::string _text;           
    };

    /** 
        XML Element representation class
    */
    class Element: 
        public Node
    {
    public:
        Element(const std::string& name, const char** attribs = NULL);
	Element(const Element& e);
        ~Element();

        Element& operator=(const Element& e);

        Element* addElement(const std::string& name, const char** attribs = NULL);
	Element* addElement(const std::string& name, const std::string& cdata, 
			    bool escaped = false);
        CDATA*   addCDATA(const char* data, int datasz, bool escaped = false);	

        void   putAttrib(const std::string& name, const std::string& value);
        std::string getAttrib(const std::string& name) const;
        void   delAttrib(const std::string& name);
        bool   cmpAttrib(const std::string& name, const std::string& value) const;

        std::string toString() const;
	std::string toStringEx(bool recursive = false, bool closetag = false) const;

	void accumulate(XMLAccumulator& acc) const;

	std::string getCDATA() const;
	

	typedef std::list<Node*>::iterator iterator;
	typedef std::list<Node*>::const_iterator const_iterator;

	/**
	   Append child node to this Element
	   @param child child node pointer
	*/
	void appendChild(Node* child)
	    { _children.push_back(child); }
        
        Node* detachChild(iterator it);
        
	/** 
	    Determine if this Element is empty (i.e. has no child nodes)
	    @returns True if no child nodes exist
	*/
	bool empty() const
	    { return _children.empty(); }

	/**
	   Determine the number of child nodes this Element
	   has. Prefer @Element::empty to comparing size() == 0
	   @returns Number of child nodes in this Element
	*/
	int size() const
	    { return _children.size(); }
	
	/**
	   Return an iterator to the first child Node.
	*/
	iterator begin()
	    { return _children.begin(); }

	/**
	   Return a const iterator to the first child Node.
	*/
	const_iterator begin() const
	    { return _children.begin(); }

	/**
	   Return a iterator indicating the end of child nodes.
	*/
	iterator end()
	    { return _children.end(); }

	/**
	   Return a const iterator indicating the end of child nodes.
	*/
	const_iterator end() const
	    { return _children.end(); }

	iterator find(const std::string& name, Node::Type type = Node::ntElement);

        const_iterator find(const std::string& name, Node::Type type = Node::ntElement) const;
	/**
	   Delete the child Node designated by the supplied iterator.
	   @param it Iterator pointing to the child Node 
	*/
	void Element::erase(Element::iterator it)
	    { delete *it; _children.erase(it); }

	Element* findElement(const std::string& name);
	const Element* findElement(const std::string& name) const;
	void     eraseElement(const std::string& name);

	std::string getChildCData(const std::string& name) const;
	int         getChildCDataAsInt(const std::string& name, int defaultvalue) const;


    protected:
	TESTER(ElementTest)
	
        std::list<Node*>        _children;
        std::map<std::string,std::string> _attribs;
    };
    
    /**
       XML parser event interface
    */
    class ElementStreamEventListener
    {
    public:
	virtual ~ElementStreamEventListener() {}
	/**
	   Event point for XML document start event. Override this method
	   to get notified when the XML document is starting
	   @param e Root document element. Be sure to delete this once you
	   are done with it
	*/
	virtual void onDocumentStart(Element* e) = 0;

	/**
	   Event point for immediate children of XML document
	   root. Override this method to get notified when a complete
	   child Element has been parsed.
	   @param e Child element. Be sure to delete this once you are
	   done with it
	*/
	virtual void onElement(Element* e) = 0;

	/**
	   Event point for immediate CDATA of an XML document
	   root. Override this method to get notified when CDATA is
	   encountered immediately below the root document. This is a
	   fairly rare condition within most Jabber processing, but is
	   required for those rare cases. If you don't override this
	   method, the CDATA is automatically cleaned up for you.
	   @param c CDATA node. Be sure to delete this once you are
	   done with it.
	*/
	virtual void onCDATA(CDATA* c) 
	    { delete c; }
	    

	/**
	   Event point for XML document end event. Override this
	   method to get notified when the XML document is ending
	*/
	virtual void onDocumentEnd() = 0;
    };
    

    /**
       XML parser wrapper class
    */
    class ElementStream 
    {
    public:
	ElementStream(ElementStreamEventListener* l);
	virtual ~ElementStream();

	void push(const char* data, int datasz);
	void reset();

	static Element* parseAtOnce(const char* buffer);

	struct exception
	{
	    class ParserError 
		{
		public:
		    ParserError(int code)
			: _code(code), _message(XML_ErrorString(code))
			{}
		    const std::string& getMessage() const
			{ return _message; }
		    int getCode() const
			{ return _code; }
		private:
		    int _code;
		    std::string _message;
		};
	    class IncompleteParse{};
	};

    private:
	TESTER(ElementStreamTest)

	XML_Parser     _parser;
	bool           _parser_ready;
	std::list<Element*> _element_stack;
	bool           _document_started;
	bool           _document_ended;

	ElementStreamEventListener* _event_listener;

	// Expat callbacks
	static void onStartElement(void* userdata, const char* name, const char** attribs);
	static void onEndElement(void* userdata, const char* name);
	static void onCDATA(void* userdata, const char* cdata, int cdatasz);

	// Expat initializers
	void initExpat();
	void cleanupExpat();
       };
}
#endif
