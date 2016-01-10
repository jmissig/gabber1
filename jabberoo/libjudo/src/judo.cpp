//============================================================================
// Project:       Jabber Universal Document Objects (Judo)
// Filename:      judo.cpp
// Description:   Utility routines
// Created at:    Fri Jun 29 11:37:01 2001
// Modified at:   Mon Jul 30 18:09:58 2001
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
// $Id: judo.cpp,v 1.2 2002/03/05 22:23:31 julian Exp $
//============================================================================

#include "judo.hpp"
using namespace std;

void judo::unescape(const char* src, unsigned int srcLen, string& dest, bool append)
{
    unsigned int i, j;
    int len;

    // Setup string size
    if (append)
    {
	len = j = dest.length();
	dest.resize(len + srcLen);
    }
    else
    {
	j = 0;
	len = 0;
	dest.resize(srcLen);
    }

    // Walk the input text, unescaping as we go..
    for (i = 0; i < srcLen; i++)
    {
	// See if this is an escape character
	if (src[i] == '&')
	{
	    if (strncmp(&src[i+1],"amp;",4)==0)
	    {
		dest[j] = '&';
		i += 4;
	    } else if (strncmp(&src[i+1],"quot;",5)==0) {
		dest[j] = '\"';
		i += 5;
	    } else if (strncmp(&src[i+1],"apos;",5)==0) {
		dest[j] = '\'';
		i += 5;
	    } else if (strncmp(&src[i+1],"lt;",3)==0) {
		dest[j] = '<';
		i += 3;
	    } else if (strncmp(&src[i+1],"gt;",3)==0) {
		dest[j] = '>';
		i += 3;
	    } else {
		dest[j] = src[i];
	    }
	}
	// Not an escape character, so just copy the 
	// exact character
	else
	    dest[j] = src[i];
	j++;
	len++;
    }
    // Cleanup
    dest.resize(len);
}

string judo::escape(const string& src)
{
    int i,j,oldlen,newlen;

    if (src.empty())
	return string(src);

    oldlen = newlen = src.length();
    for(i = 0; i < oldlen; i++)
    {
	switch(src[i])
	{
	case '<':
	case '>':
	    newlen+=4; break;
	case '&' : 
	    newlen+=5; break;
	case '\'': 
	case '\"': 
	    newlen+=6; break;
	}
    }

    if(oldlen == newlen) 
	return string(src);
	
    string result;
    result.reserve(newlen);
	
    for(i = j = 0; i < oldlen; i++)
    {
	switch(src[i])
	{
	case '&':
	    result += "&amp;"; break;
	case '\'':
	    result += "&apos;"; break;
	case '\"':
	    result += "&quot;"; break;
	case '<':
	    result += "&lt;"; break;
	case '>':
	    result += "&gt;"; break;
	default:
	    result += src[i];
	}
    }
    return result;
}
