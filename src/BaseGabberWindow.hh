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


#ifndef INCL_BASE_GABBER_WINDOW_H
#define INCL_BASE_GABBER_WINDOW_H

#include "GladeHelper.hh"

#include <sigc++/signal_system.h>
#include <sigc++/object_slot.h>
#include <sigc++/marshal.h>
#include <glade/glade-xml.h>
#include <gtk--/box.h>
#include <gtk--/button.h>
#include <gtk--/checkbutton.h>
#include <gtk--/ctree.h>
#include <gtk--/entry.h>
#include <gtk--/eventbox.h>
#include <gtk--/frame.h>
#include <gtk--/label.h>
#include <gtk--/menuitem.h>
#include <gtk--/optionmenu.h>
#include <gtk--/text.h>
#include <gtk--/widget.h>
#include <gtk--/window.h>
#include <gnome--/dialog.h>
#include <gnome--/entry.h>
#include <gnome--/pixmap.h>
#include <gnome--/pixmapmenuitem.h>

using namespace SigC;

class BaseGabberWindow
     : public SigC::Object
{
public:
     BaseGabberWindow(const char* widgetname);
     virtual ~BaseGabberWindow();
     void show() { _thisWindow->show(); }
     void hide() { _thisWindow->hide(); }
     virtual void close(); 
     // Object extender
     virtual void set_dynamic();
     // Destruction signal
     Signal0<void, Marshal<void> > evtDestroy;
protected:
     BaseGabberWindow();
     // FIXME: Should make this function properly copy
     BaseGabberWindow& operator=(const BaseGabberWindow&) { return *this;}
     BaseGabberWindow(const BaseGabberWindow&) {}

public:
     // Accessor
     Gtk::Window*          getBaseWindow()                  { return _thisWindow; }

     // Helper functions
     Gtk::Button*          getButton(const char* name)      { return getWidgetPtr<Gtk::Button>(_thisGH, name); }
     Gtk::CheckButton*     getCheckButton(const char* name) { return getWidgetPtr<Gtk::CheckButton>(_thisGH, name); }
     Gtk::CTree*           getCTree(const char* name)       { return getWidgetPtr<Gtk::CTree>(_thisGH, name); }
     Gtk::Entry*           getEntry(const char* name)       { return getWidgetPtr<Gtk::Entry>(_thisGH, name); }
     Gtk::EventBox*        getEventBox(const char* name)    { return getWidgetPtr<Gtk::EventBox>(_thisGH, name); }
     Gtk::Frame*           getFrame(const char* name)       { return getWidgetPtr<Gtk::Frame>(_thisGH, name); }
     Gtk::HBox*            getHBox(const char* name)        { return getWidgetPtr<Gtk::HBox>(_thisGH, name); }
     Gnome::Entry*         getGEntry(const char* name)      { return getWidgetPtr<Gnome::Entry>(_thisGH, name); }
     Gtk::Label*           getLabel(const char* name)       { return getWidgetPtr<Gtk::Label>(_thisGH, name); }
     Gtk::MenuItem*        getMenuItem(const char* name)    { return getWidgetPtr<Gtk::MenuItem>(_thisGH, name); }
     Gtk::OptionMenu*      getOptionMenu(const char* name)  { return getWidgetPtr<Gtk::OptionMenu>(_thisGH, name); }
     Gnome::Pixmap*        getPixmap(const char* name)      { return getWidgetPtr<Gnome::Pixmap>(_thisGH, name); }
     Gtk::PixmapMenuItem*  getPixmapMenuItem(const char* name) { return getWidgetPtr<Gtk::PixmapMenuItem>(_thisGH, name); }
     Gtk::Text*            getText(const char* name)        { return getWidgetPtr<Gtk::Text>(_thisGH, name); }
     Gtk::VBox*            getVBox(const char* name)        { return getWidgetPtr<Gtk::VBox>(_thisGH, name); }
     template <class T> T* getWidget(const char* name)      { return getWidgetPtr<T>(_thisGH, name); }
     template <class T> T* getWidget_GTK(const char* name)  { return getWidgetPtr_GTK<T>(_thisGH, name); }

protected:     
     // Internal refs
     Gtk::Window*  _thisWindow;
private:
     GladeXML*     _thisGH;
};

class BaseGabberDialog
     : public BaseGabberWindow
{
public:
     BaseGabberDialog(const char* widgetname, bool close_hides = false);
     virtual ~BaseGabberDialog() {}
     Gnome::Dialog* getBaseDialog() { return _thisDialog; }
protected:
     Gnome::Dialog* _thisDialog;
     gboolean on_Dialog_close();
};

class BaseGabberWidget
     : public SigC::Object
{
public:
     BaseGabberWidget(const char* widgetname, const char* filename);
     virtual ~BaseGabberWidget();
     void show() { _thisWidget->show(); }
     void hide() { _thisWidget->hide(); }
     Gtk::Widget* get_this_widget() { return _thisWidget; }
     virtual void close(); 
     // Object extender
     virtual void set_dynamic();
     // Destruction signal
     Signal0<void, Marshal<void> > evtDestroy;
protected:
     BaseGabberWidget();
     // FIXME: Should make this function properly copy
     BaseGabberWidget& operator=(const BaseGabberWidget&) { return *this;}
     BaseGabberWidget(const BaseGabberWidget&) {}
public:
     // Helper functions
     Gtk::Button*          getButton(const char* name)      { return getWidgetPtr<Gtk::Button>(_thisGH, name); }
     Gtk::CheckButton*     getCheckButton(const char* name) { return getWidgetPtr<Gtk::CheckButton>(_thisGH, name); }
     Gtk::Entry*           getEntry(const char* name)       { return getWidgetPtr<Gtk::Entry>(_thisGH, name); }
     Gtk::Label*           getLabel(const char* name)       { return getWidgetPtr<Gtk::Label>(_thisGH, name); }
     Gtk::MenuItem*        getMenuItem(const char* name)    { return getWidgetPtr<Gtk::MenuItem>(_thisGH, name); }
     Gtk::OptionMenu*      getOptionMenu(const char* name)  { return getWidgetPtr<Gtk::OptionMenu>(_thisGH, name); }
     Gtk::PixmapMenuItem*  getPixmapMenuItem(const char* name) { return getWidgetPtr<Gtk::PixmapMenuItem>(_thisGH, name); }
     Gtk::Text*            getText(const char* name)        { return getWidgetPtr<Gtk::Text>(_thisGH, name); }
     template <class T> T* getWidget(const char* name)      { return getWidgetPtr<T>(_thisGH, name); }
     template <class T> T* getWidget_GTK(const char* name)  { return getWidgetPtr_GTK<T>(_thisGH, name); }
     
protected:
     // Internal refs
     Gtk::Widget* _thisWidget;
private:
     GladeXML*     _thisGH;
};

#endif

