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
 *  Gabber Events Manager
 *  Copyright (C) 2002 Julian Missig
 */

#include "GabberConfig.hh" // For _()

#include "EventManager.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "MessageManager.hh"

#include <libgnome/gnome-util.h>

// For GNOME1 sounds:
#include <libgnome/gnome-triggers.h>

// For festival:
#include <unistd.h> // for fork() and exec() stuff


using namespace jabberoo;
using namespace GabberUtil;

EventManager::EventManager()
{
     // festival - Just Say It!
     char* path, *spath;
     path = getenv("PATH");
     // If we couldn't get the path from that, try a couple of common places
     if (!path)
          path = "/bin:/usr/bin:/usr/local/bin";

     spath = g_strdup(path);
     for (char* tmp = strtok(spath, ":"); tmp; tmp = strtok(NULL, ":"))
     {
          char* sfespath = g_concat_dir_and_file(tmp, "/festival");
          if (g_file_exists(sfespath))
          {
	       _festival_path = sfespath;
               //g_free(sfespath);
               //g_free(spath);
          }
          g_free(sfespath);
     }
     g_free(spath);
}

void EventManager::message_received(const Message& m, const string& nickname, MessageType type, bool queued)
{
     // message:x:event - Send back the appropriate event if requested
     Element* x = m.findX("jabber:x:event");
     if (x != NULL && !m.getBody().empty() && x->findElement("delivered") != NULL)
     {
	  // delivered event was requested and this message has been delivered
	  G_App->getSession() << m.delivered();
     }


     // GNOME1 sound - play the sound for the *real* message type ('type' attrib is the *displayed* message type)
     if (queued)
     {
	  if (m.getType() == Message::mtNormal)
	       gnome_triggers_do(NULL, NULL, "gabber", "QueueMessage", NULL);
	  else if (m.getType() == Message::mtChat)
	       gnome_triggers_do(NULL, NULL, "gabber", "QueueOOOChat", NULL);
     }
     else
     {
	  if (m.getType() == Message::mtNormal)
	       gnome_triggers_do(NULL, NULL, "gabber", "RecvMessage", NULL);
	  else if (m.getType() == Message::mtChat)
	       gnome_triggers_do(NULL, NULL, "gabber", "RecvOOOChat", NULL);
     }


}

void EventManager::message_displayed(const Message& m, const string& nickname, MessageType type)
{
     // message:x:event - Send back the appropriate event if requested
     Element* x = m.findX("jabber:x:event");
     if (x != NULL && x->findElement("displayed") != NULL)
     {
	  // displayed event was requested and this message has been displayed
	  G_App->getSession() << m.displayed();
     }
     
     
     // festival - Just Say It!
     if (!_festival_path.empty() && G_App->getCfg().speech.usefestival)
     {
	  string festival_nickname = nickname;
	  string festival_message = m.getBody();
	  
	  // Replace double-quotes with \" so festival likes them
	  for( string::size_type j = 0; j <= festival_nickname.length(); j++ )
	  {
	       if( festival_nickname[j] == '"' )
	       {
		    festival_nickname.insert(j, "\\");
		    j++;
	       }
	  }

	  for( string::size_type j = 0; j <= festival_message.length(); j++ )
	  {
	       if( festival_message[j] == '"' )
	       {
		    festival_message.insert(j, "\\");
		    j++;
	       }
	  }


     	  list<string> festival_args;
	  festival_args.push_back(_festival_path);
	  festival_args.push_back("--batch");
	  string festival_say = "(SayText \"" + festival_nickname + " says: " + festival_message +"\")";
	  festival_args.push_back(festival_say);
	  
	  char ** argv = g_new(char *, 4);
	  int i = 0;
	  for (list<string>::iterator it = festival_args.begin(); it != festival_args.end(); ++it, i++)
	  { 
	       argv[i] = (char *) it->c_str(); cout << it->c_str() << " "; 
	  }
	  cout << endl;
	  argv[i] = NULL;
	  
	  pid_t festival_pid;
	  
	  switch ( (festival_pid = fork()) )
	  {
	  case -1:
	       cerr << "festival: Alas! Couldn't fork for festival!" << endl;
	  case 0:
	       // child process
	       execv(argv[0], argv);
	       
	       // Failed!!
	       cerr << "festival: Something went wrong!  There are horses eating each other!" << endl;
	       exit(1);
	  }
	  
	  g_free(argv);
     }
}
