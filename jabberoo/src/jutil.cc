/********************************************************************************
 *   Jabberoo/Judo -- C++ Jabber Library                                        *
 *                                                                              * 
 *   Copyright (C) 1999-2000 Dave Smith (dave@jabber.org)                       *
 *                                                                              *
 *   This library is free software; you can redistribute it and/or              *
 *   modify it under the terms of the GNU Lesser General Public                 *
 *   License as published by the Free Software Foundation; either               *
 *   version 2.1 of the License, or (at your option) any later version.         *
 *                                                                              *
 *   This library is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU          *
 *   Lesser General Public License for more details.                            *
 *                                                                              *
 *   You should have received a copy of the GNU Lesser General Public           *
 *   License along with this library; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  *
 ********************************************************************************
 */

#include "jutil.hh"
using namespace jutil;

#include <time.h>

#ifdef WIN32
#define snprintf _snprintf
#define strcasecmp stricmp
#include <string.h>
#elif defined(MACOS)
#include <ctype.h>
#include <stdio.h>

/* Compare lexigraphically two strings */

int strcasecmp(const char *s1, const char *s2)
{
    char c1, c2;
    while (1)
    {
    	c1 = tolower(*s1++);
    	c2 = tolower(*s2++);
        if (c1 < c2) return -1;
        if (c1 > c2) return 1;
        if (c1 == 0) return 0;
    }
}
#else
#include <stdio.h>
#endif

bool CaseInsensitiveCmp::operator()(const string& lhs, const string& rhs) const
{
     return (strcasecmp(lhs.c_str(), rhs.c_str()) < 0);
}

string jutil::getTimeStamp()
{
    time_t t;
    struct tm *new_time;
    static char timestamp[18];
    int ret;

    t = time(NULL);

    if(t == (time_t)-1)
        return NULL;
    new_time = gmtime(&t);

    ret = snprintf(timestamp, 18, "%d%02d%02dT%02d:%02d:%02d", 1900+new_time->tm_year,
                   new_time->tm_mon+1, new_time->tm_mday, new_time->tm_hour,
                   new_time->tm_min, new_time->tm_sec);

    if(ret == -1)
        return "";

    return string(timestamp);
}
