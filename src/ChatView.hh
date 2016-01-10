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

#ifndef INCL_CHAT_VIEW_HH
#define INCL_CHAT_VIEW_HH

extern "C" {
#include "xtext.h"
}
#include <string>
#include <gtk/gtkframe.h>
#include <gtk/gtkvscrollbar.h>
#include <gtk--/container.h>
#include <gtk--/paned.h>
#include <gtk--/widget.h>

using namespace std;

typedef const string COLOR_T;

COLOR_T BLACK        ("\0030");
COLOR_T WHITE        ("\0031");
COLOR_T BLUE         ("\0032");
COLOR_T GREEN        ("\0033");
COLOR_T RED          ("\0034");
COLOR_T YELLOWBROWN  ("\0035");
COLOR_T PURPLE       ("\0036");
COLOR_T ORANGE       ("\0037");
COLOR_T YELLOW       ("\0038");
COLOR_T GREEN2       ("\0039");
COLOR_T AQUA         ("\00310");
COLOR_T LIGHTAQUA    ("\00311");
COLOR_T BLUE2        ("\00312");
COLOR_T PINK         ("\00313");
COLOR_T GREY         ("\00314");
COLOR_T LIGHTGREY    ("\00315");
COLOR_T BLUEMARKBACK ("\00316");
COLOR_T WHITEMARKFORE("\00317");
COLOR_T WHITEFORE    ("\00318");
COLOR_T BLACKBACK    ("\00319");


class ChatView
{
public:
     ChatView(Gtk::Widget* owner, Gtk::Container* parent, bool indent = true);
     ChatView(Gtk::Widget* owner, Gtk::Paned* parent, bool indent = true);
     ~ChatView();
     void render(const string& message, const string& username, const string& timestamp, COLOR_T& delimiter_color);
     void render_highlight(const string& message, const string& username, const string& timestamp, COLOR_T& delimiter_color, COLOR_T& highlight_color);
     void render_error(const string& message, const string& error, const string& timestamp, COLOR_T& delimiter_color);
     void clearbuffer();
     GtkXText* _xtext;
     GtkFrame* _frmChat;
     GtkVScrollbar* _vsChat;
protected:
     void print(const string& s);
     void print(const string& left, const string& right);
     // Handlers
     void on_word_clicked(char* word, GdkEventButton* evt);
     static void _on_word_clicked_stub(GtkXText* xtext, char* word, GdkEventButton* evt, ChatView* _this);
};

#endif
