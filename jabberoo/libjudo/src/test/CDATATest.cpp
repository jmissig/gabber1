//============================================================================
// Project:       Jabber Universal Document Objects (Judo)
// Filename:      CDATATest.cpp
// Description:   judo::CDATA unit tests
// Created at:    Tue Jul  3 13:06:11 2001
// Modified at:   Mon Jul 30 18:06:35 2001
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
// $Id: CDATATest.cpp,v 1.2 2002/03/05 22:23:31 julian Exp $
//============================================================================

#include "judo.hpp"
#include "judo_test.hpp"
using namespace judo;

#include <iostream>
using namespace std;

Test* CDATATest::getTestSuite()
{
    TestSuite* s = new TestSuite();
    s->addTest(new TestCaller<CDATATest>("testing construction",
					 &CDATATest::construct));
    s->addTest(new TestCaller<CDATATest>("testing setText",
					 &CDATATest::setText));
    s->addTest(new TestCaller<CDATATest>("testing appendText",
					 &CDATATest::appendText));
    s->addTest(new TestCaller<CDATATest>("testing getText",
					 &CDATATest::getText));
    s->addTest(new TestCaller<CDATATest>("testing toString",
					 &CDATATest::CDATAtoString));


    return s;
}

void CDATATest::construct()
{
    {
	CDATA c("Hello, world!", strlen("Hello, world!"));
	Assert(c._text == "Hello, world!");
    }

    {
	CDATA c("&amp; &apos; &quot; &lt; &gt;",strlen("&amp; &apos; &quot; &lt; &gt;"), true);
	cerr << c._text << endl;
	Assert(c._text == "& ' \" < >");
    }

}


void CDATATest::setText()
{
    CDATA c("Hello, world!", strlen("Hello, world!"));

    c.setText("Yo buddy!", 9);

    Assert(c._text == "Yo buddy!");
}

void CDATATest::appendText()
{
    CDATA c("Hello, world!", strlen("Hello, world!"));

    c.appendText(" Goodbye world!", 15);

    Assert(c._text == "Hello, world! Goodbye world!");
}

void CDATATest::getText()
{
    CDATA c("Hello, world!", strlen("Hello, world!"));

    Assert(c.getText() == "Hello, world!");
}

void CDATATest::CDATAtoString()
{
    CDATA c("Hello, world!", strlen("Hello, world!"));

    string s = c.toString();

    Assert(s == "Hello, world!");
}

