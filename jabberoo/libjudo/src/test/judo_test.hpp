//============================================================================
// Project:       Jabber Universal Document Objects (Judo)
// Filename:      judo_test.h
// Description:   Unit Tests Header
// Created at:    Tue Jul  3 13:06:11 2001
// Modified at:   Fri Jul  6 12:30:17 2001
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
// $Id: judo_test.hpp,v 1.1 2002/03/05 22:24:20 julian Exp $
//============================================================================

#ifndef INCL_JUDOTEST_H
#define INCL_JUDOTEST_H

#include "TestCaller.h"
#include "TestSuite.h"

namespace judo
{
    class GlobalsTest
	: public TestCase
    {
    public:
	GlobalsTest(const std::string& name)
	    : TestCase(name)
	    {}

	// Test suite generator
	static Test* getTestSuite();

	// Tests
	void escape();
	void unescape();
    };

    class CDATATest
        : public TestCase
    {
    public:
        CDATATest(const std::string& name)
            : TestCase(name)
            {}

        // Test suite generator
        static Test* getTestSuite();

	// Tests
	void construct();

	void setText();
	void appendText();
	void getText();

	void CDATAtoString();

    };

    class ElementTest
	: public TestCase
    {
    public:
	ElementTest(const std::string& name)
	    : TestCase(name)
	    {}

	// Test suite generator
	static Test* getTestSuite();

	// Tests
	void construct();
	void addElement();
        void findElement();
	void addCDATA();
        void detachChild();
	
	void getAttrib();
	void putAttrib();
	void delAttrib();
	void cmpAttrib();

	void ElementtoString();
	void ElementtoStringEx();

	void getCDATA();
    };

    class ElementStreamTest
	: public TestCase
    {
    public:
	ElementStreamTest(const std::string& name)
	    : TestCase(name)
	    {}

	// Test suite generator
	static Test* getTestSuite();

	// Tests
	void construct();
	void push();
	void parseAtOnce();
    };
};
#endif
