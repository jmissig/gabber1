//============================================================================
// Project:       Jabber Universal Document Objects (Judo)
// Filename:      GlobalsTest.cpp
// Description:   judo global function tests
// Created at:    Tue Jul  3 13:06:11 2001
// Modified at:   Mon Jul 30 18:10:54 2001
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
// $Id: GlobalsTest.cpp,v 1.2 2002/03/05 22:23:31 julian Exp $
//============================================================================

#include "judo.hpp"
#include "judo_test.hpp"
using namespace judo;
using namespace std;

Test* GlobalsTest::getTestSuite()
{
    TestSuite* s = new TestSuite();
    s->addTest(new TestCaller<GlobalsTest>("testing XML escaping",
					   &GlobalsTest::escape));
    s->addTest(new TestCaller<GlobalsTest>("testing XML unescape",
					   &GlobalsTest::unescape));
    return s;
}

const string G_escaped = "&amp; me &apos; you &quot; &lt; 23 &gt; 5";
const string G_unescaped = "& me ' you \" < 23 > 5";

void GlobalsTest::escape()
{
    Assert(G_escaped == judo::escape(G_unescaped));
}

void GlobalsTest::unescape()
{
    string target;

    judo::unescape(G_escaped.c_str(), G_escaped.size(), target);

    Assert(G_unescaped == target);
}
