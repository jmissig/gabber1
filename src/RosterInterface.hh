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
 *  Copyright (C) 1999-2002 Dave Smith & Julian Missig
 */

#ifndef INCL_ROSTER_INTERFACE_HH
#define INCL_ROSTER_INTERFACE_HH

#include "jabberoo.hh"

#include "BaseGabberWindow.hh"
#include "GabberUtility.hh"

class RosterExportDlg :
     public Gtk::FileSelection
{
public:
     static void execute();
     // Destructor
     ~RosterExportDlg();
protected:
     void on_ok_clicked();
     void on_cancel_clicked();
     // Internalize default constructor
     RosterExportDlg();
private:
     static RosterExportDlg* _Dialog;
};

class RosterImportDlg :
     public Gtk::FileSelection
{
public:
     static void execute();
     // Destructor
     ~RosterImportDlg();
protected:
     void on_ok_clicked();
     void on_update_refresh();
     void on_cancel_clicked();
     // Internalize default constructor
     RosterImportDlg();
private:
     static RosterImportDlg* _Dialog;
};

class RosterLoader
     : private judo::ElementStreamEventListener,
       private judo::ElementStream
{
public:
     RosterLoader(const string& filename, const string& dummy);
     ~RosterLoader();
private:
     virtual void onDocumentStart(Element* e);
     virtual void onElement(Element* t);
     virtual void onDocumentEnd();
};

#endif
