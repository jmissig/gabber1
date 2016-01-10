/* TCPTransmitter.cc
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

/*
 * This file contains code from GNet by David A. Helder
 * http://www.eecs.umich.edu/~dhelder/misc/gnet/
 */

#include "TCPTransmitter.hh"

#ifdef WITH_IPV6
#include <resolv.h>
#endif // WITH_IPV6
#include <netdb.h>
#include <iostream>
#include <string>
#ifdef HAVE_STD_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>

using namespace std;

#ifdef HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
     GStaticMutex TCPTransmitter::_gethostbyname_mutex = G_STATIC_MUTEX_INIT;
#endif // HAVE_GETHOSTBYNAME_R_GLIB_MUTEX

#ifndef HAVE_SOCKLEN_T
#ifdef USE_SOCKLEN_T_SIZE_T
typedef size_t socklen_t;
#else // USE_SOCKLEN_T_INT is the only other case
typedef int socklen_t;
#endif // USE_SOCKLEN_T_SIZE_T
#endif // HAVE_SOCKLEN_T

#ifdef SOLARIS
#define INET_ADDRSTRLEN 16
#endif // SOLARIS

TCPTransmitter::TCPTransmitter():
     _socketfd(-1), _IOChannel(NULL), _reconnect_timeoutid(0),
     _reconnect_delay(5), _reconnect_timeval(new GTimeVal),
     _state(Offline), _hostResolved(false), _use_ssl(false),
     _autoreconnect(false), _need_send(false),
     _resolver_watchid(0), _poll_eventid(0),
     _send_retry_eventid(0), _retries(0), _socket_flags(0),
     _resolver_pid(-1), _socket_watchid(0)
{
     _reconnect_timeval->tv_sec = 0;

     _proxy.type = none;
     _proxy.host = "";
     _proxy.port = 0;
     
#ifdef WITH_SSL
     _adapter = NULL;
#endif
     
#ifdef WITH_IPV6
     // get an AAAA record from gethostbyname if possible
     res_init();
     _res.options |= RES_USE_INET6;
#endif
}

TCPTransmitter::~TCPTransmitter()
{
     disconnect();
     delete _reconnect_timeval;
}

void TCPTransmitter::connect(const string& host, guint port, bool use_ssl, bool autoreconnect)
{
     string chost; // where we connect to - could also be our proxy
     guint cport;

     // do we connect over a proxy?
     if (_proxy.type != none)
     {
          chost = _proxy.host;
	  cport = _proxy.port;

	  // reconnecting without given host?
	  if (host != "")
	  {
	       _proxy.dest_host = host;
	       _proxy.dest_port = port;
	  }
     }
     else
     {
	  chost = host;
	  cport = port;
     }
    
     _proxy.response = 0;
     _proxy.response_line = "";

     // Set autoreconnect and use_ssl
     _autoreconnect = autoreconnect;
     _use_ssl = use_ssl;
     
#ifdef WITH_IPV6
     if (inet_addr(chost.c_str()) != INADDR_NONE)
     { // IP address in IPv4 notation - convert
	     chost = "::ffff:" + chost;
     }
#endif
     
#ifdef TRANSMITTER_DEBUG
     if (_proxy.type == none)
          cout << "Connecting to " << chost << ":" << cport << endl;
     else
          cout << "Connecting to " << _proxy.dest_host << " with proxy " <<
		  chost << ":" << cport << endl;
#endif
     
     if (_state == Offline || _state == Reconnecting || _state == ProxyConnecting)
     {
#ifdef WITH_SSL
	  if (use_ssl && _adapter == NULL)
	  {
	       _adapter = new SSLAdapter();
	  }
	  else if (!use_ssl && _adapter != NULL)
	  {
	       delete _adapter;
	       _adapter = NULL;
	  }
#else
	  if (use_ssl)
	  {
	       cerr << "SSL support not compiled in, but attempt was made to use SSL." << endl;
	       cout << "SSL disabled." << endl;
	  }
#endif
	  if (_reconnect_timeoutid != 0) 
	  {
	       g_source_remove(_reconnect_timeoutid);
	       _reconnect_timeoutid = 0;
	  }
	  g_get_current_time(_reconnect_timeval);
	  
	  _state = Connecting;
	  if (!_hostResolved) 
	  {
#ifdef WITH_IPV6
	       _host_sockaddr.sin6_family = AF_INET6;
	       _host_sockaddr.sin6_port = g_htons(cport);
#else
	       _host_sockaddr.sin_family = AF_INET;
	       _host_sockaddr.sin_port = g_htons(cport);
#endif
	       // resolve host
	       _async_resolve(chost.c_str());
	  }
	  else 
	  {
	       // we don't need to resolve the host again
	       on_host_resolved(NULL, G_IO_NVAL, this);
	  }
     }
     else if(_state == Connected) 
     {
	  // get current time
	  g_get_current_time(_reconnect_timeval);
	  evtConnected();
     }
}

void TCPTransmitter::listen(const string& host, guint port)
{
#ifdef WITH_IPV6
    int fd = socket(PF_INET6, SOCK_STREAM, 0);
    struct sockaddr_in6 name;

	name.sin6_family = AF_INET6;
	name.sin6_flowinfo = 0;
	name.sin6_port = g_htons(port);
	name.sin6_addr = in6addr_any;
#else
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in name;

	name.sin_family = AF_INET;
	name.sin_port = g_htons(port);
	name.sin_addr.s_addr = g_htonl(INADDR_ANY);
#endif


    socklen_t length = sizeof(name);
    if(bind(fd, (struct sockaddr*)&name, length) < 0)
    {
        handleError("Can not bind socket");
        return;
    }

    if (port == 0)
    {
#ifdef WITH_IPV6
	struct sockaddr_in6 sa;
#else
        struct sockaddr_in sa;
#endif
        length = sizeof(sa);
        if(::getsockname(fd, (struct sockaddr*)&sa, &length) < 0)
        {
            handleError("Can not retrieve local port number");
            close(fd);
            return;
        }
#ifdef WITH_IPV6
        _port = ntohs(sa.sin6_port);
#else
        _port = ntohs(sa.sin_port);
#endif
    }
    else
    {
	 _port = port;
    }

    if(::listen(fd, 10) < 0)
    {
        handleError("Could not listen with socket");
        close(fd);
        return;
    }

    _state = Listening;
    _socketfd = fd;
    
    _socket_watchid = g_io_add_watch(g_io_channel_unix_new(_socketfd),
            GIOCondition (G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL),
            &TCPTransmitter::on_socket_accept, this);
}

void TCPTransmitter::disconnect()
{
#ifdef TRANSMITTER_DEBUG
     cout << "disconnect() called!" << endl;
#endif
     
    if (_state == Offline)
    {
        // Lame
        return;
    }
    if (_socket_watchid > 0)
    {
        g_source_remove(_socket_watchid);
        _socket_watchid = 0;
    }
	// remove the poll function
	if (_poll_eventid > 0) 
	{
	    g_source_remove(_poll_eventid);
	    _poll_eventid = 0;
	}

	if (_send_retry_eventid > 0)
    {
	    g_source_remove(_send_retry_eventid);
	    _send_retry_eventid = 0;
	}
    send_data_buf* data;
    while (!_sendBuffer.empty() && _state != ProxyConnecting)
    {
        data = _sendBuffer.front();
        cout << "Deleteing " << data->sz << endl;
        delete data->data;
        delete data;
        _sendBuffer.pop_front();
    }
	       
#ifdef WITH_SSL
    if (_adapter) 
	{
	    _adapter->disconnect();
	    delete _adapter;
	    _adapter = NULL;
    }
#endif
	  
	// close socket
	if (_socketfd != -1) 
	{
	    close(_socketfd);
	}
	_socketfd = -1;
     
    if (_state != Reconnecting && _state != ProxyConnecting)
    {
        // close IOChannel
        if (_IOChannel != NULL) 
        {
            g_io_channel_close(_IOChannel);
        }
        _state = Offline;
        if (_reconnect_timeoutid) 
        {
            g_source_remove(_reconnect_timeoutid);
            _reconnect_timeoutid = 0;
            _reconnect_delay = 5;
        }
    	evtDisconnected();
    }
}

void TCPTransmitter::startPoll()
{
    // add poll function to event loop
    // poll every 15sec
    _poll_eventid = g_timeout_add(150000, &TCPTransmitter::_connection_poll, this);
#ifdef TRANSMITTER_DEBUG
    cout << "Poll function added to event loop" << endl;
#endif
}

// Transmitter methods

void TCPTransmitter::sendsz(const gchar* data, guint sz)
{
     send_data_buf* send_data = new send_data_buf;
     send_data->data = new char[sz];
     memcpy(send_data->data, data, sz);
     send_data->sz = sz;
     send_data->cur_pos = 0;

     _send(send_data);
}

void TCPTransmitter::send(const gchar* data)
{
     send_data_buf* send_data = new send_data_buf;
     send_data->data = g_strdup(data);
     send_data->sz = strlen(data);
     send_data->cur_pos = 0;

     _send(send_data);
}

void TCPTransmitter::needSend(bool notify)
{
    _need_send = notify;
    if (notify)
    {
        if (_socket_watchid)
            g_source_remove(_socket_watchid);
        _socket_watchid = g_io_add_watch(_IOChannel, GIOCondition (G_IO_IN | G_IO_PRI | G_IO_OUT | G_IO_ERR | G_IO_HUP | G_IO_NVAL), &TCPTransmitter::on_socket_event, this);
    } else {
        if (_socket_watchid)
            g_source_remove(_socket_watchid);
        _socket_watchid = g_io_add_watch(_IOChannel, GIOCondition (G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL), &TCPTransmitter::on_socket_event, this);
    }
}

void TCPTransmitter::_send(send_data_buf* data)
{
     guint bytes_written = 0;
     // Write the data to the socket using glib
     GIOError result = G_IO_ERROR_NONE;

    if ((_state != Offline) && (_state != Error))
    {
        if (data != NULL)
        {
            _sendBuffer.push_back(data);
        }

	// if we connect through a proxy we shouldn't send data before we
	// have connection to the destination
	if (_proxy.type != none && _state != Connected && _state != Listening)
	{
	    return;
	}
    }
    else
    {
        delete data->data;
        delete data;

        // XXX Bounce?
        handleError("Sending data while offline");
        return;
     }
     
     
    send_data_buf* send_data = NULL;
    while (!_sendBuffer.empty())
    {
        if(send_data == NULL)
            send_data = _sendBuffer.front();

#ifdef WITH_SSL
        if(_adapter) 
        {
            result = _adapter->send(send_data->data + send_data->cur_pos, 
                                    send_data->sz - send_data->cur_pos,
				                    &bytes_written);
        }
        else 
        {
#endif
        g_assert(_IOChannel != NULL);
        result = g_io_channel_write(_IOChannel, 
                                    send_data->data + send_data->cur_pos, 
                                    send_data->sz - send_data->cur_pos,
				                    &bytes_written);
#ifdef WITH_SSL
        }
#endif // WITH_SSL
        if ((result == G_IO_ERROR_NONE || result == G_IO_ERROR_AGAIN) &&
            bytes_written > 0)
        {
            // Tell the client that we sent some data
            evtDataSent(bytes_written);
            // Update the sent block and delete it necessary
            send_data->cur_pos += bytes_written;
            // This really shouldn't be great than, but we should check anyway
            if (send_data->cur_pos >= send_data->sz)
            {
                // It's done, destroy it
                delete send_data->data;
                delete send_data;
                send_data = NULL;
                _sendBuffer.pop_front();
            }

            if (result == G_IO_ERROR_AGAIN)
            {
                needSend(true);
                return;
            }
        } else {
#ifdef WITH_SSL
	        if(_adapter) 
	        {
	            handleError(_adapter->getError());
	        } else {
#endif
	        handleError(errSocket);
#ifdef WITH_SSL
            }
#endif // WITH_SSL
            disconnect();
            return;
        }
    }

    // We're done with all the data
    needSend(_need_send);
}

// Socket related callbacks
gboolean TCPTransmitter::on_socket_accept(GIOChannel* iochannel, GIOCondition cond, gpointer data) 
{
    TCPTransmitter& transmitter = *(static_cast<TCPTransmitter*>(data));
     
    g_source_remove(transmitter._socket_watchid);
     
    if (cond != G_IO_IN)
    {
        transmitter.handleError("Accepting socket died");
        return (FALSE);
    }

    transmitter._state = Accepting;

    // Ok we're ready to accept
#ifdef WITH_IPV6
    struct sockaddr_in6 clientname;
#else
    struct sockaddr_in clientname;
#endif
	socklen_t size;

    size = sizeof(clientname);
    int fd = accept(transmitter._socketfd, 
                    (struct sockaddr*)&clientname, &size);
    if (fd < 0)
    {
        transmitter.handleError("Error accepting new socket");
        transmitter.disconnect();
        return (FALSE);
    }

    // Replace socketfd with the new one and cleanup the old
    close(transmitter._socketfd);
    transmitter._socketfd = fd;

    transmitter._IOChannel = g_io_channel_unix_new(transmitter._socketfd);
    transmitter._socket_watchid = g_io_add_watch(transmitter._IOChannel,
            GIOCondition (G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL),
            &TCPTransmitter::on_socket_event, data);

    // Finally fire our event
    transmitter.evtAccepted();

    return (FALSE);
}

gboolean TCPTransmitter::on_socket_event(GIOChannel* iochannel, GIOCondition cond, gpointer data) 
{
    TCPTransmitter& transmitter = *(static_cast<TCPTransmitter*>(data));

    if (!transmitter._state == Connected)
    {
        // If we aren't connected don't do anything and return false to get 
        //rid of the socket callback
        return FALSE;
    }
     
    switch(cond) 
    {
    case G_IO_IN:
    {
	// Read the data from the socket and push it into the session
        guint bytes_read = 2047;
	guint bytes_count = 0;
        char buf[2048];

        while (bytes_read == 2047) 
        {
            // Ensure buffer is empty
            memset(buf, 0, 2048);
            // Read data from the channel
            GIOError result;
#ifdef WITH_SSL
            if (transmitter._adapter && transmitter._state != ProxyConnecting) 
            {
                result = transmitter._adapter->read(buf, 2047, &bytes_read);
            } else {
#endif
		g_assert(iochannel != NULL);
		result = g_io_channel_read(iochannel, buf, 2047, &bytes_read);
#ifdef WITH_SSL
            }
#endif
	    bytes_count += bytes_read;

	    if (((result == G_IO_ERROR_AGAIN) || (result == G_IO_ERROR_NONE)) &&
                 (bytes_read > 0)) 
	    {
		// maybe we wait for the proxy to connect and have to skip its header lines
		if (transmitter._state != ProxyConnecting)
		{
		     transmitter.evtDataAvailable(buf, bytes_read);
		}
		else
		{
		     transmitter.proxyHandshakeIn(buf, bytes_read);
		}
	    } else {
		if (bytes_count == 0)
		{
		    transmitter.disconnect();
		    return (FALSE);
		}
		if ((result != G_IO_ERROR_AGAIN) && (result != G_IO_ERROR_NONE))
		{
#ifdef WITH_SSL
		    if(transmitter._adapter) 
		    {
			transmitter.handleError(transmitter._adapter->getError());
		    } else {
#endif
			transmitter.handleError(errSocket);
#ifdef WITH_SSL
		    }
#endif
		    return (FALSE);
		}
	    }
        }
	return (TRUE);
    }
    // We can write, send out our data
    case G_IO_OUT:
        if (transmitter._sendBuffer.empty())
        {
            transmitter.evtCanSendMore();
        } else {
            // Attempt to clear out our buffer
            transmitter._send(NULL);
        }
        return (TRUE);
    default:
	    transmitter.handleError(errSocket);
        return (FALSE);
    }
     
    return (FALSE);
}

gboolean TCPTransmitter::on_socket_connect(GIOChannel* iochannel, GIOCondition cond, gpointer data)
{
     TCPTransmitter& transmitter = *(static_cast <TCPTransmitter*>(data));
   
     // remove watch, in case we don't return immediately to avoid being
     // called more than once
     // (since we always return false, we can do it here already)
     g_source_remove(transmitter._socket_watchid);
     
     if (!transmitter._state == Connecting)
     {
          // Occurs when the Transmitter is disconnected in the process of connecting.  Return to prevent
          // an error from being triggered 
          return FALSE;
     }
     
     if ((cond & G_IO_IN) || (cond & G_IO_OUT)) 
     {
	  // was there an error?
	  gint error;
	  socklen_t len;
	  len = sizeof(error);
	  if (getsockopt(transmitter._socketfd, SOL_SOCKET, SO_ERROR,
			 static_cast <void*>(&error), &len) >= 0) 
	  {
	       if (!error) 
	       {

		    
#ifdef TRANSMITTER_DEBUG
		    cout << "Async connect worked!" << endl;
#endif
		    

		    // proxy connect?
		    if (transmitter._proxy.type == none)
		    {
                         transmitter._state = Connected;
		    }
		    else
		    {
			 transmitter.proxyHandshakeOut();
		    }
		    
		    // register socket for notification
#ifdef WITH_SSL
		    if (transmitter._adapter) 
		    {
			 // FIXME: reset the socket flags to blocking 
			 // (we currently need that for OpenSSL to work properly)
			 if ((fcntl(transmitter._socketfd, F_SETFL, transmitter._socket_flags)) == -1 ) 
			 {
#ifdef TRANSMITTER_DEBUG
			      cout << "Couldn't reset socket flags!" << endl;
#endif
			      transmitter.handleError(strerror(errno));
			      return(FALSE);
			 }
			 bool success =
			      transmitter._adapter->registerSocket(transmitter._socketfd);
			 if(!success) 
			 {
			      transmitter.handleError(transmitter._adapter->getError());
			 }
		    }
#endif
		    
		    transmitter._socket_watchid = g_io_add_watch(transmitter._IOChannel,
								 GIOCondition (G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL),
								 &TCPTransmitter::on_socket_event, data);
		    
		    // get current time
		    g_get_current_time(transmitter._reconnect_timeval);
		    transmitter.evtConnected();
		    return (FALSE);
	       }
	       else /*if(!error)*/ 
	       {
		    transmitter.handleError(strerror(error));
	       }
	  }
	  else /*if(getsockopt() >= 0)*/ 
	  {
	       transmitter.handleError(strerror(errno));
	  }
     }
     else /*if ((cond & G_IO_IN) || (cond & G_IO_OUT))*/ 
     {
	  // there was some nasty error...
	  
#ifdef TRANSMITTER_DEBUG
	  cout << "Async connect failed: " << cond << endl;
#endif
	  
	  // close the socket
	  transmitter.handleError(errSocket);
	  g_io_channel_close(transmitter._IOChannel);
	  transmitter._socketfd = -1;
     }
     
     return (FALSE);
}

gboolean TCPTransmitter::on_host_resolved(GIOChannel* iochannel, GIOCondition cond, gpointer data)
{
     TCPTransmitter& transmitter = *(static_cast <TCPTransmitter*>(data));
     bool error = false;
     
     // remove the watch on the IO channel
     if (transmitter._resolver_watchid > 0)
          g_source_remove(transmitter._resolver_watchid);

     // if _hostResolved == true it means the address was already resolved
     if (!transmitter._hostResolved && transmitter._state == Connecting) 
     {
	  if (cond & G_IO_IN) 
	  {
	       // read from the channel
	       
#ifdef TRANSMITTER_DEBUG
	       cout << "reading from pipe" << endl;
#endif
	       
#ifdef WITH_IPV6
	       guint buf_length = sizeof(guchar) + sizeof(struct in6_addr);
#else
	       guint buf_length = sizeof(guchar) + sizeof(struct in_addr);
#endif
	       gchar * buffer = new gchar[buf_length];
	       guint bytes_read = 0;
	       GIOError result = g_io_channel_read(iochannel, buffer, buf_length, &bytes_read);
	       if (result != G_IO_ERROR_NONE) 
	       {

#ifdef TRANSMITTER_DEBUG
		    cout << "Error while reading from pipe" << endl;
#endif
		    error = true;
	       }
	       else if (buffer[0] == 0) 
	       {
		    // the hostname couldn't be resolved properly
		    
#ifdef TRANSMITTER_DEBUG
		    cout << "read from pipe got me a 0!" << endl;
#endif
		    error = true;
	       }
	       else if (bytes_read != buf_length) 
	       {
		    // something nasty happened, bail out!
#ifdef TRANSMITTER_DEBUG
		    cout << "Didn't receive enough data from resolver process!\n" << "Received: " << bytes_read << " Wanted: " << buf_length << endl;
#endif
		    //when this happens wait a second and try again
		    sleep(1);
		    guint next_bytes_read;
		    result = g_io_channel_read(iochannel, &buffer[bytes_read], buf_length - bytes_read, &next_bytes_read);
		    if (result == G_IO_ERROR_NONE) 
		    {
			 bytes_read += next_bytes_read;
			 if (bytes_read != buf_length) 
			 {
			      error = true;
#ifdef TRANSMITTER_DEBUG
			      cout << "Not enough data on retry. Received: " << bytes_read << " Wanted: " << buf_length << endl;
#endif
			 }
		    }
		    else 
		    {
			 error = true;
		    }
		    
	       }
	       
	       if (!error) 
	       {
		    // copy the received struct in_addr into _host_sockaddr.sin_addr
#ifdef WITH_IPV6
		    memcpy(static_cast <void*>(&transmitter._host_sockaddr.sin6_addr),
			   static_cast <void*>(&buffer[sizeof(guchar)]), sizeof(struct in6_addr));
#else
		    memcpy(static_cast <void*>(&transmitter._host_sockaddr.sin_addr),
			   static_cast <void*>(&buffer[sizeof(guchar)]), sizeof(struct in_addr));
#endif
#ifdef TRANSMITTER_DEBUG
		    cout << "wrote in_addr into _host_sockaddr" << endl;
		    cout << "got the following from the pipe:" << endl;
#ifdef WITH_IPV6
		    cout << "sin6_family: " << transmitter._host_sockaddr.sin6_family << endl;
		    cout << "sin6_port:   " << transmitter._host_sockaddr.sin6_port << endl;
		    char name_buffer[INET6_ADDRSTRLEN];
		    if (inet_ntop(AF_INET6, transmitter._host_sockaddr.sin6_addr.s6_addr, name_buffer, INET6_ADDRSTRLEN))
			 cout << "sin6_addr:   " << name_buffer << endl;
#else
		    cout << "sin_family: " << transmitter._host_sockaddr.sin_family << endl;
		    cout << "sin_port  : " << transmitter._host_sockaddr.sin_port << endl;
		    cout << "sin_addr  : " << transmitter._host_sockaddr.sin_addr.s_addr << endl;
#endif
#endif
		    transmitter._hostResolved = true;
	       }
	       
	       delete[] buffer;
	  }
	  else 
	  {
	       // should not happen...
	       error = true;
	  }
     }
     
     // close the pipes
     if (iochannel != NULL)
	  g_io_channel_close(iochannel);

     // wait for the resolver process to exit to avoid zombies
     if (transmitter._resolver_pid != -1) 
     {
	  waitpid(transmitter._resolver_pid, NULL, 0);
	  transmitter._resolver_pid = -1;
     }
     
     if(error) 
     {
	  transmitter.handleError(errAddressLookup);
     } 
     else if (transmitter._state == Connecting) 
     {
	  transmitter._async_connect();
     }

     return (FALSE);
}

gboolean TCPTransmitter::reconnect(gpointer data)
{
     TCPTransmitter & t = *(static_cast<TCPTransmitter *>(data));
     if (t._reconnect_timeoutid) 
     { 
	  // this means we haven't reconnected yet
	  t._reconnect_timeoutid = 0;
	  t._state = Reconnecting;
	  t.connect("", 0, t._use_ssl, t._autoreconnect);
     }
     return (FALSE);
}

void TCPTransmitter::handleError(const TransmitterError e) 
{
     string emsg;

     if (_state == Offline || _state == Error)
        return;
     
     switch(e) 
     {
     case errAddressLookup:
	  emsg = "Couldn't Resolve Hostname";
	  break;
     case errSocket:
	  emsg = getSocketError();
	  break;
     default:
	  // shouldn't happen
	  emsg = "Unknown Error";
     }
     
     handleError(emsg);
}

void TCPTransmitter::handleError(const string & emsg)
{
#ifdef TRANSMITTER_DEBUG
     cout << "A Transmitter Error occurred: " << emsg << endl;
#endif
     if (_state == Offline || _state == Error)
        return;
     
     // check if we should auto-reconnect: 
     if (_autoreconnect) 
     {
	  GTimeVal now;
	  g_get_current_time(&now);
	  glong connected_time = now.tv_sec - _reconnect_timeval->tv_sec;
	  if (connected_time < 60) 
	  {
	       // we've been only connected for less than a minute
	       _reconnect_delay *= 2; // double the delay
	  } 
	  else 
	  {
	       _reconnect_delay = 10; // 10 secs 
	  }
	       
#ifdef TRANSMITTER_DEBUG
	  cout << "Disconnected. Attempting auto-reconnect in " << _reconnect_delay << " seconds" << endl;
#endif
	  evtReconnect();
	  _reconnect_timeoutid = g_timeout_add(_reconnect_delay * 1000, &TCPTransmitter::reconnect, static_cast<void*>(this));
	  // set _reconnecting to true to prevent evtDisconnected from being called by disconnect ()
	  _state = Reconnecting;
     } 
     else 
     {
      _state = Error;
	  evtError(emsg);
     }
	 disconnect();
}

const string TCPTransmitter::getSocketError() 
{
     gint error;
     socklen_t len = sizeof(error);
     string emsg;
     if (_socketfd != -1) 
     {
	  if (getsockopt(_socketfd, SOL_SOCKET, SO_ERROR,
			 static_cast <void*>(&error), &len) >= 0) {
	       if (error) 
	       {
		    return (strerror(error));
	       }
	  }
     }
     return("Unknown error");
}

void TCPTransmitter::_async_resolve(const gchar* hostname)
{
     g_assert(hostname != NULL);
     // check if hostname is in dotted decimal notation
#ifdef WITH_IPV6
     if (inet_pton(AF_INET6, hostname, &_host_sockaddr.sin6_addr) != 0) 
#else
     if (inet_aton(hostname, &_host_sockaddr.sin_addr) != 0) 
#endif
     {
	  // all done
	  _hostResolved = true;
	  
#ifdef TRANSMITTER_DEBUG
	  cout << "dot-dec resolved" << endl;
#endif
	  
	  on_host_resolved(NULL, G_IO_NVAL, this);
	  
	  return;
     }
     
     // didn't work, we need to do a lookup
     _resolver_pid = -1;
     gint pipes[2];
     
     // create a pipe
     if (pipe(pipes) == -1)
     {
	  handleError(errAddressLookup);
	  return;
     }
     
     errno = 0;
     // try to fork as long as errno == EAGAIN
     do 
     {
	  // fork the lookup process
	  if ((_resolver_pid = fork()) == 0) 
	  {
#ifdef WITH_IPV6
	       struct in6_addr ia;
#else
	       struct in_addr ia;
#endif
	       if (!_gethostbyname(hostname, &ia)) 
	       {
		    guchar zero = 0;
		    if (write(pipes[1], &zero, sizeof(zero)) == -1) 
		    {
			 g_warning("Problem with writing to pipe");
		    }
#ifdef TRANSMITTER_DEBUG
		    cout << "host couldn't be resolved, wrote NULL " << endl;
#endif
	       }
	       else 
	       {
#ifdef WITH_IPV6
		    guchar size = sizeof(struct in6_addr);
#else
		    guchar size = sizeof(struct in_addr);
#endif
		    
#ifdef TRANSMITTER_DEBUG
#ifdef WITH_IPV6
		    char name_buffer[INET6_ADDRSTRLEN];
		    if (inet_ntop(AF_INET6, ia.s6_addr, name_buffer, INET6_ADDRSTRLEN))
			 cout << "wrote to pipe: sin6_addr:   " << name_buffer << endl;
#else
		    cout << "wrote to pipe: sin_addr: " << ia.s_addr << endl;
#endif
#endif
		    
#ifdef WITH_IPV6
		    if ( (write(pipes[1], &size, sizeof(guchar))) == -1 ||
			 (write(pipes[1], &ia, sizeof(struct in6_addr)) == -1)) 
		    {
			 g_warning("Problem with writing to pipe");
		    }
#else 
		    if ( (write(pipes[1], &size, sizeof(guchar))) == -1 ||
			 (write(pipes[1], &ia, sizeof(struct in_addr)) == -1)) 
		    {
			 g_warning("Problem with writing to pipe");
		    }
#endif
		    
		    
#ifdef TRANSMITTER_DEBUG
		    cout << "lookup process done!" << endl;
#endif
		    
	       }
	       // close our end of the pipe
	       close(pipes[1]);
	       
	       // exit (call _exit() to avoid atexit being called)	
	       _exit(EXIT_SUCCESS);
	       
	  }
	  else if (_resolver_pid > 0) 
	  {
	       // parent process creates an IOChannel to read from the pipe
	       _resolver_watchid = g_io_add_watch(g_io_channel_unix_new(pipes[0]),
						  GIOCondition (G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL),
						  &TCPTransmitter::on_host_resolved,
						  this);
	       return;
	  }

	  else if (errno != EAGAIN) 
	  {
	       // nasty error
	       g_warning("Resolver fork error: %s (%d)\n", g_strerror(errno), errno);
	       handleError(errAddressLookup);
	       return;
	  }
	  
     }
     while (errno == EAGAIN);
	  
     return;
}

void TCPTransmitter::_async_connect()
{
     // connect non-blocking
     
     // create socket
#ifdef WITH_IPV6
     _socketfd = socket(PF_INET6, SOCK_STREAM, 0);
#else
     _socketfd = socket(PF_INET, SOCK_STREAM, 0);
#endif
     if (_socketfd < 0) 
     {
	  // something nasty happened
#ifdef TRANSMITTER_DEBUG
	  cout << "socket() failed: " << strerror(errno) << endl;
#endif
	  handleError(strerror(errno));
	  return;
     }
     _socket_flags = fcntl(_socketfd, F_GETFL, 0);
     if (_socket_flags == -1) 
     {
	  // not good
#ifdef TRANSMITTER_DEBUG
	  cout << "fcntl F_GETFL failed on socket: " << strerror(errno) << endl;
#endif
	  handleError(errSocket);
	  return;
     }
     if (fcntl(_socketfd, F_SETFL, _socket_flags | O_NONBLOCK) == -1) 
     {
	  // damn!
#ifdef TRANSMITTER_DEBUG
	  cout << "fcntl F_SETFL failed on socket: " << strerror(errno) << endl;
#endif
	  handleError(strerror(errno));
	  return;
     }
     
     int one = 1;
     if (setsockopt(_socketfd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one)) < 0) 
     {
#ifdef TRANSMITTER_DEBUG
          cout << "setsockopt failed: " << strerror(errno) << endl;
#endif
     }
     
     // try to connect non-blocking
     if (::connect(_socketfd, (struct sockaddr*) (&_host_sockaddr), sizeof(_host_sockaddr)) < 0) 
     {
	  if (errno != EINPROGRESS) 
	  {
	       // Yikes!
#ifdef TRANSMITTER_DEBUG
	       cout << "connect failed: " << strerror(errno) << endl;
#endif	       
	       handleError(strerror(errno));
	       return;
	  }
     }
     
     _IOChannel = g_io_channel_unix_new(_socketfd);
     _socket_watchid = g_io_add_watch(_IOChannel,
				      GIOCondition(G_IO_IN | G_IO_OUT | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL),
				      &TCPTransmitter::on_socket_connect, this);

#ifdef TRANSMITTER_DEBUG
     cout << "IOChannel watch added!" << endl;
#endif
}

#ifdef WITH_IPV6
bool TCPTransmitter::_gethostbyname(const gchar* hostname, struct in6_addr* addr_result)
#else
bool TCPTransmitter::_gethostbyname(const gchar* hostname, struct in_addr* addr_result)
#endif
{
#ifdef HAVE_GETHOSTBYNAME_R_GLIBC
     struct hostent result_buf, *result;
     size_t len;
     char* buf;
     int herr;
     int res;
     
     len = 1024;
     buf = g_new(gchar, len);
     
     while ((res = gethostbyname_r (hostname, &result_buf, buf, len,
				    &result, &herr)) == ERANGE) 
     {
	  len *= 2;
	  buf = g_renew (gchar, buf, len);
     }
     
     if (res || result == NULL || result->h_addr_list[0] == NULL) 
     {
	  g_free(buf);
	  return false;
     }
     
     if (addr_result)
     {
	  memcpy(addr_result, result->h_addr_list[0], result->h_length);
     }
     
     g_free(buf);

#else
#ifdef HAVE_GET_HOSTBYNAME_R_SOLARIS
     struct hostent result;
     size_t len;
     char* buf;
     int herr;
     int res;
     
     len = 1024;
     buf = g_new(gchar, len);
     
     while ((res = gethostbyname_r (hostname, &result, buf, len,
				    &herr)) == ERANGE)
     {
	  len *= 2;
	  buf = g_renew (gchar, buf, len);
     }
     
     if (res || hp == NULL || hp->h_addr_list[0] == NULL) 
     {
	  g_free(buf);
	  return false;
     }
     
     if (addr_result)
     {
	  memcpy(addr_result, result->h_addr_list[0], result->h_length);
     }
     
     g_free(buf);
#else
#ifdef HAVE_GETHOSTBYNAME_R_HPUX
     struct hostent result;
     struct hostent_data buf;
     int res;
     
     res = gethostbyname_r (hostname, &result, &buf);
     
     if (res == 0) 
     {
	  if (addr_result) 
	  {
	       memcpy(addr_result, result.h_addr_list[0], result.h_length);
	  }
     }
     else
	  return false;
#else 
#ifdef HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
     struct hostent* he;
     
     g_static_mutex_lock(&_gethostbyname_mutex);
     he = gethostbyname(hostname);
     g_static_mutex_unlock(&_gethostbyname_mutex);
     
     if (he != NULL && he->h_addr_list[0] != NULL) 
     {
	  if (addr_result)
	  {
	       memcpy(addr_result, he->h_addr_list[0], he->h_length);
	  }
     }
     else
	  return false;
#else
     struct hostent* he;
     
     he = gethostbyname(hostname);
     if (he != NULL && he->h_addr_list[0] != NULL)
     {
	  if (addr_result)
	  {
	       memcpy(addr_result, he->h_addr_list[0], he->h_length);
	  }
     }
     else
	  return false;
#endif // HAVE_GETHOSTBYNAME_R_GLIB_MUTEX
#endif // HAVE_GETHOSTBYNAME_R_HPUX
#endif // HAVE_GET_HOSTBYNAME_R_SOLARIS
#endif // HAVE_GETHOSTBYNAME_R_GLIBC

     return true;
}

gboolean TCPTransmitter::_connection_poll(gpointer data) 
{
     static struct sockaddr addr;
     static socklen_t addr_length = 1;
     
     TCPTransmitter& transmitter = *(static_cast<TCPTransmitter*> (data));
     
#ifdef TRANSMITTER_DEBUG
     cout << "_connection_poll called" << endl;
#endif     
     
     if (transmitter._state == Connected) 
     {
	  transmitter.send(" ");
     }
     
#ifdef POLL_DEBUG     
     if (getpeername(transmitter._socketfd, &addr, &addr_length) != 0) 
     {
	  if (transmitter._state == Connected) 
	  {
	       //an error occurred
	       cout << "_connection_poll: Error: " << strerror(errno) << endl;
	       transmitter.handleError(strerror(errno));
	  } 
	  return (FALSE);  
     }
#endif
     
#ifdef TRANSMITTER_DEBUG
     cout << "_connection_poll: Everything fine" << endl;
#endif
     return (TRUE);
}

const string TCPTransmitter::getsockname()
{
#ifdef WITH_IPV6
     struct sockaddr_in6 sa;
#else
     struct sockaddr_in sa;
#endif
     socklen_t length = sizeof(sa);

     if (_state != Connected)
     {
	  cerr << "transmitter not connected" << endl;
	  return ("");
     }
	
     if(::getsockname(_socketfd, (struct sockaddr*)&sa, &length) < 0)
     {
	  cerr << "error getting socket name" << endl;
	  return ("");
     }
     else
     {
	  struct hostent *hp;
#ifdef WITH_IPV6
	  char namebuffer[INET6_ADDRSTRLEN];
	  
	  hp = gethostbyaddr((char *) &sa.sin6_addr, sizeof(sa.sin6_addr), AF_INET6);
	  if (hp) {
		  return (hp->h_name);
	  }

	  return (string("[")+inet_ntop(AF_INET6, &sa.sin6_addr, namebuffer, sizeof(namebuffer))+"]");

#else
	  char namebuffer[INET_ADDRSTRLEN];

	  hp = gethostbyaddr((char *) &sa.sin_addr, sizeof(sa.sin_addr), AF_INET);
	  return (inet_ntoa(sa.sin_addr));
#endif
     }
}

void TCPTransmitter::setProxy(const string &ptype, const string &host, guint port, const string &user, const string &password, bool tryOther)
{
     if (_state != Offline)
     {
	  cerr << "Can't change proxy type while not offline" << endl;
	  return;
     }
     
     if (ptype == "none")
          _proxy.type = none;
     else if (ptype == "CONNECT")
	  _proxy.type = httpConnect;
     else if (ptype == "PUT")
	  _proxy.type = httpPut;
     else if (ptype == "POST")
	  _proxy.type = httpPost;
     else if (ptype == "SOCKS4")
	  _proxy.type = socks4;
     else if (ptype == "SOCKS5")
	  _proxy.type = socks5;
     else
     {
	  _proxy.type = none;
	  cerr << "Unknown proxy type requested" << endl;
     }

     _proxy.host = host;
     _proxy.port = port;
     _proxy.user = user;
     _proxy.password = password;

     _proxy.try_other = tryOther;
     _proxy.failed_connect = false;
     _proxy.failed_put = false;
     _proxy.failed_post = false;
     _proxy.failed_socks4 = false;
     _proxy.failed_socks5 = false;

     _hostResolved = false;
}


void TCPTransmitter::proxyHandshakeIn(const char *buf, guint bytes_read)
{
     _proxy.response_line.append(buf, bytes_read);

     switch (_proxy.type)
     {
	  case httpConnect:
	  case httpPut:
	  case httpPost:
	       proxyHandleHttpHead();
	       break;
	  case socks4:
	       proxyHandleSocks4Head();
	       break;
	  case socks5:
	       proxyHandleSocks5Head();
	       break;
	  default:
	       cout << "Unknown proxy type" << endl;
	       _state = Error;
     }

     if (_state == Connected)
     {
	  _send(NULL);
     }
     else if (_state == Error)
     {
	  disconnect();
     }

     return;
}

void TCPTransmitter::proxyHandleSocks5Head()
{
     switch (_proxy.socks5_state)
     {
	  case MethodsSent:
	       proxyHandleSocks5MethodReply();
	       break;
	  case AuthenticationSent:
	       proxyHandleSocks5AuthReply();
	       break;
	  case ConnectCmdSent:
	       proxyHandleSocks5ConnectReply();
	       break;
	  default:
	       cout << "unhandled SOCKS5 state" << endl;
	       _state = Error;
     }

     return;
}

void TCPTransmitter::proxyHandleSocks5MethodReply()
{
     // reply complete?
     if (_proxy.response_line.length() < 2)
	  return;

     // authentication method
     int auth_method = _proxy.response_line[1];

     // erase reply from buffer
     _proxy.response_line.erase(0,2);

     switch (auth_method)
     {
	  case 0: // no authentication required
	       proxySendSocks5Connect();
	       break;
	  case 2: // plaintext authentication
	       proxySendSocks5Auth();
	       break;
	  case 255: // no accepted authentication
	       _state = Error;
	       _proxy.response = 407;
	       break;
	  default:
	       cout << "unexpected reply from SOCKS5 server" << endl;
	       _state = Error;
	       _proxy.response = 500;
     }
}

void TCPTransmitter::proxyHandleSocks5AuthReply()
{
     // reply complete?
     if (_proxy.response_line.length() < 2)
	  return;

     // authentication reply
     int auth_reply = _proxy.response_line[1];

     // erase reply from buffer
     _proxy.response_line.erase(0,2);

     if (auth_reply == 0)
     {
	  proxySendSocks5Connect();
     }
     else
     {
	  cout << "SOCKS5 authentication failed" << endl;
	  _state = Error;
	  _proxy.response = 407;
     }
}

void TCPTransmitter::proxyHandleSocks5ConnectReply()
{
     // correct reply has at least 7 bytes
     if (_proxy.response_line.length() < 7)
	  return;

     int addr_type = _proxy.response_line[3];
     unsigned int min_len = 7;

     switch (addr_type)
     {
	  case 1: // IPv4
	       min_len = 10;
	       break;
	  case 3: // domain
	       min_len = _proxy.response_line[4] + 7;
	       break;
	  case 4: // IPv6
	       min_len = 22;
	       break;
	  default:
	       cout << "Warning: unknown address type from SOCKS5 server";
	       cout << endl;
     }

     if (_proxy.response_line.length() < min_len)
	  return;

     // XXX Save bound address and port

     int reply_code = _proxy.response_line[1];
     _proxy.response_line = "";

     switch (reply_code)
     {
	  case 0: // succeeded
	       _proxy.response = 200;
	       _state = Connected;
	       break;
	  case 1: // general SOCKS server failure
	       _proxy.response = 500;
	       _state = Error;
	       break;
	  case 2: // connection not allowed
	       _proxy.response = 403;
	       _state = Error;
	       break;
	  case 3: // network unreachable
	       _proxy.response = 404;
	       _state = Error;
	       break;
	  case 4: // host unreachable
	       _proxy.response = 404;
	       _state = Error;
	       break;
	  case 5: // connection refused
	       _proxy.response = 404;
	       _state = Error;
	       break;
	  case 6: // TTL expired
	       _proxy.response = 408;
	       _state = Error;
	       break;
	  case 7: // command not supported
	       _proxy.response = 501;
	       _state = Error;
	       break;
	  case 8: // address type not supported
	       _proxy.response = 404;
	       _state = Error;
	       break;
	  default:
	       cout << "unexpected reply from SOCKS5 server" << endl;
	       _proxy.response = 500;
	       _state = Error;
     }
}

void TCPTransmitter::proxySendSocks5Auth()
{
#ifdef HAVE_STD_SSTREAM
     ostringstream header;
#else
     ostrstream header;
#endif

     header << (char)1; // version

     // user
     if (_proxy.user.length()<=255)
     {
	  header << (char)_proxy.user.length();
	  header << _proxy.user;
     }
     else
     {
	  header << (char)255; // length
	  header << _proxy.user.substr(0, 255);
     }

     // password
     if (_proxy.password.length()<=255)
     {
	  header << (char)_proxy.password.length();
	  header << _proxy.password;
     }
     else
     {
	  header << (char)255; // length
	  header << _proxy.password.substr(0, 255);
     }

     _proxy.socks5_state = AuthenticationSent;
     proxySendHead(header.str());
}

void TCPTransmitter::proxySendSocks5Connect()
{
#ifdef HAVE_STD_SSTREAM
     ostringstream header;
#else
     ostrstream header;
#endif

     header << (char)5; // version
     header << (char)1; // connect
     header << (char)0; // reserved
     header << (char)3; // addr type

     // domain
     if (_proxy.dest_host.length()<=255)
     {
	  header << (char)_proxy.dest_host.length();
	  header << _proxy.dest_host;
     }
     else
     {
	  header << (char)255; // length
	  header << _proxy.dest_host.substr(0, 255);
     }

     // port
     header << (char)(_proxy.dest_port/256);
     header << (char)(_proxy.dest_port%256);

     _proxy.socks5_state = ConnectCmdSent;
     proxySendHead(header.str());
}
	
void TCPTransmitter::proxyHandleSocks4Head()
{
     // have we received the complete header?
     if (_proxy.response_line.size()<8)
	  return;

     int reply = _proxy.response_line[1];

#ifdef TRANSMITTER_DEBUG
     cout << "socks reply code: " << reply << endl;
#endif

     switch (reply)
     {
	  case 90: // request granted
	       _proxy.response = 200;
	       break;
	  case 91: // rejected or failed
	       _proxy.response = 403;
	       break;
	  case 92: // can't get ident
	       _proxy.response = 400;
	       break;
	  case 93: // ident != userid
	       _proxy.response = 409;
	       break;
	  default:
	       _proxy.response = 500;
     }

     if (_proxy.response == 200)
     {
	  _state = Connected;
     }
     else
     {
	  cout << "SOCKS4 connect failed. Code ";
	  cout << reply << " (" << _proxy.response << ")" << endl;
	  _state = Error;
     }
     
}
	
void TCPTransmitter::proxyHandleHttpHead()
{
     // have we already read the response code?
     if (!_proxy.response)
     {
#ifdef HAVE_STD_SSTREAM
	  istringstream ist(_proxy.response_line);
#else
	  istrstream ist(_proxy.response_line.c_str());
#endif
	  string skipstring;

	  ist >> skipstring;
	  ist >> _proxy.response;

	  // if we have read the full code it's at least 100
	  if (_proxy.response<100)
	       _proxy.response = 0;
     }

#ifdef TRANSMITTER_DEBUG
     cout << "Proxy-Response-Code: " <<
	     _proxy.response << endl;
#endif

     // erase the CRs from proxy response
     string::size_type pos;
     do
     {
	  pos = _proxy.response_line.find("\r");
	  if (pos != string::npos)
	       _proxy.response_line.erase(pos, 1);
     }
     while (pos != string::npos);

     // have we read the complete header?
     if (_proxy.response_line.find("\n\n") != std::string::npos)
     {
	  // codes 2xx mean we have successfully connected
	  if (_proxy.response>=200
	       && _proxy.response<300)
	  {
#ifdef TRANSMITTER_DEBUG
	       cout << "Proxy connect successful" << endl;
#endif
	       _state = Connected;
	  }
	  else
	  {
	       cout << "Proxy connect failed: Code " <<
		       _proxy.response << endl;


	       // remember what type of connect failed
	       switch (_proxy.type)
	       {
		    case httpConnect:
			  _proxy.failed_connect = true;
			  break;
		    case httpPut:
			  _proxy.failed_put = true;
			  break;
		    case httpPost:
			  _proxy.failed_post = true;
			  break;
		    default:
			  cout << "onknown proxy type" << endl;
	       }

	       // try other http methods?
	       if (_proxy.try_other && !_proxy.failed_connect)
	       {
		    _proxy.type = httpConnect;
	       }
	       else if (_proxy.try_other && !_proxy.failed_put)
	       {
		    _proxy.type = httpPut;
	       }
	       else if (_proxy.try_other && !_proxy.failed_post)
	       {
		    _proxy.type = httpPost;
	       }
	       else
	       {
		    _state = Error;
	       }

	       // reconnect with other proxy type?
	       if (_state != Error)
	       {
		    disconnect();
		    connect("", 0, _use_ssl, _autoreconnect);
	       }
	  }
     }
}

void TCPTransmitter::proxyHandshakeOut()
{
#ifdef HAVE_STD_SSTREAM
     ostringstream proxy_head;
#else
     ostrstream proxy_head;
#endif
     bool isHttpRequest = false;

     _state = ProxyConnecting;

     switch (_proxy.type)
     {
	  case httpConnect:
	       proxy_head << "CONNECT "
			  << _proxy.dest_host
			  << ":"
			  << _proxy.dest_port
			  << " HTTP/1.0";
	       isHttpRequest = true;
	       break;
	  case httpPut:
	       proxy_head << "PUT http://"
			  << _proxy.dest_host
			  << ":"
			  << _proxy.dest_port
			  << "/ HTTP/1.0";
	       isHttpRequest = true;
	       break;
	  case httpPost:
	       proxy_head << "POST http://"
			  << _proxy.dest_host
			  << ":"
			  << _proxy.dest_port
			  << "/ HTTP/1.0";
	       isHttpRequest = true;
	       break;
	  case socks5:
	       proxy_head << (char)5; // socks version
	       if (_proxy.user != "")
	       {
		    proxy_head << (char)2; // two auth methods follow
		    proxy_head << (char)0; // no authentication
		    proxy_head << (char)2; // plain text authentication
	       }
	       else
	       {
		    proxy_head << (char)1; // one auth method follows
		    proxy_head << (char)0; // no authentication
	       }
	       _proxy.socks5_state = MethodsSent;
	       break;
	  case socks4:
	       // XXX: We should _async_resolve this too
#ifdef WITH_IPV6
	       in6_addr serv6addr;
	       if (_gethostbyname(_proxy.dest_host.c_str(), &serv6addr) && IN6_IS_ADDR_V4MAPPED(&serv6addr))
#else
	       in_addr servaddr;
	       if (_gethostbyname(_proxy.dest_host.c_str(), &servaddr))
#endif
	       {
		    proxy_head << (char)4; // socks version
		    proxy_head << (char)1; // CONNECT command
		    proxy_head << (char)(_proxy.dest_port/256);
		    proxy_head << (char)(_proxy.dest_port%256);
#ifdef WITH_IPV6
		    proxy_head << (char)serv6addr.s6_addr[12];
		    proxy_head << (char)serv6addr.s6_addr[13];
		    proxy_head << (char)serv6addr.s6_addr[14];
		    proxy_head << (char)serv6addr.s6_addr[15];
#else
		    proxy_head << (char)(servaddr.s_addr);
		    proxy_head << (char)(servaddr.s_addr>>8);
		    proxy_head << (char)(servaddr.s_addr>>16);
		    proxy_head << (char)(servaddr.s_addr>>24);
#endif
		    proxy_head << "Gabber" << ends; // userid
	       }
	       else
	       {
		    cout << "can't resolve server" << endl;
		    _state=Error;
		    disconnect();
	       }
	       break;
	  default: cerr << "unhandled proxy type" << endl;
     }

     // for http proxies we need some more headers
     if (isHttpRequest)
     {
	  // insert password header?
	  if (_proxy.user != "" && _proxy.password != "")
	  {
	       proxy_head << "\r\nProxy-Authorization: Basic "
		    	  << encodeBase64(_proxy.user + ":" + _proxy.password);
	  }

	  proxy_head << "\r\nHost: "
		     << _proxy.dest_host << ":"
		     << _proxy.dest_port
		     << "\r\nUser-Agent: Gabber/"
		     << VERSION << "\r\n\r\n";
     }

     proxySendHead(proxy_head.str());
}

void TCPTransmitter::proxySendHead(const string& header)
{
     guint bytes_written;

     gchar *sendbuffer = new gchar[header.size()];
     memcpy(sendbuffer, header.c_str(), header.size());
     g_assert(_IOChannel != NULL);
     g_io_channel_write(_IOChannel,
			sendbuffer, header.size(),
			&bytes_written);
}

string TCPTransmitter::encodeBase64(string text)
{
     char *encodings = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef"
	  	       "ghijklmnopqrstuvwxyz0123456789+/";

     string result;
     long block;

     while (text.length() >= 3)
     {
	  block = (text[0]*256+text[1])*256+text[2];

	  result += encodings[block>>18];
	  result += encodings[(block>>12)%64];
	  result += encodings[(block>>6)%64];
	  result += encodings[block%64];

	  text.erase(0,3);
     }

     switch (text.length())
     {
	  case 2:
	  	block = text[0]*256+text[1];
		result += encodings[block>>10];
		result += encodings[(block>>4)%64];
		result += encodings[(block*4)%64];
		result += "=";
		break;
	  case 1:
		block = text[0];
		result += encodings[block>>2];
		result += encodings[(block*16)%64];
		result += "==";
     }

     return result;
}
