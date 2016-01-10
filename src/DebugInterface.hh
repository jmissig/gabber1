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


#ifndef INCL_DEBUG_INTERFACE_HH
#define INCL_DEBUG_INTERFACE_HH

#include "BaseGabberWindow.hh"

#include <string>
using namespace std;

namespace Debug
{
     // Extraneous information to output for the curious user
     void Info(const string& text);
     // Something went wrong, but not too badly - the user doesn't need to know
     void Warning(const string& text);
     // Something went wrong, but we'll survive - the user might want to know
     void Error(const string& text);
     // Something went wrong, and we can't help it - impending problem
     void Death(const string& text);
};

class RawXMLInput
     : public BaseGabberDialog
{
public:
     static void execute();
     RawXMLInput();
protected:
     void on_Send_clicked();
     void on_Close_clicked();
     void on_Message_clicked();
     void on_IQ_clicked();
private:
     Gtk::Text* _memRaw;
};

#endif

