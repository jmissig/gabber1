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

#include "ChatView.hh"

extern "C" {
#include "xtext.h"
}

#include "jabberoo.hh"
using namespace jabberoo;

#include "AddContactDruid.hh"

#include <libgnome/gnome-url.h>
#include <gtk--/text.h>

GdkColor colors[] =
{
   {0, 0, 0, 0},                /* 0  black */
   {0, 0xcccc, 0xcccc, 0xcccc}, /* 1  white */
   {0, 0, 0, 0xcccc},           /* 2  blue */
   {0, 0, 0xcccc, 0},           /* 3  green */
   {0, 0xcccc, 0, 0},           /* 4  red */
   {0, 0xbbbb, 0xbbbb, 0},      /* 5  yellow/brown */
   {0, 0xbbbb, 0, 0xbbbb},      /* 6  purple */
   {0, 0xffff, 0xaaaa, 0},      /* 7  orange */
   {0, 0xffff, 0xffff, 0},      /* 8  yellow */
   {0, 0, 0xffff, 0},           /* 9  green */
   {0, 0, 0xcccc, 0xcccc},      /* 10 aqua */
   {0, 0, 0xffff, 0xffff},      /* 11 light aqua */
   {0, 0, 0, 0xffff},           /* 12 blue */
   {0, 0xffff, 0, 0xffff},      /* 13 pink */
   {0, 0x7777, 0x7777, 0x7777}, /* 14 grey */
   {0, 0x9999, 0x9999, 0x9999}, /* 15 light grey */
   {0, 0, 0, 0xcccc},           /* 16 blue markBack */
   {0, 0xeeee, 0xeeee, 0xeeee}, /* 17 white markFore */
   {0, 0xcccc, 0xcccc, 0xcccc}, /* 18 foreground (white) */
   {0, 0, 0, 0},                /* 19 background (black) */
};

typedef const int CONSTANT;
const int WORD_URL  = 1;
const int WORD_HOST = 2;
const int WORD_JABBER = 3;

void palette_load(GtkWidget* w)
{
     int i;
     
     if (!colors[0].pixel)             /* don't do it again */
	  for (i = 0; i < 20; i++)
	  {
	       colors[i].pixel = (gulong) ((colors[i].red & 0xff00) * 256 +
	       			   (colors[i].green & 0xff00) +
	       			   (colors[i].blue & 0xff00) / 256);
	       if (!gdk_colormap_alloc_color (gtk_widget_get_colormap (w), &colors[i], 0, 1))
	           cerr << "Error allocating color " << i << endl;
	  }
}

int word_check(GtkWidget* t, char* word)
{
   if (!word)
	return 0;
   int len = strlen (word);

   if (!strncasecmp (word, "irc://", 6))
      return WORD_URL;

   if (!strncasecmp (word, "irc.", 4))
      return WORD_URL;

   if (!strncasecmp (word, "ftp.", 4))
      return WORD_URL;

   if (!strncasecmp (word, "ftp:", 4))
      return WORD_URL;

   if (!strncasecmp (word, "www.", 4))
      return WORD_URL;

   if (!strncasecmp (word, "http:", 5))
      return WORD_URL;

   if (!strncasecmp (word, "https:", 6))
      return WORD_URL;

   if (!strncasecmp (word, "jabber:", 7))
      return WORD_JABBER;

   if (!strncasecmp (word + len - 4, ".org", 4))
      return WORD_HOST;

   if (!strncasecmp (word + len - 4, ".net", 4))
      return WORD_HOST;

   if (!strncasecmp (word + len - 4, ".com", 4))
      return WORD_HOST;

   if (!strncasecmp (word + len - 4, ".edu", 4))
      return WORD_HOST;

   return 0;
}

ChatView::ChatView(Gtk::Widget* owner, Gtk::Container* parent, bool indent)
{
     // Create a Gtk::Text so we can grab the colors, then destroy it
     Gtk::Text textwidget;
     parent->add(textwidget);
     textwidget.realize();

     // Copy the style of the Gtk::Text widget
     GtkStyle* gs = gtk_widget_get_style(GTK_WIDGET(textwidget.gtkobj()));
     colors[16] = gs->bg[GTK_STATE_SELECTED];
     colors[17] = gs->fg[GTK_STATE_SELECTED];
     colors[18] = gs->fg[GTK_STATE_NORMAL];
     colors[19] = gs->base[GTK_STATE_NORMAL];

     // Initialize the palette
     palette_load(owner->gtkobj());

     // Create the GtkXText object
     _xtext = GTK_XTEXT(gtk_xtext_new(75, 0));

     // Internal init
     _xtext->max_lines = 1024;
     _xtext->urlcheck_function = word_check;    
     _xtext->auto_indent = !indent;
     _xtext->wordwrap = true;

     // Display the widget
     //gtk_widget_set_usize (GTK_WIDGET(_xtext), 200, 200); 
     gtk_xtext_set_palette(_xtext, colors);
     gtk_xtext_set_font(_xtext, gs->font);

     // destroy the Gtk::Text widget now that we've used the last bits of the style
     parent->remove(textwidget);
     textwidget.destroy();

     // Create a frame around it
     _frmChat = GTK_FRAME(gtk_frame_new(NULL));
     gtk_frame_set_shadow_type(GTK_FRAME(_frmChat), GTK_SHADOW_IN);

     // Add the widget to a container, which happens to be a frame
     gtk_container_add(parent->gtkobj(), GTK_WIDGET(_frmChat));
     gtk_container_add(GTK_CONTAINER(_frmChat), GTK_WIDGET(_xtext));
     gtk_widget_show(GTK_WIDGET(_frmChat));
     gtk_widget_show(GTK_WIDGET(_xtext));

     // Create a scrollbar
     _vsChat = GTK_VSCROLLBAR(gtk_vscrollbar_new(_xtext->adj));
     gtk_box_pack_start (GTK_BOX (parent->gtkobj()), GTK_WIDGET(_vsChat), FALSE, FALSE, 1);
     GTK_WIDGET_UNSET_FLAGS (_vsChat, GTK_CAN_FOCUS);
     gtk_widget_show (GTK_WIDGET(_vsChat));

     // Hookup stub callback
     gtk_signal_connect(GTK_OBJECT(_xtext), "word_click", 
			GTK_SIGNAL_FUNC(&ChatView::_on_word_clicked_stub), this);

}

ChatView::~ChatView()
{
     // Destroy!
     gtk_widget_destroy(GTK_WIDGET(_xtext));
     gtk_widget_destroy(GTK_WIDGET(_frmChat));
     gtk_widget_destroy(GTK_WIDGET(_vsChat));
}

void ChatView::render(const string& message, const string& username, const string& timestamp, COLOR_T& c)
{
     // Process system messages
     if (username == "")
     {
	  print (timestamp + " " + c + "-\017" , message);
     }
     // Process action messages (/me)
     // I have two compares so that most of the time it only has to do one (first will fail)
     // but in the few cases where there is a /me, it has to do one more check - as opposed to always doing one more
     else if (message.substr(0,3) == "/me" && (message.substr(0,4) == "/me " || message.substr(0,6) == "/me's "))
     {
	  print (timestamp + " " + c + "*\017" , username + message.substr(3));
     }

     // Process std messages
     else
     {
	  print(timestamp + c + "<\017" + username + c + ">\017" , message);
     }

}

void ChatView::render_highlight(const string& message, const string& username, const string& timestamp, COLOR_T& c, COLOR_T& hi)
{
     // Process action messages (/me)
     // I have two compares so that most of the time it only has to do one (first will fail)
     // but in the few cases where there is a /me, it has to do one more check - as opposed to always doing one more
     if (message.substr(0,3) == "/me" && (message.substr(0,4) == "/me " || message.substr(0,6) == "/me's "))
     {
	  print (timestamp + " " + c + "*\017" , hi + username + "\017" + message.substr(3));
     }

     // Process std messages
     else
     {
	  print(timestamp + c + "<\017" + hi + username + c + ">\017" , message);
     }
}

void ChatView::render_error(const string& message, const string& error, const string& timestamp, COLOR_T& c)
{
     // Process error messages
     print(timestamp + c + error + "\017:", message);
}

void ChatView::clearbuffer()
{
     // Flush the buffer
     gtk_xtext_clear(_xtext);
}

void ChatView::on_word_clicked(char* word, GdkEventButton* evt)
{
     string s_word = word;

     if (evt->button == 3) // right-click
     {
	  switch(word_check(GTK_WIDGET(_xtext), word))
	  {
	  case WORD_URL:
	       gnome_url_show(word);
	       break;
	  case WORD_HOST:
	       s_word = "http://" + s_word;
	       gnome_url_show(s_word.c_str());
	       break;
	  case WORD_JABBER:
	       string::size_type i = s_word.find(":");
	       if (i != string::npos)
		    s_word = s_word.substr(i+1, s_word.length());
	       AddContactDruid::display(s_word);
	       break;
	  }	  
     }
}

void ChatView::_on_word_clicked_stub(GtkXText* xtext, char* word, GdkEventButton* evt, ChatView* _this)
{
     _this->on_word_clicked(word, evt);
}

inline void ChatView::print(const string& s)
{
     unsigned char* astr = (unsigned char*)s.c_str();
     char* anewline = strchr((char*)astr, '\n');
     if (anewline)
     {
	  while(1)
	  {
	       gtk_xtext_append(_xtext, astr, (unsigned long)anewline - (unsigned long)astr);
	       while (*(anewline) != '\0' && *(anewline+1) == '\n')
	       {
		    gtk_xtext_append_indent (_xtext, NULL, 0, (unsigned char *) "\n", 1);
		    anewline++;
	       }
	       astr = (unsigned char*)anewline + 1;
	       if (*astr == 0)
		    break;
	       anewline = strchr((char*)astr, '\n');
	       if (!anewline)
	       {
		    gtk_xtext_append(_xtext, astr, -1);
		    break;
	       }
	  }
     }
     else
     {
	  gtk_xtext_append(_xtext, astr, s.length());
     }
}

inline void ChatView::print(const string& left, const string& right)
{
     unsigned char* s = (unsigned char*)right.c_str();
     char* newline = strchr((char*)s, '\n');
     if (newline)
     {
	  // Display the first line w/ the user who said it
	  gtk_xtext_append_indent(_xtext, (unsigned char*)left.c_str(), left.length(),
			   s, (unsigned long)newline - (unsigned long)s);
	  // Now loop through the rest of the string, displaying it
	  while (1)
	  {
	       while (*(newline) != '\0' && *(newline+1) == '\n')
	       {
		    gtk_xtext_append_indent (_xtext, NULL, 0, (unsigned char *) "\n", 1);
		    newline++;
	       }
	       s = (unsigned char*)newline + 1;
	       if (*s == 0)
		    break;
	       newline = strchr((char*)s, '\n');
	       // If another newline is found, append it to the display and loop again	       
	       if (newline)
	       {
		    gtk_xtext_append_indent(_xtext, NULL, 0, s, (unsigned long)newline - (unsigned long)s);
	       }
	       // Otherwise append the rest of the string and break out of the loop
	       else
	       {
		    gtk_xtext_append_indent(_xtext, NULL, 0, s, -1);
		    break;
	       }
	  }
     }
     else
     {
	  gtk_xtext_append_indent(_xtext, (unsigned char*)left.c_str(), left.length(),
				  (unsigned char*)right.c_str(), right.length());
     }
}
