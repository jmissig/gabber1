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


#ifndef INCL_GLADE_HELPER_HH
#define INCL_GLADE_HELPER_HH

#include <iostream>
#include <glade/glade-xml.h>
#include <gtk/gtkobject.h>
#include <gtk--/base.h>

template<class T> T* getWidgetPtr(GladeXML* g, const char* name)
{
     T* result = static_cast<T*>(Gtk::wrap_auto((GtkObject*)glade_xml_get_widget(g, name)));
     if (result == NULL)
     {
	  std::cerr << "** ERROR **: unable to load widget: " << name << std::endl;
	  g_assert(result != NULL);
     }
     return result;
}

template<class T> T* getWidgetPtr_GTK(GladeXML* g, const char* name)
{
     T* result = (T*)glade_xml_get_widget(g, name);
     if (result == NULL)
     {
	  std::cerr << "** ERROR **: unable to load widget: " << name << std::endl;
	  g_assert(result != NULL);
     }
     return result;     
}

#endif
