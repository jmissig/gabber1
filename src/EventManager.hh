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

#ifndef INCL_EVENTS_MANAGER_HH
#define INCL_EVENTS_MANAGER_HH

#include "jabberoo.hh"

#include <sigc++/object_slot.h>

/**
 * A manager for things that happen when events occur.
 * Gabber creates an instance and maintains all the settings (temporary and otherwise).
 * It calls the various functions when the events occur.
 * You can either plop your code in the manager itself or attach to the signals.
 */
class EventManager
     : public SigC::Object
{
public:
     /** 
      * Construct a manager. Gabber will do this.
      * @see GabberApp::getEventsManager
      */
     EventManager();

     /**
      * The MessageManager::MessageType
      */
     typedef int MessageType;

     /**
      * Gabber calls this when a message is first received.
      * It may not yet be displayed. Note that an unqueued received event does not happen 
      * at the same time as a displayed event.
      * @param m The actual message.
      * @param nickname The nickname of the person who sent the message.
      * @param type The type of Message this will be displayed as. May be different from the actual type.
      * @param queued Whether this message was queued (true) or is being popped up immediately (false).
      */
     void message_received(const jabberoo::Message& m, const string& nickname, MessageType type, bool queued);

     /**
      * Gabber calls this when a message is displayed.
      * This may be some time after it is received.
      * @param m The actual message.
      * @param nickname The nickname of the person who sent the message.
      * @param type The type of Message this will be displayed as. May be different from the actual type.
      */
     void message_displayed(const jabberoo::Message& m, const string& nickname, MessageType type);

private:
     string _festival_path;
};

#endif // INCL_EVENTS_MANAGER_HH
