//============================================================================
// Project:       Jabber Universal Document Objects (Judo)
// Filename:      ElementStream.cpp
// Description:   Expat parsing wrapper
// Created at:    Fri Jun 29 13:13:09 2001
// Modified at:   Mon Jul 30 18:10:20 2001
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
// $Id: ElementStream.cpp,v 1.2 2002/03/05 22:23:31 julian Exp $
//============================================================================

#include "judo.hpp"
using namespace judo;
using namespace std;


ElementStream::ElementStream(ElementStreamEventListener* l)
    : _parser_ready(false), _event_listener(l)
{
    reset();
}

ElementStream::~ElementStream()
{
    XML_ParserFree(_parser);
}

/**
   Push data into the stream for parsing
   @param data Data to parse
   @param datasz Length of data to parse
*/
void ElementStream::push(const char* data, int datasz)
{
    assert(_document_ended != true);
    if (!XML_Parse(_parser, data, datasz, 0))
    {
	throw exception::ParserError(XML_GetErrorCode(_parser));
    }
}

/**
   Reset the parser for a new XML document/stream
*/
void ElementStream::reset()
{
    // Reset status flags
    _document_started = false;
    _document_ended = false;

    // Release the current parser and create
    // a new one
    if (_parser_ready)
	XML_ParserFree(_parser);
    _parser = XML_ParserCreate(NULL);
    _parser_ready = true;
    
    // Setup callbacks
    XML_SetUserData(_parser, this);
    XML_SetElementHandler(_parser, &ElementStream::onStartElement, 
			  &ElementStream::onEndElement);
    XML_SetCharacterDataHandler(_parser, &ElementStream::onCDATA);
}

void ElementStream::onStartElement(void* userdata, const char* name, const char** attribs)
{
    ElementStream* ts = (ElementStream*)userdata;

    // If the document has started..
    if (ts->_document_started)
    {
	if (!ts->_element_stack.empty())
	    ts->_element_stack.push_back(ts->_element_stack.back()->addElement(name, attribs));
	else
	    ts->_element_stack.push_back(new Element(name, attribs));
    }
    // Document hasn't started; we need to generate a document start
    // event
    else
    {
	Element* root = new Element(name, attribs);
	ts->_document_started = true;
	ts->_event_listener->onDocumentStart(root);	
    }
}

void ElementStream::onEndElement(void* userdata, const char* name)
{
    ElementStream* ts = (ElementStream*)userdata;

    switch (ts->_element_stack.size())
    {
	// Only one remaining element on the stack; thus we must be
	// closing the packet-level element
    case 1:
	ts->_event_listener->onElement(ts->_element_stack.back());
	ts->_element_stack.pop_back();
	break;
	// No packet-level elements currently being built; thus the
	// document must be closing
    case 0:
	ts->_event_listener->onDocumentEnd();
	ts->_document_ended = true;
	break;
    default:
	ts->_element_stack.pop_back();
    }
}

void ElementStream::onCDATA(void* userdata, const char* cdata, int cdatasz)
{
    ElementStream* ts = (ElementStream*)userdata;

    if (!ts->_element_stack.empty())
	ts->_element_stack.back()->addCDATA(cdata, cdatasz, true);
    else
	ts->_event_listener->onCDATA(new CDATA(cdata, cdatasz, true));
}


class BufferParser
    : public ElementStreamEventListener
{
public:
    BufferParser()
	: _finished(false), _root(NULL), _stream(this)
	{}
    ~BufferParser()
	{ 
	    if (!_finished && _root != NULL)
		delete _root;
	}
    Element* process(const char* data)
	{
	    _stream.push(data, strlen(data));
	    if (!_finished)
		throw ElementStream::exception::IncompleteParse();
	    return _root;
	}
    void onDocumentStart(Element* t)
	{ _root = t; }
    void onElement(Element* t)
	{ _root->appendChild(t); }
    void onCDATA(CDATA* c)
	{ _root->appendChild(c); }
    void onDocumentEnd()
	{ _finished = true; }
    bool isFinished()
	{ return _finished; }
private:
    bool _finished;
    Element* _root;
    ElementStream _stream;
};

/** 
    Parse a single buffer of XML.
*/
Element* ElementStream::parseAtOnce(const char* data)
{
    BufferParser p;
    return p.process(data);
}

