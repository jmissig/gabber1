/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 *  Gabber
 *  Copyright (C) 1999-2001 Dave Smith & Julian Missig
 */

#ifndef INCL_ERROR_MANAGER_HH
#define INCL_ERROR_MANAGER_HH

#include "jabberoo.hh"

#include <sigc++/object_slot.h>
#include <sigc++/signal_system.h>

class ErrorManager
     : public SigC::Object
{
public:
     ErrorManager();
     void add(const jabberoo::Message& m);
     void add(const jabberoo::Presence& p);
     string translateError(int errorcode);

public:
     // concerning JabberID, ErrorCode, Error Message, message body
     SigC::Signal4<void, const string&, int, const string&, const string&> errorNormal;
     SigC::Signal4<void, const string&, int, const string&, const string&> errorChat;
     SigC::Signal4<void, const string&, int, const string&, const string&> errorGroupchat;
     SigC::Signal4<void, const string&, int, const string&, const string&> errorHeadline;
};

#endif
