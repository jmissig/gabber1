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



#include "BaseGabberWindow.hh"
#include "GabberApp.hh"
#include "GladeHelper.hh"

// ---------------------------------------------------------
//
// Base Gabber Window
//
// ---------------------------------------------------------
BaseGabberWindow::BaseGabberWindow(const char* widgetname)
{ 
     _thisGH = G_App->load_resource(widgetname); 
     _thisWindow  = getWidgetPtr<Gtk::Window>(_thisGH, widgetname);
     reference();
}

void BaseGabberWindow::set_dynamic()
{
     SigC::Object::set_dynamic();
     set_sink();
}

void BaseGabberWindow::close()
{
     unreference();
}

BaseGabberWindow::~BaseGabberWindow()
{
     evtDestroy();
     _thisWindow->destroy();
     gtk_object_unref(GTK_OBJECT(_thisGH));
}

// ---------------------------------------------------------
//
// Base Gabber Dialog
//
// ---------------------------------------------------------
BaseGabberDialog::BaseGabberDialog(const char* widgetname, bool close_hides)
     : BaseGabberWindow(widgetname)
{
     _thisDialog = static_cast<Gnome::Dialog*>(_thisWindow);
     _thisDialog->close_hides(close_hides);
     if (!close_hides)
          _thisDialog->close.connect(slot(this, &BaseGabberDialog::on_Dialog_close));
}

gboolean BaseGabberDialog::on_Dialog_close()
{
     _thisWindow->destroy();
     return true;
}


// ---------------------------------------------------------
//
// Base Gabber Widget
//
// ---------------------------------------------------------
BaseGabberWidget::BaseGabberWidget(const char* widgetname, const char* filename)
{ 
     _thisGH = G_App->load_resource(widgetname, filename); 
     _thisWidget = getWidgetPtr<Gtk::Widget>(_thisGH, widgetname);
     reference();
}

void BaseGabberWidget::set_dynamic()
{
     SigC::Object::set_dynamic();
     set_sink();
}

void BaseGabberWidget::close()
{
     unreference();
}

BaseGabberWidget::~BaseGabberWidget()
{
     evtDestroy();
     _thisWidget->destroy();
     gtk_object_unref(GTK_OBJECT(_thisGH));
}
