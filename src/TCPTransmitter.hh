/* TCPTransmitter.hh
 * TCP connection
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
 * Contributor(s): Konrad Podloucky, Matthias Wimmer, Brandon Lees, 
 *  JP Sugarbroad, Julian Missig
 */

#ifndef INCL_TCP_TRANSMITTER_HH
#define INCL_TCP_TRANSMITTER_HH

#include "GabberConfig.hh"
#include <string>
#include <list>
#include <glib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sigc++/signal_system.h>
#ifdef WITH_SSL
#include "SSLAdapter.hh"
#endif

using namespace std;

class TCPTransmitter 
     : public SigC::Object
{
public:
     TCPTransmitter();
     ~TCPTransmitter();
     
     // General connection methods
     void connect(const string& host, guint port, bool use_ssl, bool autoreconnect);
     // XXX possibly expand this to have a reuse addr option and ssl
     void listen(const string& host, guint port);
     void disconnect();
     void startPoll();
     const string getsockname();
     
     // Signals
     SigC::Signal0<void>		evtConnected;
     SigC::Signal0<void>		evtAccepted;
     SigC::Signal0<void>		evtDisconnected;
     SigC::Signal0<void>                evtReconnect;
     SigC::Signal0<void>                evtCanSendMore;
     SigC::Signal1<void, int>	        evtDataSent;
     SigC::Signal1<void, const string&>	evtError;
     SigC::Signal2<void, const char*, int> evtDataAvailable;
     
     
     // Transmitter methods
     void sendsz(const char* data, guint sz);
     void send(const char* data);
     void needSend(bool notify);
     guint getPort() const
     {
        return _port;
     }

     enum Proxy
     {
	  none, httpConnect, httpPut, httpPost, socks4, socks5
     };

     void setProxy(const string &ptype, const string &host, guint port, const string &user, const string &password, bool tryOther = true);
     
private:
     enum TransmitterError {
	  errAddressLookup, errSocket
     };
     
     typedef struct
     {
        char *data;
        int sz;
        int cur_pos;
     } send_data_buf;
     
     enum State
     {
        Offline,
        Connecting,
        Reconnecting,
        Connected,
        Listening,
        Accepting,
        Error,
	ProxyConnecting
     };

     enum HandshakeState
     {
	  MethodsSent,
	  AuthenticationSent,
	  ConnectCmdSent
     };
        
     // send method implementation
     void _send(send_data_buf* data);

     // Socket related callbacks
     static gboolean on_socket_accept(GIOChannel* iochannel, GIOCondition cond, gpointer data);
     static gboolean on_socket_event(GIOChannel* iochannel, GIOCondition cond, gpointer data);
     static gboolean on_socket_connect(GIOChannel* iochannel, GIOCondition cond, gpointer data);
     static gboolean on_host_resolved(GIOChannel* iochannel, GIOCondition cond, gpointer data);
     
     static gboolean reconnect(gpointer data);
          
     void handleError(const TransmitterError e);
     void handleError(const string & emsg);
     const string getSocketError();
     void proxyHandshakeIn(const char *buf, guint bytes_read);
     void proxyHandshakeOut();
     void proxyHandleHttpHead();
     void proxyHandleSocks4Head();
     void proxyHandleSocks5Head();
     void proxyHandleSocks5MethodReply();
     void proxyHandleSocks5AuthReply();
     void proxyHandleSocks5ConnectReply();
     void proxySendSocks5Auth();
     void proxySendSocks5Connect();
     void proxySendHead(const string &header);
     static string encodeBase64(string text);
    

     void _async_resolve(const gchar* hostname);
     void _async_connect();
#ifdef WITH_IPV6
     bool _gethostbyname(const gchar* hostname, struct in6_addr* result);
#else
     bool _gethostbyname(const gchar* hostname, struct in_addr* result);
#endif
     static gboolean _connection_poll(gpointer data);

     
     // Members
     gint				_socketfd;
     guint               _port;
     GIOChannel*			_IOChannel;
     guint				_reconnect_timeoutid;
     guint				_reconnect_delay;
     GTimeVal*				_reconnect_timeval;
     State              _state;
     bool               _hostResolved;
     bool                               _use_ssl;
     bool                               _autoreconnect;
     bool               _need_send;

     struct {
	  Proxy			type;
	  string		host;
	  guint			port;
	  string		dest_host;
	  guint			dest_port;
	  string		user;
	  string		password;
	  bool			try_other;
	  bool			failed_connect;
	  bool			failed_put;
	  bool			failed_post;
	  bool			failed_socks4;
	  bool			failed_socks5;
	  guint			response;
	  string		response_line;
	  HandshakeState	socks5_state;
     } _proxy;
     
#ifdef WITH_IPV6
     struct sockaddr_in6		_host_sockaddr;
#else
     struct sockaddr_in			_host_sockaddr;
#endif
     guint				_resolver_watchid;
     guint				_poll_eventid;
     guint				_send_retry_eventid;
     guint				_retries;
     gint				_socket_flags;
     pid_t				_resolver_pid;
     guint				_socket_watchid;
#ifdef WITH_SSL
     SSLAdapter*			_adapter;
#endif
     std::list<send_data_buf*> _sendBuffer;
#ifdef HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
     static GStaticMutex		_gethostbyname_mutex;
#endif
};


#endif /* INCL_TCP_TRANSMITTER_HH */
