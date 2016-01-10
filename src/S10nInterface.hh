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

#ifndef INCL_S10N_INTERFACE_HH
#define INCL_S10N_INTERFACE_HH

#include "jabberoo.hh"

#include "BaseGabberWindow.hh"

class S10nReceiveDlg
     : public BaseGabberDialog
{
public:
     ~S10nReceiveDlg();
public:
     S10nReceiveDlg(const jabberoo::Presence& p);
     static void execute(const jabberoo::Presence& p);
     void on_Yes_clicked();
     void on_No_clicked();
     void on_UserInfo_clicked();
     void on_message_user();
protected:
     jabberoo::Presence _Info;
};

#endif
