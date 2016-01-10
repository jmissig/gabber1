//============================================================================
// Project:       Jabber Universal Document Objects (Judo)
// Filename:      judo_test.cpp
// Description:   Unit test driver
// Created at:    Fri Jun 29 13:13:09 2001
// Modified at:   Fri Jul 13 11:21:50 2001
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
// $Id: judo_test.cpp,v 1.2 2002/03/05 22:23:31 julian Exp $
//============================================================================

#include "TestRunner.h"
#include "judo_test.hpp"

int main(int argc, char** argv)
{
    TestRunner r;

    // Load up all our test suites
    r.addTest("judo", judo::GlobalsTest::getTestSuite());
    r.addTest("judo::CDATA", judo::CDATATest::getTestSuite());
    r.addTest("judo::Element", judo::ElementTest::getTestSuite());
    r.addTest("judo::ElementStream", judo::ElementStreamTest::getTestSuite());

    // Start processing
    r.run(argc, argv);
    return 0;
}
