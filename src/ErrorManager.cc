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

#include "GabberConfig.hh" // For _()

#include "ErrorManager.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "MessageManager.hh"

#include <libgnome/gnome-i18n.h>
#include <gnome--/dialog.h>

using namespace jabberoo;
using namespace GabberUtil;

ErrorManager::ErrorManager()
{
     // um.. yeah..
}

void ErrorManager::add(const Message& m)
{
     // Who this error is concerning
     string concerning = m.getFrom();

     // The error message
     string errormsg = m.getError();

     // XXX: This is really ugly with the new MessageManager, need to figure
     // something out...
     // Route the error if we have a view for the same jid as the same type
     if (G_App->getMessageManager().hasView(concerning, MessageManager::translateType("chat")))
	  errorChat(concerning, m.getErrorCode(), errormsg, m.getBody());
     // JID::getUserHost for groupchats since resource == nickname
     else if (G_App->getMessageManager().hasView(concerning, MessageManager::translateType("groupchat")))
	  errorGroupchat(JID::getUserHost(concerning), m.getErrorCode(), errormsg, m.getBody());
     // XXX: if we don't have a view for a type then we don't have a type
     // FIx this when headlines are added
#if 0
     else if (G_App->getMessageManager().hasView(concerning, MessageManager::mtHeadline))
	  errorHeadline(concerning, m.getErrorCode(), errormsg, m.getBody());
#endif
     else if (!m.getBody().empty() && !m.getError().empty())
     {
	  // It has a message body and a separate error message
	  // but isn't from ooochat, groupchat, or headline.
	  // must be normal.  XXX: ACK..False assumption, let it slide for now.
	  G_App->getMessageManager().add(m, false);
	  G_App->getMessageManager().display(m.getFrom(), MessageManager::translateType("normal"));
     }
     else if (!m.getError().empty())
     {
	  // REALLY BAD HACK
	  // Christ Temas, that's it. I'm so goddamn sick of this aim-t error.
	  // It's not like it's even telling the truth, and it occurs hundreds of times.
	  // Yes, this is an utter hack.
	  if (m.getError() != "You are talking too fast. Message has been dropped.")
	  {
	       // It has a specific error message, but due to previous tests must not have a body
	       // so we'll use the error as the error
	       errormsg = m.getError();
	       //manage(new ErrorView(m, errormsg));
	       Gnome::Dialog* d = manage(Gnome::Dialogs::warning(substitute(_("A server-side error has occurred:\n%s"), errormsg)));
	       // d is a child window of this window:
	       main_dialog(d);
	       d->set_modal(true);
	  }
     }
     else if (!m.getBody().empty())
     {
	  // It has a message body, but due to previous tests must not have an error
	  // so we'll use the message body as the error
	  errormsg = m.getBody();
	  Gnome::Dialog* d = manage(Gnome::Dialogs::warning(substitute(_("A server-side error has occurred:\n%s"), errormsg)));
	  // d is a child window of this window:
	  main_dialog(d);
	  d->set_modal(true);
     }
     else
     {
	  // Um, it doesn't have an error or a body. 
	  // Let's just drop it since nothing would get displayed
	  ;
     }
}

void ErrorManager::add(const Presence& p)
{
     // Who this error is concerning
     string concerning = p.getFrom();

     // The error message
     string errormsg = p.getError();

     // Route the error if we have a view for the same jid as the same type
     if (G_App->getMessageManager().hasView(concerning, MessageManager::translateType("chat")))
	  errorChat(concerning, p.getErrorCode(), errormsg, errormsg);
     // JID::getUserHost for groupchats since resource == nickname
     else if (G_App->getMessageManager().hasView(concerning, MessageManager::translateType("groupchat")))
	  errorGroupchat(JID::getUserHost(concerning), p.getErrorCode(), errormsg, errormsg);
     // this doesn't exist atm
#if 0
     else if (G_App->getMessageManager().hasView(concerning, MessageManager::mtHeadline))
	  errorHeadline(concerning, p.getErrorCode(), errormsg, errormsg);
#endif
     else
     {
	  // We usually don't display error presences
	  ;
     }
}

string ErrorManager::translateError(int errorcode)
{
     switch(errorcode)
     {
     case 400:
	  return _("Bad Request");
     case 401:
	  return _("Unauthorized");
     case 402:
	  return _("Payment Required");
     case 403:
	  return _("Forbidden");
     case 404:
	  return _("Not Found");
     case 405:
	  return _("Not Allowed");
     case 406:
	  return _("Not Acceptable");
     case 407:
	  return _("Registration Required");
     case 408:
	  return _("Request Timeout");
     case 409:
	  return _("Username Not Available");
     case 500:
	  return _("Internal Server Error");
     case 501:
	  return _("Not Implemented");
     case 502:
	  return _("Remote Server Error");
     case 503:
	  return _("Service Unavailable");
     case 504:
	  return _("Remote Server Timeout");
     }
     return _("No Error");
}
