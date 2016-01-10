/* SSLAdapter.hh
 * SSL adapter for TCP connection
 *
 * Copyright (C) 1999-2001 Dave Smith & Julian Missig
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contributor(s): Konrad Podloucky
 */

#ifndef INCL_SSL_ADAPTER_HH
#define INCL_SSL_ADAPTER_HH

#include <string>
#include <glib.h>
#include <openssl/ssl.h>

using namespace std;

class SSLAdapter
{
public:
     SSLAdapter();
     ~SSLAdapter();
  
     void disconnect();
     GIOError send(const gchar* data, const guint len, guint* written);
     GIOError read(gchar* buffer, const guint count, guint* bytes_read);
     bool registerSocket(const gint socket);
     const string getError();

protected:
     void seed_rng();

private:
     SSL*			_client_ssl;
     SSL_METHOD*		_ssl_method;
     SSL_CTX*			_ssl_client_context;

     string			_lastError;
     bool			_connected;
};

#endif /* INCL_SSL_TRANSMITTER_HH */
