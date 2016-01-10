/* SSLAdapter.cc
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

#include "SSLAdapter.hh"

#include <iostream>
#include <string>
#include <openssl/err.h>

/* Begin PSF/EXPTOOLS Solaris fix 
 * 
 * This code is extracted from entropy.c in the openssh package, as
 * modified for exptools. 
 */

/*
 * Copyright (c) 2001 Damien Miller.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Portable OpenSSH PRNG seeding:
 * If OpenSSL has not "internally seeded" itself (e.g. pulled data from 
 * /dev/random), then we execute a "ssh-rand-helper" program which 
 * collects entropy and writes it to stdout. The child program must 
 * write at least RANDOM_SEED_SIZE bytes. The child is run with stderr
 * attached, so error/debugging output should be visible.
 *
 * XXX: we should tell the child how many bytes we need.
 */
#define OPENSSL_PRNG_ONLY
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <openssl/rand.h>

#ifndef OPENSSL_PRNG_ONLY
#define SSL_RAND_HELPER "/opt/exp/lib/openssh/sbin/prngd"
#define RANDOM_SEED_SIZE 48
static uid_t original_uid, original_euid;
#endif

void SSLAdapter::seed_rng()
{
#ifndef OPENSSL_PRNG_ONLY
       int devnull;
       int p[2];
       pid_t pid;
       int ret;
       unsigned char buf[RANDOM_SEED_SIZE];

       if (RAND_status() == 1) {
               cerr << "RNG is ready, skipping seeding" << endl;
               return;
       }

       /* the following call and test won't be needed after OpenSSL 0.9.6c */
       RAND_egd("/etc/entropy");
       if (RAND_status() == 1) {
               cerr << "RNG was seeded from /etc/entropy" << endl;
               return;
       }

       cerr << "Seeding PRNG from " <<  SSL_RAND_HELPER << endl;

       if ((devnull = open("/dev/null", O_RDWR)) == -1) {
               fprintf(stderr,"Couldn't open /dev/null: %s", strerror(errno));
               exit(1);
       }
       if (pipe(p) == -1) {
               fprintf(stderr,"pipe: %s", strerror(errno));
               exit(1);
       }
       if ((pid = fork()) == -1) {
               fprintf(stderr,"Couldn't fork: %s", strerror(errno));
               exit(1);
       }

       /* We don't want the current process to get interrupted
        * by the death of the forked entropy generator.  Save
        * the current signal handler and put it back later 
        */
       SIG_PF old_signal = signal(SIGCHLD, SIG_DFL);
       
       if (pid == 0) {
               dup2(devnull, STDIN_FILENO);
               dup2(p[1], STDOUT_FILENO);
               /* Keep stderr open for errors */
               close(p[0]);
               close(p[1]);
               close(devnull);

               if (original_uid != original_euid && 
                   setuid(original_uid) == -1) {
                       fprintf(stderr, "(rand child) setuid: %s\n", 
                           strerror(errno));
                       _exit(1);
               }
               
               execl(SSL_RAND_HELPER, SSL_RAND_HELPER, "-n", "-c", "/opt/exp/lib/openssh/etc/ssh_prng_cmds", "-i", "48", NULL);
               fprintf(stderr, "(rand child) Couldn't exec '%s': %s\n", 
                   SSL_RAND_HELPER, strerror(errno));
               _exit(1);
       }

       close(devnull);
       close(p[1]);

       memset(buf, '\0', sizeof(buf));
       ret = read( p[0], buf, sizeof(buf));

       if (ret == -1) {
               fprintf(stderr,"Couldn't read from ssh-rand-helper: %s",
                   strerror(errno));
               exit(1);
       }
       if (ret != sizeof(buf)) {
               fprintf(stderr,"ssh-rand-helper child produced insufficient data (%d bytes)",ret);
               exit(1);
       }
       close(p[0]);

       if (waitpid(pid, &ret, 0) == -1) {
              fprintf(stderr,"Couldn't wait for ssh-rand-helper completion: %s", 
                  strerror(errno));
               exit(1);
       }
       signal(SIGCHLD, old_signal);
       
       /* We don't mind if the child exits upon a SIGPIPE */
       if (!WIFEXITED(ret) && 
           (!WIFSIGNALED(ret) || WTERMSIG(ret) != SIGPIPE)) {
               fprintf(stderr,"ssh-rand-helper terminated abnormally");
               exit(1);
       }
       if (WEXITSTATUS(ret) != 0) {
               fprintf(stderr,"ssh-rand-helper exit with exit status %d", ret);
               exit(1);
       }
       RAND_add(buf, sizeof(buf), sizeof(buf));
       memset(buf, '\0', sizeof(buf));

#endif /* OPENSSL_PRNG_ONLY */
       if (RAND_status() != 1) {
               fprintf(stderr,"PRNG is not seeded");
               exit(1);
       }
}


/* End PSF/EXPTOOLS Solaris fix */

using namespace std;

SSLAdapter::SSLAdapter():
     _client_ssl(NULL), _ssl_method(NULL),
     _ssl_client_context(NULL), _connected(false)
{

     // initialize SSL library
     SSL_library_init();
     SSL_load_error_strings();

     seed_rng();

     // construct a ssl method
     _ssl_method = TLSv1_client_method();

     // construct a context
     _ssl_client_context = SSL_CTX_new(_ssl_method);

     // create SSLs
     _client_ssl = SSL_new(_ssl_client_context);
     SSL_set_ssl_method(_client_ssl, _ssl_method);

     g_assert(_client_ssl != NULL && _ssl_method != NULL && _ssl_client_context != NULL);


#ifdef TRANSMITTER_DEBUG
     cerr << "SSLAdapter constructor done." << endl;
#endif
}


SSLAdapter::~SSLAdapter() 
{
     disconnect();
     if(_client_ssl != NULL) {
	  SSL_free(_client_ssl);
     }
     if(_ssl_client_context) {
	  SSL_CTX_free(_ssl_client_context);
     }
     
     ERR_free_strings();
     ERR_remove_state(0);


#ifdef TRANSMITTER_DEBUG
     cerr << "SSLAdapter destructor done." << endl;
#endif
}


void SSLAdapter::disconnect() 
{
#ifdef TRANSMITTER_DEBUG
     cerr << "SSLAdapter disconnecting" << endl;
#endif
     
     if(_connected) {
          _connected = false;
	  SSL_shutdown(_client_ssl);
     }
}


GIOError SSLAdapter::send(const gchar* data, const guint len, guint* written) 
{
     g_assert(_connected);

     int bytes_written = SSL_write(_client_ssl, data, len);
     if(bytes_written < 0) {
	  long errorno = ERR_get_error();
	  
	  _lastError = ERR_error_string(errorno, NULL);
	  *(written) = 0;
#ifdef TRANSMITTER_DEBUG
	  cerr << "SSLAdapter::send error in write! " << _lastError << endl;
#endif
	  return(G_IO_ERROR_UNKNOWN);

     }

     g_assert(bytes_written > 0);
     *(written) = bytes_written;
#ifdef TRANSMITTER_DEBUG
     cerr << "SSLAdapter::send successful" << endl;
#endif
     return(G_IO_ERROR_NONE);
}



GIOError SSLAdapter::read(gchar* buffer, const guint count, guint* bytes_read) 
{
#ifdef TRANSMITTER_DEBUG
       cerr << "trying to read from SSL socket..." << endl;
       cerr << SSL_pending(_client_ssl) << " bytes waiting" << endl;
#endif
     *(bytes_read) = SSL_read(_client_ssl, buffer, count);

     g_assert(_connected);
     
     if(*(bytes_read) < 0) {
	  _lastError = ERR_error_string(ERR_get_error(), NULL);
	  
#ifdef TRANSMITTER_DEBUG
	  cerr << "SSLAdapter::socketRead error: " << _lastError << endl;
#endif
	  return (G_IO_ERROR_UNKNOWN);
     }
     if(*(bytes_read) == count && SSL_pending(_client_ssl) > 0) {
	  return (G_IO_ERROR_AGAIN);
     }

     return (G_IO_ERROR_NONE);
}



bool SSLAdapter::registerSocket(const gint socket)
{
#ifdef TRANSMITTER_DEBUG
     cerr << "SSLAdapter::registerConnection socket: " << socket << endl;
#endif

     if(SSL_set_fd(_client_ssl, socket) < 0) {
	  _lastError = ERR_error_string(ERR_get_error(), NULL);

#ifdef TRANSMITTER_DEBUG
	  cerr << "SSLAdapter::registerConnection SSL_set_fd failed! " << _lastError << endl;
#endif
	  return(false);
     }

     SSL_set_connect_state(_client_ssl);
     cerr <<  SSL_state_string_long(_client_ssl) << endl;

     int connect_error = SSL_connect(_client_ssl);
     cerr << connect_error << endl;
     
     if(connect_error < 0) {
	  _lastError = ERR_error_string(ERR_get_error(), NULL);
#ifdef TRANSMITTER_DEBUG
	  cerr <<  SSL_state_string_long(_client_ssl) << endl;
	  cerr << "SSLAdapter::registerConnection SSL_connect failed! " << _lastError << endl;
#endif
	  return(false);
     }

     
     if(SSL_do_handshake(_client_ssl) < 0) {
	  _lastError = ERR_error_string(ERR_get_error(), NULL);
#ifdef TRANSMITTER_DEBUG
	  cerr << "SSLAdapter::registerConnection SSL_do_handshake failed! " << _lastError << endl;
#endif
	  return(false);
     }

     // print SSL information
       SSL_CIPHER * cipher = NULL;
       X509 * cert = NULL;
       int max_bits = 0;
       int real_bits = 0;

       cipher = SSL_get_current_cipher(_client_ssl);
       real_bits = SSL_CIPHER_get_bits(cipher, &max_bits);
  
       cout << "Version: " << SSL_get_version(_client_ssl) << endl
	    << "Cipher: " << SSL_CIPHER_get_name(cipher) 
	    << " Version: " << SSL_CIPHER_get_version(cipher)
	    << " " << real_bits << "(" << max_bits << ") bits" << endl;
  
       cert = SSL_get_peer_certificate(_client_ssl);
       if(cert) {
	    EVP_PKEY * pkey = X509_get_pubkey(cert);
	    if(pkey) {
		 if(pkey->type == EVP_PKEY_RSA && pkey->pkey.rsa && pkey->pkey.rsa->n) {
		      cout << BN_num_bits(pkey->pkey.rsa->n) << " bit RSA key" << endl;
		 } else if(pkey->type == EVP_PKEY_DSA && pkey->pkey.dsa && pkey->pkey.dsa->p) {
		      cout << BN_num_bits(pkey->pkey.dsa->p) << " bit DSA key" << endl;
		 }
	    }
	    EVP_PKEY_free(pkey);
	    X509_free(cert);
       }


     _connected = true;
     return(true);
}


const string SSLAdapter::getError() 
{
  return (_lastError);
}

