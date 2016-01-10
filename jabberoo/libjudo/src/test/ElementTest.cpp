//============================================================================
// Project:       Jabber Universal Document Objects (Judo)
// Filename:      ElementTest.cpp
// Description:   judo::Element unit tests
// Created at:    Tue Jul  3 13:06:11 2001
// Modified at:   Thu Aug 16 18:01:23 2001
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
// $Id: ElementTest.cpp,v 1.2 2002/03/05 22:23:31 julian Exp $
//============================================================================

#include "judo.hpp"
#include "judo_test.hpp"
using namespace judo;
using namespace std;

Test* ElementTest::getTestSuite()
{
    TestSuite* s = new TestSuite();
    s->addTest(new TestCaller<ElementTest>("testing construction",
				       &ElementTest::construct));
    s->addTest(new TestCaller<ElementTest>("testing addElement",
				       &ElementTest::addElement));
    s->addTest(new TestCaller<ElementTest>("testing addCDATA",
				       &ElementTest::addCDATA));
    s->addTest(new TestCaller<ElementTest>("testing putAttrib",
				       &ElementTest::putAttrib));
    s->addTest(new TestCaller<ElementTest>("testing getAttrib",
				       &ElementTest::getAttrib));
    s->addTest(new TestCaller<ElementTest>("testing delAttrib",
				       &ElementTest::delAttrib));
    s->addTest(new TestCaller<ElementTest>("testing cmpAttrib",
				       &ElementTest::cmpAttrib));
    s->addTest(new TestCaller<ElementTest>("testing toString",
				       &ElementTest::ElementtoString));
    s->addTest(new TestCaller<ElementTest>("testing toStringEx",
				       &ElementTest::ElementtoStringEx));
    s->addTest(new TestCaller<ElementTest>("testing getCDATA",
				       &ElementTest::getCDATA));
    s->addTest(new TestCaller<ElementTest>("testing findElement",
				       &ElementTest::findElement));
    s->addTest(new TestCaller<ElementTest>("testing detachChild",
                                       &ElementTest::detachChild));
    return s;
}

void ElementTest::construct()
{
    Element e("message");

    Assert(e.getName() == "message");
    Assert(e.getType() == Node::ntElement);

    {
	// Test copy constructor
	Element e1("message");
	e.putAttrib("to", "dizzyd");
	e.addElement("body", "Hello, dizzyd");
	e.addElement("subject", "Yo buddy");

	Element e2(e1);
	Assert(e1.toString() == e2.toString());
    }
}

void ElementTest::addElement()
{
    {
	Element e("message");
	
	Element* e1 = e.addElement("body");
	
	Assert(e._children.size() == 1);
	Assert(e._children.front() == (Element*)e1);
	Assert(e1->getType() == Node::ntElement);
	Assert(e1->getName() == "body");
    }
    // Test overloaded version (formerly addTaggedCDATA)
    {
	Element e("message");
	
	Element* e1 = e.addElement("body", "hello, world!");
	
	Assert(e._children.size() == 1);
	
	Assert(e._children.front() == (Element*)e1);
	
	Assert(e1->getType() == Node::ntElement);
	Assert(e1->getName() == "body");
	
	Assert(e1->_children.size() == 1);
	Assert(e1->_children.front()->getType() == Node::ntCDATA);
	Assert(e1->_children.front()->getName() == "#CDATA");
    }
}

void ElementTest::addCDATA()
{
    Element e("message");

    CDATA* c = e.addCDATA("hello, world!", strlen("hello, world!"));

    Assert(e._children.size() == 1);

    Assert(e._children.front() == (Element*)c);
    Assert(c->getType() == Node::ntCDATA);
    Assert(c->getName() == "#CDATA");
}

void ElementTest::putAttrib()
{
    Element e("message");
    e.putAttrib("name", "value");

    Assert(e._attribs.size() == 1);

    map<string,string>::iterator it = e._attribs.find("name");
    Assert(it != e._attribs.end());
    Assert(it->first  == "name");
    Assert(it->second == "value");
}

void ElementTest::getAttrib()
{
    Element e("message");

    e._attribs["name1"] = "value1";
    e._attribs["name2"] = "value2";
    e._attribs["name3"] = "value3";

    string result = e.getAttrib("name2");
    Assert(result == "value2");
}

void ElementTest::delAttrib()
{
    Element e("message");

    e._attribs["name1"] = "value1";
    e._attribs["name2"] = "value2";
    e._attribs["name3"] = "value3";

    e.delAttrib("name2");

    Assert(e._attribs.find("name2") == e._attribs.end());
}

void ElementTest::cmpAttrib()
{
    Element e("message");

    e._attribs["name1"] = "value1";
    e._attribs["name2"] = "value2";
    e._attribs["name3"] = "value3";

    Assert(e.cmpAttrib("name3", "value3"));
}
void ElementTest::ElementtoString()
{
    Element e("message");
    e.putAttrib("name1", "value1");
    e.putAttrib("name3", "value3");
    e.addElement("body", "Hullo, world!");

    Assert(e.toString() == "<message name1='value1' name3='value3'><body>Hullo, world!</body></message>");
}

void ElementTest::ElementtoStringEx()
{
    Element e("message");
    e.putAttrib("name1", "value1");
    e.putAttrib("name3", "value&&3");
    e.addElement("body", "Hullo, world!");

    Assert(e.toStringEx() == "<message name1='value1' name3='value&amp;&amp;3'>");
}

void ElementTest::getCDATA()
{
    Element e("message");
    Element* e1 = e.addElement("body", "Hullo, world");

    Assert(e1->getCDATA() == "Hullo, world");
}

void ElementTest::findElement()
{
    Element e("message");
    e.putAttrib("name1", "value1");
    Element *f = e.addElement("body", "this is a test");
    Element *g = e.addElement("subject", "greetings");
    e.addElement("body", "howdy");
    Assert (e.findElement("body") == f);
    Assert (e.findElement("subject") == g);
    Assert (e.findElement("topic") == NULL); 
};

void ElementTest::detachChild()
{
   Element* e = ElementStream::parseAtOnce("<foo><bar>baz</bar>fiz</foo>");
   Element::iterator it = e->begin();
   Node* bar = e->detachChild(it);
   Assert (bar);
   Assert (bar->getName() == "bar");
   Assert (bar->getType() == Node::ntElement);
   Assert (((Element*)bar)->getCDATA() == "baz");
   it = e->begin();
   Assert ((*it)->getType() == Node::ntCDATA);
   Assert (((CDATA*)*it)->getText() == "fiz");
   delete bar;
   delete e;
}
