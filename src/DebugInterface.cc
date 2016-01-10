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

#include "DebugInterface.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"

#include <gtk--/toolbar.h>

using namespace jabberoo;
using namespace GabberUtil;

// ---------------------------------------------------------
//
// Debug Output
//
// ---------------------------------------------------------

// Extraneous information to output for the curious user
void Debug::Info(const string& text)
{
     cerr << ConfigManager::get_PACKAGE() << ":info: " << text << endl;
}

// Something went wrong, but not too badly - the user doesn't need to know
void Debug::Warning(const string& text)
{
     cerr << ConfigManager::get_PACKAGE() << ":warning: " << text << endl;
}

// Something went wrong, but we'll survive - the user might want to know
void Debug::Error(const string& text)
{
     cerr << ConfigManager::get_PACKAGE() << ":error: " << text << endl;
}

// Something went wrong, and we can't help it - impending problem
void Debug::Death(const string& text)
{
     cerr << ConfigManager::get_PACKAGE() << ":death: " << text << endl;
}


// ---------------------------------------------------------
//
// Raw XML Input
//
// ---------------------------------------------------------

void RawXMLInput::execute()
{
     manage(new RawXMLInput());
}

RawXMLInput::RawXMLInput()
     : BaseGabberDialog("RawXML_dlg")
{
     _thisWindow->realize();

     getButton("RawXML_Send_btn")->clicked.connect(slot(this, &RawXMLInput::on_Send_clicked));
     getButton("RawXML_Close_btn")->clicked.connect(slot(this, &RawXMLInput::on_Close_clicked));
     getButton("RawXML_Message_btn")->clicked.connect(slot(this, &RawXMLInput::on_Message_clicked));
     getButton("RawXML_IQ_btn")->clicked.connect(slot(this, &RawXMLInput::on_IQ_clicked));

     _memRaw = getWidget<Gtk::Text>("RawXML_Input_txt");
     _memRaw->set_word_wrap(true);

     // Toolbar
     getWidget<Gtk::Toolbar>("RawXML_toolbar")->set_style(GTK_TOOLBAR_ICONS);

     _thisWindow->set_default_size(325, 300);
     _thisWindow->show();
     main_dialog(_thisDialog);

     // Display
     show();
}

void RawXMLInput::on_Send_clicked()
{
     _memRaw->freeze();
     string rawxml = toUTF8(_memRaw, _memRaw->get_chars(0, -1));
     if(!rawxml.empty())
     {
	  G_App->getSession() << rawxml.c_str();

	  _memRaw->delete_text(0, -1);
	  // Can't do in gtkmm :(
	  gtk_signal_emit_by_name(GTK_OBJECT(_memRaw->gtkobj()), "activate");
     }
     _memRaw->thaw();
}

void RawXMLInput::on_Close_clicked()
{
     close();
}

void RawXMLInput::on_Message_clicked()
{
     int i = _memRaw->get_length();
     string blankmessage = "<message type='' to=''>\n"
	                   "<body>\n"
	                   "</body>\n"
                           "</message>\n";
     _memRaw->insert_text(blankmessage.c_str(),
			  blankmessage.length(), &i);
}

void RawXMLInput::on_IQ_clicked()
{
     int i = _memRaw->get_length();
     string id = G_App->getSession().getNextID();
     string blankiq = "<iq id='" + id + "' type='' to=''>\n"
	              "</iq>";
     _memRaw->insert_text(blankiq.c_str(),
			  blankiq.length(), &i);
}
