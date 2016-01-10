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

#ifndef INCL_GABBER_LOGGER_HH
#define INCL_GABBER_LOGGER_HH

#include "jabberoo.hh"

#include <fstream>

class GabberLogger
{
public:
     GabberLogger(const std::string& logdir, const std::string& nick, bool html = true);
     ~GabberLogger();
     // log formating
     void setLogHTML(bool html);
     // log utilites
     void setLogDir(const std::string& dir);
     const std::string getLogFile(const std::string& jid) const;
     // main log call
     void log(const std::string& jid, const jabberoo::Message& m);
     bool tryToLog(const std::string& jid, const jabberoo::Message& m, bool encrypted);
private:
     void initdir();
     void format(std::ostream* log, const std::string& body) const;
     bool moveLogs();
     std::string                           _logdir;
     std::string                           _nickname;
     bool                                  _html;
     std::map<std::string, std::fstream*> _streams;
     static const std::string              HTML_FOOTER;
     static const std::string              XML_FOOTER;
};

#endif
