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

#include "RosterInterface.hh"

#include "GabberApp.hh"
#include "GabberConfig.hh" // for _()
#include "GabberUtility.hh"

#include <libgnome/gnome-i18n.h>

using namespace judo;
using namespace jabberoo;
using namespace GabberUtil;

// ---------------------------------------------------------
//
// Roster Export
//
// ---------------------------------------------------------

RosterExportDlg* RosterExportDlg::_Dialog = NULL;

RosterExportDlg::RosterExportDlg()
     : FileSelection(_("Export Jabber Roster - Gabber"))
{
     // Attach buttons
     get_ok_button()->clicked.connect(slot(this, &RosterExportDlg::on_ok_clicked));
     get_cancel_button()->clicked.connect(slot(this, &RosterExportDlg::on_cancel_clicked));

     // Set default filename
     set_filename("gabber.roster");

     show();
}

void RosterExportDlg::on_ok_clicked()
{
      string filename = get_filename();
     
      // Save the file
      ofstream file(filename.c_str());
      if (!file)
      {
	   Gnome::Dialog* d = manage(Gnome::Dialogs::warning(_("Unable to save file. Please try another name or location.")));
	   d->set_modal(true);
      }
      else
      {
	   // <jabber> root
	   Element jabber("jabber");
	   jabber.putAttrib("xmlns", "jabber:client");
	   // iq set
	   Element* iq = jabber.addElement("iq");
	   iq->putAttrib("type", "set");
	   // query of jabber:iq:roster namespace
	   Element* query = iq->addElement("query");
	   query->putAttrib("xmlns", "jabber:iq:roster");
	   
	   // Run through every roster item
	   for(Roster::iterator i = G_App->getSession().roster().begin(); i != G_App->getSession().roster().end(); i++)
	   {
		// An item element for this user
		Element* item = query->addElement("item");
		// Insert item specifics
		item->putAttrib("jid", i->getJID());
		if (!i->getNickname().empty())
		     item->putAttrib("name", i->getNickname());
		// Add all groups in this item
		for (Roster::Item::iterator it = i->begin(); it != i->end(); it++)
		     item->addElement("group", *it);
	   }

	   // Actually insert it all into a string
	   file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	   file << "<!--Exported Roster from Gabber-->" << endl;
	   file << endl;
	   file << jabber.toString() << endl;

	   // Get rid of file selection dialog
	   hide();
	   destroy();
      }
}

void RosterExportDlg::on_cancel_clicked()
{
     // Get rid of file selection dialog
     hide();
     destroy();
}

RosterExportDlg::~RosterExportDlg()
{
     _Dialog = NULL;
}

void RosterExportDlg::execute()
{
     if (_Dialog == NULL)
	  _Dialog = manage(new RosterExportDlg());
}


// ---------------------------------------------------------
//
// Roster Import
//
// ---------------------------------------------------------

RosterImportDlg* RosterImportDlg::_Dialog = NULL;

RosterImportDlg::RosterImportDlg()
     : FileSelection(_("Import Jabber Roster - Gabber"))
{
     // Attach buttons
     get_ok_button()->clicked.connect(slot(this, &RosterImportDlg::on_ok_clicked));
     get_cancel_button()->clicked.connect(slot(this, &RosterImportDlg::on_cancel_clicked));

     // Set default filename
     set_filename("gabber.roster");

     show();
}

void RosterImportDlg::on_ok_clicked()
{
      string filename = get_filename();
     
      // Save the file
      ifstream file(filename.c_str());
      if (!file)
      {
	   Gnome::Dialog* d = manage(Gnome::Dialogs::warning(_("Unable to open file. Please try another name or location.")));
	   d->set_modal(true);
      }
      else
      {
	   RosterLoader(filename, "foo");

	   // Get rid of file selection dialog
	   hide();
	   destroy();
      }
}

void RosterImportDlg::on_update_refresh()
{

}

void RosterImportDlg::on_cancel_clicked()
{
     // Get rid of file selection dialog
     hide();
     destroy();
}

RosterImportDlg::~RosterImportDlg()
{
     _Dialog = NULL;
}

void RosterImportDlg::execute()
{
     if (_Dialog == NULL)
	  _Dialog = manage(new RosterImportDlg());
}

// --------------------------------------
//
// RosterLoader
//
// --------------------------------------
RosterLoader::RosterLoader(const string& filename, const string& dummy)
     : ElementStream(this)
{
     // Open an istream 
     ifstream spoolfile(filename.c_str());
     if (spoolfile)
     { 
	  // Feed all istream data into the parser
	  char buf[4096];
	  spoolfile.getline(buf, 4096);
	  while (spoolfile.gcount() != 0)
	  {
	       push(buf, strlen(buf));
	       spoolfile.getline(buf, 4096);
	  }
     }
}

RosterLoader::~RosterLoader()
{
}

void RosterLoader::onDocumentStart(Element* e)
{
}

void RosterLoader::onElement(Element* t)
{
     Element& tref = *t;
     // Check for the iq
     if (tref.getName() == "iq")
     {
	  Packet p(tref);
	  G_App->getSession() << p;
     }
     else if (tref.getName() == "item")
     {
	  Packet p(tref);
	  G_App->getSession() << "<iq type='set'><query xmlns='jabber:iq:roster'>";
	  G_App->getSession() << p;
	  G_App->getSession() << "</query></iq>";
     }
     delete t;
}

void RosterLoader::onDocumentEnd()
{
}
