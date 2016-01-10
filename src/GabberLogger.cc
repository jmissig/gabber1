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

/*
 * HTML logging added by Dave Lee <dave@cherryville.org> 01/11/2000
 */

#include "GabberConfig.hh" // for _()

#include "GabberLogger.hh"

#include "GabberApp.hh"
#include "GabberGPG.hh"
#include "GabberUtility.hh"

#include <fcntl.h>
#include <sys/stat.h>
#include <utime.h>
#include <unistd.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h>

using std::ios;
using std::string;
using std::map;
using std::for_each;
using jabberoo::Roster;
using jabberoo::Session;
using jabberoo::Message;
using jabberoo::JID;
using namespace GabberUtil;

const string GabberLogger::HTML_FOOTER = "</body>\n</html>";
const string GabberLogger::XML_FOOTER = "</jabber>";

void releaseStream(pair<string, fstream*> p)
{
     p.second->close();
     delete p.second;
}

GabberLogger::GabberLogger(const string& logdir, const string& nick, bool html)
  : _logdir(logdir), _nickname(nick), _html(html)
{
     initdir();
     if (moveLogs());
}

void GabberLogger::initdir()
{
     // Ensure _logdir is terminated with a /
     if (_logdir[_logdir.length()-1] != '/')
	  //_logdir.resize(_logdir.length() - 1);
	  _logdir += "/";
     // Replace ~ with $HOME
     if (_logdir[0] == '~')
	  _logdir.replace(0, 1, g_get_home_dir());
     // See if directory already exists..
     if ((!g_file_test(_logdir.c_str(), G_FILE_TEST_ISDIR)) &&
	 mkdir(_logdir.c_str(), 0700) == -1)
     {
	  g_error(("Unable to create logging directory: " + _logdir).c_str());
	  G_App->quit();
     }
     // Add the gabber-logs.css file to the log dir
     if (!g_file_exists((_logdir + "gabber-logs.css").c_str()))
	  system(("/bin/cp " + G_App->getCfg().get_CSSFILE() + " "
		  + _logdir).c_str());
}

GabberLogger::~GabberLogger()
{
     for_each(_streams.begin(), _streams.end(), releaseStream);
}

void GabberLogger::setLogHTML(bool html)
{
     _html = html;
}

void GabberLogger::setLogDir(const string& dir)
{
     _logdir = dir;
     initdir();
}

const string GabberLogger::getLogFile(const string& jid) const
{
     // We don't want resources
     string file = JID::getUserHost(jid);
     // Replace @ with _
//     if (file.find("@") != string::npos)
//          file.replace(file.find("@"), 1, "_");
     // Convert to lowercase
     for (int i = 0; i < (int)file.length(); i++)
	  file[i] = tolower(file[i]);
     return _logdir + file + (_html ? ".html" : ".xml");
}

void GabberLogger::format(ostream* log, const string& body) const
{
     int start = 0, size = body.size(), end;
     // if its a multi-line body, then start off on a new line
     if ((end = body.find('\n')) != (int)string::npos)
	  *log << "<br />" << endl;

     // cycle through line by line
     do
     {
	  // get the first line
	  if (end == (int)string::npos)
	       end = size;
	  if (body[end - 1] == '\r')
	       end--;
	  string line = body.substr(start, end - start);

	  // find first url
	  int urlstart = line.find("http://"), urlend = 0;
	  int ftp = line.find("ftp://");
	  if (ftp < urlstart && ftp != (int)string::npos)
	       urlstart = ftp;
	  // iterate through the urls
	  while (urlstart >= 0)
	  {
	       // write stuff before url
	       *log << line.substr(urlend, urlstart - urlend);
	       // find the end
	       urlend = line.find(' ', urlstart);
	       if (urlend == (int)string::npos)
		    urlend = line.size();
	       char last = line[urlend - 1];
	       if (!isalnum(last) && last != '/')
		    urlend--;
	       // extract and write the url
	       string url = line.substr(urlstart, urlend - urlstart);
	       *log << "<a href=\"" << url << "\">" << url << "</a>";
	       urlstart = line.find("http://", urlend);
	       // search for the next url if there is one
	       ftp = line.find("ftp://", urlend);
	       if (ftp < urlstart && ftp != (int)string::npos)
		    urlstart = ftp;
	  }
	  // print what ever is left over
	  if (urlend < (int)line.size() && urlend != (int)string::npos)
	       *log << line.substr(urlend);
	  *log << "<br />" << endl;
	  // move pointer forward
	  if (end < size)
	       if (body[end] == '\r')
		    start = end + 2;
	       else
		    start = end + 1;
     } while (end != size && (end = body.find('\n', start)));
}

void GabberLogger::log(const string& jid, const Message& m)
{
     // Special case for groupchat, we don't log messages we send out
     // because they will get logged when anyway when recieve it
     if (m.getType() == Message::mtGroupchat && m.getFrom().empty())
	  return;

     bool movedLogs = moveLogs();
     fstream* log = NULL;
     string fname = getLogFile(jid);
     streampos logpos;
     const string& footer = (_html ? HTML_FOOTER : XML_FOOTER);

     // see if a stream is already available...
     map<string, fstream*>::iterator it = _streams.find(fname);
     if (it == _streams.end() || movedLogs)
     {
	  // Create the stream..
          if (!g_file_exists(fname.c_str()))
	       open(fname.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	  log = new fstream(fname.c_str(), ios::in | ios::out);
	  if (log != NULL && log->is_open())
          {
               _streams[fname] = log;
               // If its a new log file, place the required html at the top,
	       log->seekp(0, ios::end);
	       if (log->tellp() == (streampos) 0) // if true then the file is new
	       {
		    *log << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
		    if (_html)
		    {
			 *log << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">" << endl;
			 *log << "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n<head>" << endl;
			 *log << "  <link rel=\"stylesheet\" type=\"text/css\" href=\"gabber-logs.css\" />" << endl;
			 *log << "</head>\n<body>" << endl;
			 *log << "  <div class=\"title\">" << toUTF8(_("Logs for "));
			 *log << (m.getFrom().empty()
				? JID::getUserHost(m.getTo())
				: JID::getUserHost(m.getFrom()));
			 *log << "</div>" << endl;
		    }
		    else
		    {
			 *log << "<?xml-stylesheet href=\"gabber-logs.css\" type=\"text/css\"?>" << endl;
			 *log << "<jabber xmlns=\"jabber:client\" xmlns:log=\"gabber:client:log\">" << endl;
		    }
		    *log << footer;
	       }
          }
	  else
	  {
	       cerr << "Unable to create message log file: " << fname << endl;
	       return;
	  }
     }
     else
     {
          log = it->second;
     }

     log->seekp( - (int)footer.size(), ios::end);
     // Log the message in html
     if (_html)
     {
	  *log << "<div class=\"message\">";
	  *log << "<span class=\"date\">" << toUTF8(m.getDateTime()) << "</span> ";
	  *log << "<span class=\"";
	  // from atrrib means its us recieving
	  if (!m.getFrom().empty())
	  {
	       *log << "them\">";
	       string nick = JID::getUser(jid);
	       // Try looking up nickname
	       try {
		    nick = G_App->getSession().roster()[jid].getNickname();
	       } catch (jabberoo::Roster::XCP_InvalidJID& e) {
		    // Special handling for groupchats, resource = nickname
		    if (G_App->isGroupChatID(jid))
			 nick = JID::getResource(jid);
	       }
	       *log << nick;
	  }
	  else
	  {
	       *log << "me\">" << _nickname;
	  }
	  *log << "</span> ";
	  string subject = m.getSubject();
	  if (!subject.empty())
	       *log << "<span class=\"subject\">" << subject << ":</span> ";
	  *log << "<span class=\"body\">";

	  // get the CDATA ... 
	  format(log, judo::escape(m.getBody()));
	  *log << "</span></div>" << endl;
     }
     else // log the straight xml
     {
	  Element* x = m.findX("jabber:x:delay");
	  if (x == NULL) // there's no timestamp, which means we sent it
	  {
	       Message mdated(m);
	       x = mdated.addX("jabber:x:delay");
	       x->putAttrib("stamp", toUTF8(mdated.getDateTime("%Y%m%dT%H:%M:%S%z")));
	       *log << mdated.toString() << endl;
	  }
	  else
	  {
	       *log << m.toString() << endl;
	  }
     }
     *log << footer;
     log->flush();
}

bool GabberLogger::moveLogs() 
{
     bool movedLogs = false;
     bool save  = G_App->getCfg().logs.save;
     bool purge = G_App->getCfg().logs.purge;

     if (save || purge)
     {
	  struct stat sb;
	  time_t t = time(0);
	  // get the current time
	  struct tm* now = new tm;
	  memcpy(now, localtime(&t), sizeof(struct tm));
	  int movedAtMon = G_App->getCfg().logs.lastMovedMonth;
	  int movedAtYr  = G_App->getCfg().logs.lastMovedYear;

	  // if this is the default year, then moveLogs() has not been called before...
	  if (movedAtYr == 0)
	  {
		// So initialize the year/month values in the config file:
	     G_App->getCfg().logs.lastMovedMonth = now->tm_mon;
	     G_App->getCfg().logs.lastMovedYear = now->tm_year;
	     delete now;
	     return false;
	  }

	  if (stat(_logdir.c_str(), &sb) != -1)
	  {
	       if ( (movedAtYr < now->tm_year) || (movedAtMon < now->tm_mon))
	       {
		    //close all *log* file streams
		    //XXX Just a point of concern: this will only close *log* files, right?
		    for_each(_streams.begin(), _streams.end(), releaseStream);

		    // archive dir is _logdir/YYYY-MM
		    char dir[10];
		    snprintf(dir, sizeof(dir), "%d-%.2d",
			     1900 + movedAtYr, 1 + movedAtMon);
		    string archive = _logdir + dir;
		    if (purge)
		    {
			 system(("/bin/rm " + _logdir + "*.*ml").c_str());
		    }
		    else if (save)
		    {
			 if (g_file_test(archive.c_str(), G_FILE_TEST_ISDIR) ||
			     mkdir(archive.c_str(), 0700) != -1)
			 {
			      // mv logs, and copy css file
			      system(("/bin/mv " + _logdir + "*.*ml "
				      + archive).c_str());
			      system(("/bin/cp " + _logdir + "gabber-logs.css "
				      + archive).c_str());
			 }
			 else
			 {
			      g_error(("Could not create archive directory: "
				       + archive).c_str());
			 }
		    }
		     // update timestamp to show that last months's logs have been moved/purged:
		     G_App->getCfg().logs.lastMovedMonth = now->tm_mon;
                     G_App->getCfg().logs.lastMovedYear = now->tm_year;
		     movedLogs = true;
	       }
	  }
	  else
	  {
	       g_error(("Unable to stat log directory: " + _logdir).c_str());
	  }
	  delete now;
     }
     return movedLogs;
}

bool GabberLogger::tryToLog(const string& jid, const Message& m, bool encrypted)
{
	// if the user wants groupchats logged and this is a groupchat, log it.
	if (m.getType() == Message::mtGroupchat )
	{
		if (G_App->getCfg().logs.groupchat)
		{
			log(jid, m);
			return true;
		}
	}
	//for all non-groupchat messages...
	else if (!G_App->getCfg().logs.none)
	{
		// Is this a message that was or will be encrypted?
		if (encrypted)
		{	
			if (G_App->getCfg().logs.encrypted)
			{
				// Decrypt the message here.  That way log()
				// doesn't have to do it in two places.
				// If the message is encrypted, decrypt the
				// message before it's logged
				GabberGPG& gpg = G_App->getGPG();
				string body = m.getBody();
				Element* x = m.findX("jabber:x:encrypted");
				if (gpg.enabled() && x != NULL && !m.getFrom().empty())
				{
					string encrypted = x->getCDATA();
					GPGInterface::DecryptInfo info;
					GPGInterface::Error err;
					gpg.decrypt(info, encrypted, body);
					Message decrypted(m.getBaseElement());
					decrypted.eraseX("jabber:x:encrypted");
					decrypted.setBody(body);
					log(jid, decrypted); // log the decrypted message.
				} else {
					log(jid, m); // The body wasn't encrypted.
				}
				return true;
			}
		} else if (!m.getBody().empty()) 
		{
			log(jid, m);
			return true;
		}
	}
	//Nothing was logged.
	return false;
}
