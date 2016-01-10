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

/*
 * GPGInterface
 * Author Brandon Lees <brandon@sci.brooklyn.cuny.edu>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <iostream>
#include <errno.h>
#include <signal.h>
#include <sys/shm.h> 
#include <string.h>
#include <gnome.h>

#include "GPGInterface.hh"

#include "GabberApp.hh" // For G_App->getCfg().gpg.keyserver, nothing else

#ifndef GPG_DEBUG
#define GPG_DEBUG false
#endif

string GPGInterface::_gpg_path;

GPGInterface::GPGInterface()
     : _running(false), _gpg_pid(0), _armor(false), _debug(GPG_DEBUG), _status_offset(0), _error(errNone)
{
     _status_fd = -1;
     _gpg_input_fd = -1;
     _gpg_output_fd = -1;

     find_gpg();
}

GPGInterface::GPGInterface(const string& local_key, const string& passphrase, bool armor)
     : _local_key(local_key), _passphrase(passphrase), _running(false), _gpg_pid(0), _armor(armor), _debug(GPG_DEBUG), _status_offset(0), _error(errNone)
{
     _status_fd = -1;
     _gpg_input_fd = -1;
     _gpg_output_fd = -1;

     find_gpg();
}

GPGInterface::~GPGInterface()
{
     // cleanup the process
     cleanup();
}

bool GPGInterface::sign(SignType type, istream* source, ostream* dest)
{
     list<string> args;

     _error = errNone;
     _input = source;
     _output = dest;

     if (!_local_key.empty())
     {
          args.push_back("--local-user");
          args.push_back(_local_key);
     }

     switch (type)
     {
     case sigNormal:
	  args.push_back("--sign");
	  break;
     case sigClear:
	  args.push_back("--clearsign");
	  break;
     case sigDetach:
	  args.push_back("--detach-sign");
	  break;
     }

     if (!execute(args))
          return false;

     bool sign_done = false, sign_ok = true;

     while (!sign_done)
     {
          if (!wait_event())
          {
	       if (_exit_status != 0)
	            sign_ok = false;
               sign_done = true;
               continue;
          }

          while (process_event() && !sign_done) {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
		    {
			 _error = errShm;
			 sign_ok = false;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
			 sign_done = true;
		    }
                    break;
               case gpgShmGetHidden:
                    if (strcmp(_status_arg, "passphrase.enter") == 0)
                    {
                         // GPG needs the passphrase for the key
                         shm_write_str(_passphrase.c_str());
                         break;
                    }
                    break;
	       case gpgBadPass:
		    cerr << "Bad passphrase" << endl;
		    if (!gpgNeedPassphrase(_passphrase))
		    {
			 _error = errPass;
			 sign_ok = false;
			 sign_done = true;
		    }
		    break;
	       // We don't do anything with these messages
	       case gpgNeedPass:
	       case gpgGoodPass:
	       case gpgSigCreated:
		    break;
               default:
		    cerr << "Got unknown message " << (unsigned long) _status_type << endl;
                    break;
               }
          }
     }
     return sign_ok;
}

bool GPGInterface::encrypt(const string& recipient, istream* source, ostream* dest, bool sign)
{
     list<string> args;

     _error = errNone;
     _input = source;
     _output = dest;

     if (sign)
     {
	  if (!_local_key.empty())
	  {
               args.push_back("--local-user");
               args.push_back(_local_key);
	  }
          args.push_back("--sign");
     }
     args.push_back("--recipient");
     args.push_back(recipient);
     args.push_back("--encrypt");
     // This causes problems for people who have multiple keys.  Could consider making
     // it an option.  Is the space savings really worth it?
     // args.push_back("--throw-keyid");


     if (!execute(args))
          return false;

     bool encrypt_done = false, encrypt_ok = true;

     while (!encrypt_done)
     {
          if (!wait_event())
          {
	       if (_exit_status != 0)
                    encrypt_ok = false;
               encrypt_done = true;
               continue;
          }

          while (process_event() && !encrypt_done) {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
		    {
			 _error = errShm;
			 encrypt_ok = false;
			 encrypt_done = true;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
		    }
                    break;
               case gpgShmGetHidden:
                    if (strcmp(_status_arg, "passphrase.enter") == 0)
                    {
                         // GPG needs the passphrase for the key
                         shm_write_str(_passphrase.c_str());
                         break;
                    }
                    break;
	       case gpgShmGetBool:
		    if (strcmp(_status_arg, "untrusted_key.override") == 0)
		    {
		         shm_write_bool(1);
		    }
		    break;
               case gpgBadPass:
		    if (!gpgNeedPassphrase(_passphrase))
		    {
		         _error = errPass;
                         encrypt_ok = false;
                         encrypt_done = true;
		    }
                    break;
	       case gpgBeginEncryption:
	       case gpgEndEncryption:
	       case gpgNeedPass:
	       case gpgGoodPass:
		    break;
               default:
		    cerr << "Got unknown message " << (unsigned long) _status_type << endl;
                    break;
               }
          }
     }
     return encrypt_ok;
}

bool GPGInterface::decrypt(DecryptInfo& decrypt, istream* source, ostream* dest)
{
     list<string> args;

     _error = errNone;
     _input = source;
     _output = dest;

     if (!execute(args))
          return false;

     bool decrypt_done = false, decrypt_ok = false;

     decrypt.reset();
     while (!decrypt_done)
     {
          if (!wait_event())
          {
	       if (_exit_status != 0)
		   decrypt_ok = false;
               decrypt_done = true;
               continue;
          }

          while (process_event()) {
               switch (_status_type)
               {
	       case gpgShmInfo:
                    if (!shm_init())
		    {
			 _error = errShm;
			 decrypt_done = true;
			 decrypt_ok = false;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
		    }
                    break;
               case gpgShmGetHidden:
                    if (strcmp(_status_arg, "passphrase.enter") == 0)
                    {
                         // GPG needs the passphrase for the key
                         shm_write_str(_passphrase.c_str());
                         break;
                    }
                    break;
	       case gpgBadPass:
		    if (!gpgNeedPassphrase(_passphrase))
		    {
		         _error = errPass;
		         decrypt_ok = false;
		         decrypt_done = true;
		    }
		    break;
	       case gpgNoSecKey:
		    _error = errSecKey;
		    break;
	       case gpgDecryptionFailed:
		    decrypt_ok = false;
		    decrypt._valid = false;
		    break;
	       case gpgDecryptionOkay:
		    decrypt._valid = true;
                    decrypt_ok = true;
                    break;
               case gpgSigExpired:
                    decrypt._sig._key._expired = true;
                    break;
               case gpgSigID:
		    decrypt._has_sig = true;
		    break;
	       case gpgGoodSig:
		{
		    // Good Signature, status_arg contains key id and user id
                    string arg = _status_arg;
                    decrypt._sig._key._keyid = arg.substr(0, arg.find(" "));
                    decrypt._sig._key._userid = arg.substr(arg.find(" ") + 1);
                    break;
		}
               case gpgBadSig:
                {
                    string arg = _status_arg;
                    decrypt._sig._valid = false;
                    decrypt._sig._key._keyid = arg.substr(0, arg.find(" "));
                    decrypt._sig._key._userid = arg.substr(arg.find(" ") + 1);
                    _error = errSig;
                    decrypt_ok = false;
                    break;
                }
               case gpgErrSig:
                {
                    string arg = _status_arg;
                    decrypt._sig._valid = false;
                    decrypt._sig._key._keyid = arg.substr(0, arg.find(" "));
                    string::size_type s = arg.rfind(" ");
                    decrypt._sig._timestamp = atol(arg.substr(arg.rfind(" ", s) + 1, s).c_str());
		    break;
		}
               case gpgValidSig:
		{
                    string arg = _status_arg;
                    decrypt._sig._valid = true;
                    decrypt._sig._fingerprint = arg.substr(0, arg.find(" "));
                    decrypt._sig._timestamp = atol(arg.substr(arg.rfind(" ") + 1).c_str());
                    break;
                }
               case gpgTrustUltimate:
                    decrypt._sig._key._trust = GPGInterface::trustUltimate;
                    break;
               case gpgTrustFull:
                    decrypt._sig._key._trust = GPGInterface::trustFull;
                    break;
               case gpgTrustMarginal:
                    decrypt._sig._key._trust = GPGInterface::trustMarginal;
                    break;
               case gpgTrustNever:
                    decrypt._sig._key._trust = GPGInterface::trustNever;
                    break;
               case gpgTrustUndefined:
                    decrypt._sig._key._trust = GPGInterface::trustUndefined;
                    break;
	       case gpgBadMdc:
		    decrypt_ok = false;
		    break;
	       case gpgErrMdc:
		    decrypt_ok = false;
		    break;
	       case gpgNoData:
		    decrypt_ok = false;
		    decrypt._valid = false;
		    break;
	       case gpgBeginDecryption:
		    decrypt._has_sig = false;
		    break;
               case gpgShmGet:
		    // if gpg is asking for trust, just send it TRUST_UNDEFINED for now
		    if (strcmp(_status_arg, "edit_ownertrust.value") == 0)
			 shm_write_str("1");
		    break;
	       case gpgNeedPass:
	       case gpgGoodPass:
	       case gpgEndDecryption:
	       case gpgEncTo:
	       case gpgGoodMdc:
		    break;
               default:
		    cerr << "Got unknown message " << (unsigned long) _status_type << endl;
                    break;
               }
          }
     }
     return decrypt_ok;
}

bool GPGInterface::verify(SigInfo& sig, istream* source, ostream* dest)
{
     list<string> args;

     _error = errNone;
     _input = source;
     _output = dest;

     if (!execute(args))
          return false;

     bool verify_done = false, verify_ok = true;

     sig.reset();
     while (!verify_done)
     {
          if (!wait_event())
          {
	       if (_exit_status != 0)
		     verify_ok = false;
               verify_done = true;
               continue;
          }

          while (process_event()) {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
		    {
			 _error = errShm;
			 verify_ok = false;
			 verify_done = true;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
		    }
                    break;
	       case gpgSigExpired:
		    // The key used to make the signature has expired
		    sig._key._expired = true;
		    break;
	       case gpgSigID:
		    // Not sure if this line contains any useful information
		    break;
	       case gpgGoodSig:
		{
		    // Good Signature, status_arg contains key id and user id
		    string arg = _status_arg;
		    sig._key._keyid = arg.substr(0, arg.find(" "));
		    sig._key._userid = arg.substr(arg.find(" ") + 1);
		    break;
		}
	       case gpgBadSig:
		{
		    string arg = _status_arg;
		    sig._valid = false;
		    sig._key._keyid = arg.substr(0, arg.find(" "));
		    sig._key._userid = arg.substr(arg.find(" ") + 1);
		    _error = errSig;
		    verify_ok = false;
		    break;
		}
	       case gpgErrSig:
		{
		    string arg = _status_arg;
		    sig._valid = false;
		    sig._key._keyid = arg.substr(0, arg.find(" "));
		    string::size_type s = arg.rfind(" ");
		    sig._timestamp = atol(arg.substr(arg.rfind(" ", s) + 1, s).c_str());
		    break;
		}
	       case gpgValidSig:
		{
		    string arg = _status_arg;
		    sig._valid = true;
		    sig._fingerprint = arg.substr(0, arg.find(" "));
		    sig._timestamp = atol(arg.substr(arg.rfind(" ") + 1).c_str());
		    break;
		}
	       case gpgTrustUltimate:
		    sig._key._trust = GPGInterface::trustUltimate;
		    break;
	       case gpgTrustFull:
		    sig._key._trust = GPGInterface::trustFull;
		    break;
	       case gpgTrustMarginal:
		    sig._key._trust = GPGInterface::trustMarginal;
		    break;
	       case gpgTrustNever:
		    sig._key._trust = GPGInterface::trustNever;
		    break;
	       case gpgTrustUndefined:
		    sig._key._trust = GPGInterface::trustUndefined;
		    break;
	       case gpgNoPubKey:
		    _error = errPubKey;
		    verify_ok = false;
		    verify_done = true;
		    break;
	       case gpgShmGet:
		    // if gpg is asking for trust, just send it TRUST_UNDEFINED for now
		    if (strcmp(_status_arg, "edit_ownertrust.value") == 0)
			 shm_write_str("1");
		    break;
               default:
		    cerr << "Got Unknown message " << (unsigned long) _status_type << endl;
                    break;
               }
          }
     }
     return verify_ok;
}

bool GPGInterface::verify_detached(SigInfo& sig, const string& filename, istream* source)
{
     list<string> args;

     _error = errNone;
     _input = source;
     _output = NULL;

     if (!execute(args))
          return false;

     bool verify_done = false, verify_ok = true;

     sig.reset();
     while (!verify_done)
     {
          if (!wait_event())
          {
               if (_exit_status != 0)
                    verify_ok = false;
               verify_done = true;
               continue;
          }

          while (process_event()) {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
                    {
                         _error = errShm;
                         verify_ok = false;
                         verify_done = true;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
                    }
                    break;
               case gpgShmGet:
	            if (strcmp(_status_arg, "detached_signature.filename") == 0)
	            {
	                 // GPG needs the passphrase for the key
	                 shm_write_str(filename.c_str());
	            }
		    else if (strcmp(_status_arg, "edit_ownertrust.value") == 0)
			 shm_write_str("1");
	            break;
               case gpgSigExpired:
                    // The key used to make the signature has expired
                    sig._key._expired = true;
                    break;
               case gpgSigID:
                    // Not sure if this line contains any useful information
                    break;
               case gpgGoodSig:
                {
                    // Good Signature, status_arg contains key id and user id
                    string arg = _status_arg;
                    sig._key._keyid = arg.substr(0, arg.find(" "));
                    sig._key._userid = arg.substr(arg.find(" ") + 1);
                    break;
                }
	       case gpgBadSig:
	        {
	            string arg = _status_arg;
	            sig._valid = false;
	            sig._key._keyid = arg.substr(0, arg.find(" "));
	            sig._key._userid = arg.substr(arg.find(" ") + 1);
	            _error = errSig;
                    verify_ok = false;
                    break;
                }
               case gpgErrSig:
                {
                    string arg = _status_arg;
                    sig._valid = false;
                    sig._key._keyid = arg.substr(0, arg.find(" "));
                    string::size_type s = arg.rfind(" ");
                    sig._timestamp = atol(arg.substr(arg.rfind(" ", s) + 1, s).c_str());
                    break;
                }
               case gpgValidSig:
                {
                    string arg = _status_arg;
                    sig._valid = true;
                    sig._fingerprint = arg.substr(0, arg.find(" "));
                    sig._timestamp = atol(arg.substr(arg.rfind(" ") + 1).c_str());
                    break;
                }
               case gpgTrustUltimate:
                    sig._key._trust = GPGInterface::trustUltimate;
                    break;
               case gpgTrustFull:
                    sig._key._trust = GPGInterface::trustFull;
                    break;
               case gpgTrustMarginal:
                    sig._key._trust = GPGInterface::trustMarginal;
                    break;
               case gpgTrustNever:
                    sig._key._trust = GPGInterface::trustNever;
                    break;
               case gpgTrustUndefined:
                    sig._key._trust = GPGInterface::trustUndefined;
                    break;
               case gpgNoPubKey:
                    _error = errPubKey;
                    verify_ok = false;
                    verify_done = true;
                    break;
               default:
                    cerr << "Got Unknown message " << (unsigned long) _status_type << endl;
                    break;
               }
          }
     }
     return verify_ok;
}


bool GPGInterface::import_key(KeyInfo& info, istream* key)
{
     list<string> args;

     _error = errNone;
     _input = key;
     _output = NULL;

     args.push_back("--import");

     if (!execute(args))
          return false;

     // Import is only successful if we get a IMPORTED status message
     bool import_done = false, import_ok = false;

     info.reset();
     while (!import_done)
     {
          if (!wait_event())
          {
               if (_exit_status != 0)
                    import_ok = false;
               import_done = true;
               continue;
          }

          while (process_event()) {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
                    {
                         _error = errShm;
                         import_ok = false;
                         import_done = true;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
                    }
                    break;
	       case gpgImported:
		{
		    import_ok = true;

		    string arg = _status_arg;
		    info._keyid = arg.substr(0, arg.find(" "));
		    info._userid = arg.substr(arg.find(" ") + 1);
		    break;
		}
	       case gpgNoData:
		    _error = errNoData;
		    import_ok = false;
		    import_done = true;
		    break;
	       default:
		    cerr << "Got Unknown message " << (unsigned long) _status_type << endl;
		    break;
               }
          }
     }
     return import_ok;
}

bool GPGInterface::key_info_from_key(KeyInfo& info, istream* key)
{
     list<string> args;
#ifdef HAVE_STD_SSTREAM
     stringstream output;
#else
     strstream output;
#endif

     _error = errNone;
     _input = key;
     _output = &output;

     args.push_back("--with-colons");

     if (!execute(args))
          return false;

     bool info_done = false, info_ok = true;

     info.reset();
     while (!info_done)
     {
          if (!wait_event())
          {
               if (_exit_status != 0)
                    info_ok = false;
               info_done = true;
               continue;
          }

          while (process_event()) {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
                    {
                         _error = errShm;
                         info_ok = false;
                         info_done = true;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
                    }
                    break;
               default:
                    cerr << "Got Unknown message " << (unsigned long) _status_type << endl;
                    break;
               }
          }
     }
     char buffer[4096];
     while (output.getline(buffer, 4096))
     {
          if (strncmp(buffer, "pub", 3) != 0 && strncmp(buffer, "uid", 3) != 0)
               continue;

          char **fields = g_strsplit(buffer, ":", 0);
	  if (strncmp(buffer, "pub", 3) == 0)
	  {
	       info._keyid = fields[4];
	       info._userid = fields[9];
	       parse_trust_letter(*fields[1], info);
	  }
	  else
	  {
	       info._aliases.push_back(fields[9]);
	  }
          g_strfreev(fields);
     }
     return info_ok;
}

bool GPGInterface::key_info_from_id(KeyInfo& info, const string& keyid)
{
     list<string> args;
#ifdef HAVE_STD_SSTREAM
     stringstream output;
#else
     strstream output;
#endif

     _error = errNone;
     _input = NULL;
     _output = &output;

     args.push_back("--with-colons");
     args.push_back("--list-keys");
     args.push_back(keyid);

     if (!execute(args, false))
          return false;

     bool info_done = false, info_ok = true;

     info.reset();
     while (!info_done)
     {
          if (!wait_event())
          {
               if (_exit_status != 0)
                    info_ok = false;
               info_done = true;
               continue;
          }

          while (process_event()) {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
                    {
                         _error = errShm;
                         info_ok = false;
                         info_done = true;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
                    }
                    break;
               default:
                    cerr << "Got Unknown message " << (unsigned long) _status_type << endl;
                    break;
               }
          }
     }
     char buffer[4096];
     while (output.getline(buffer, 4096))
     {
          if (strncmp(buffer, "pub", 3) != 0 && strncmp(buffer, "uid", 3) != 0)
               continue;

          char **fields = g_strsplit(buffer, ":", 0);
	  if (strncmp(buffer, "pub", 3) == 0)
	  {
               info._keyid = fields[4];
               info._userid = fields[9];
	       parse_trust_letter(*fields[1], info);
	  }
	  else
	  {
	       info._aliases.push_back(fields[9]);
	  }
          g_strfreev(fields);
     }
     return info_ok;
}

bool GPGInterface::export_key(ostream* dest)
{
     return export_key(dest, _local_key);
}

bool GPGInterface::export_key(ostream* dest, const string& keyid)
{
     list<string> args;

     _error = errNone;
     _input = NULL;
     _output = dest;

     args.push_back("--export");
     args.push_back(keyid);

     if (!execute(args))
          return false;

     bool export_done = false, export_ok = true;

     while (!export_done)
     {
          if (!wait_event())
          {
               if (_exit_status != 0)
                     export_ok = false;
               export_done = true;
               continue;
          }

          while (process_event()) {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
                    {
                         _error = errShm;
                         export_ok = false;
                         export_done = true;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
                    }
                    break;
	       default:
		    cerr << "Got Unknown message " << (unsigned long) _status_type << endl;
		    break;
	       }
          }
     }
     return export_ok;
}

bool GPGInterface::get_keys(list<KeyInfo>& keys)
{
     list<string> args;
#ifdef HAVE_STD_SSTREAM
     stringstream output;
#else
     strstream output;
#endif

     _error = errNone;
     _input = NULL;
     _output = &output;

     args.push_back("--with-colons");
     args.push_back("--list-keys");

     if (!execute(args, false))
          return false;

     bool list_done = false, list_ok = true;

     while (!list_done)
     {
          if (!wait_event())
          {
               if (_exit_status != 0)
                     list_ok = false;
               list_done = true;
               continue;
          }

          while (process_event()) {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
                    {
                         _error = errShm;
                         list_ok = false;
                         list_done = true;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
                    }
                    break;
	       default:
		    cerr << "Got Unknown message " << (unsigned long) _status_type << endl;
		    break;
               }
          }
     }
     // GPG Has finished, now we have to parse the output to get the keys
     char buffer[4096];
     while (output.getline(buffer, 4096))
     {
          if (strncmp(buffer, "pub", 3) != 0 && strncmp(buffer, "uid", 3) != 0)
               continue;

          char **fields = g_strsplit(buffer, ":", 0);
	  KeyInfo info;
	  if (strncmp(buffer, "pub", 3) == 0)
	  {
	       info._keyid = fields[4];
	       info._userid = fields[9];
	       parse_trust_letter(*fields[1], info);
	  }
	  else
	  {
	       info._aliases.push_back(fields[9]);
	  }
          keys.push_back(info);
          g_strfreev(fields);
     }
     return list_ok;
}

bool GPGInterface::get_secret_keys(list<KeyInfo>& keys)
{
     list<string> args;
#ifdef HAVE_STD_SSTREAM
     stringstream output;
#else
     strstream output;
#endif

     _error = errNone;
     _input = NULL;
     _output = &output;

     args.push_back("--with-colons");
     args.push_back("--list-secret-keys");

     if (!execute(args, false))
          return false;

     bool list_done = false, list_ok = true;

     while (!list_done)
     {
          if (!wait_event())
          {
               if (_exit_status != 0) {
                     cerr << "exit status = " << _exit_status << endl;
                     list_ok = false;
               }
               list_done = true;
               continue;
          }

          while (process_event()) {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
                    {
                         _error = errShm;
                         list_ok = false;
                         list_done = true;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
                    }
                    break;
	       default:
		    cerr << "Got Unknown message " << (unsigned long) _status_type << endl;
		    break;
               }
          }
     }
     // GPG Has finished, now we have to parse the output to get the keys
     char buffer[4096];
     while (output.getline(buffer, 4096))
     {
          if (strncmp(buffer, "sec", 3) != 0)
               continue;

          char **fields = g_strsplit(buffer, ":", 0);
          string keyid = fields[4];
          string userid = fields[9];
          keys.push_back(GPGInterface::KeyInfo(keyid, userid));
          g_strfreev(fields);
     }
     return list_ok;
}

void GPGInterface::set_local_key(const string& key, const string& passphrase)
{
     _local_key = key;
     _passphrase = passphrase;
}

void GPGInterface::find_gpg()
{
     char* path, *spath;

     if (!GPGInterface::_gpg_path.empty() && g_file_exists(GPGInterface::_gpg_path.c_str()))
	  return;

     path = getenv("PATH");
     // if we couldn't get the path try a couple of common places
     if (!path)
          path = "/bin:/usr/bin:/usr/local/bin";

     spath = g_strdup(path);
     for (char* tmp = strtok(spath, ":"); tmp; tmp = strtok(NULL, ":"))
     {
          char* gpgpath = g_concat_dir_and_file(tmp, "/gpg");
          if (g_file_exists(gpgpath))
          {
	       GPGInterface::_gpg_path = gpgpath;
               g_free(gpgpath);
               g_free(spath);
               return;
          }
          g_free(gpgpath);
    }
    g_free(spath);
    _error = errFindGPG;
}

bool GPGInterface::execute(list<string> func_args, bool use_shm)
{
     list<string> args;
     int status_pipe[2], input_pipe[2], output_pipe[2], nullfd;

     args.push_back(GPGInterface::_gpg_path);
     args.push_back("--no-tty");
     args.push_back("--no-batch");
     args.push_back("--lock-multiple");

     if (pipe(status_pipe) == -1)
     {
          cerr << "Couldn't create status pipe" << endl;
	  _error = errPipe;
          return false;
     }
     char status_pipe_fd[64];
     g_snprintf(status_pipe_fd, 64, "%d", status_pipe[1]);
     args.push_back("--status-fd");
     args.push_back(status_pipe_fd);
     if (use_shm) {
         args.push_back("--run-as-shm-coprocess");
         args.push_back("0");
     }
     args.push_back("--utf8-strings");
     args.push_back("--keyserver");
     args.push_back(G_App->getCfg().gpg.keyserver);

     if (_armor)
          args.push_back("--armor");

     args.push_back("--output");
     args.push_back("-");

     args.splice(args.end(), func_args);

     if (_input)
          args.push_back("-");

     if (_debug)
     {
          for (list<string>::iterator it = args.begin(); it != args.end(); it++)
               cerr << *it << " ";
          cerr << endl;
     }

     if (_input && pipe(input_pipe) == -1)
     {
          close(status_pipe[0]);
          close(status_pipe[1]);
	  _error = errPipe;
          return false;
     }

     if (_output && pipe(output_pipe) == -1)
     {
          close(status_pipe[0]);
          close(status_pipe[1]);
          close(input_pipe[0]);
          close(input_pipe[1]);
	  _error = errPipe;
	  return false;
     }

     // Open /dev/null for gpg stderr
     nullfd = open("/dev/null", O_WRONLY);

     // build argv
     char** argv;
     argv = g_new(char *, args.size() + 1);
     int i = 0;
     for (list<string>::iterator it = args.begin(); it != args.end(); it++, i++)
          argv[i] = (char *) it->c_str();
     argv[i] = NULL;

     switch ((_gpg_pid = fork()))
     {
     case -1:
	  cerr << "Ack!! Couldn't fork gpg!" << endl;
	  _error = errFork;
          return false;
     case 0:
          // Child
	  close(status_pipe[0]);
	  dup2(nullfd, 2);

          if (_input)
          {
               close(input_pipe[1]);
	       dup2(input_pipe[0], 0);
               close(input_pipe[0]);
          }
          else
	       dup2(nullfd, 0);

          if (_output)
          {
               close(output_pipe[0]);
               dup2(output_pipe[1], 1);
               close(output_pipe[1]);
          }
	  else
	       dup2(nullfd, 1);

	  close(nullfd);

	  execv(argv[0], argv);
	  // Failed!!
	  exit(1);
     }
     // Parent
     close(status_pipe[1]);
     close(nullfd);
     _status_fd = status_pipe[0];

     if (_input)
     {
          close(input_pipe[0]);
          _gpg_input_fd = input_pipe[1];
     }
     else
          _gpg_input_fd = -1;

     if (_output)
     {
          close(output_pipe[1]);
          _gpg_output_fd = output_pipe[0];
     }
     else
          _gpg_output_fd = -1;

     g_free(argv);

     _running = true;

     return true;
}

bool GPGInterface::wait_event()
{
     fd_set readset, writeset;
     int max_fd = 0;

     while (1)
     {
          max_fd = -1;

          FD_ZERO(&readset);
          FD_ZERO(&writeset);

          if (_input && _gpg_input_fd != -1)
          {
               FD_SET(_gpg_input_fd, &writeset);
               max_fd = _gpg_input_fd > max_fd ? _gpg_input_fd : max_fd;
          }
          if (_status_fd != -1)
          {
               FD_SET(_status_fd, &readset);
               max_fd = _status_fd > max_fd ? _status_fd : max_fd;
          }
          if (_output && _gpg_output_fd != -1)
          {
               FD_SET(_gpg_output_fd, &readset);
               max_fd = _gpg_output_fd > max_fd ? _gpg_output_fd : max_fd;
          }

          // If all of the fds are -1 then don't call select
          if (max_fd == -1)
               break;

          if (select(max_fd + 1, &readset, &writeset, NULL, NULL) == -1)
          {
               if ((errno != EAGAIN) && (errno != EINTR))
                    return false;
          }

          if (_input && _gpg_input_fd != -1 && FD_ISSET(_gpg_input_fd, &writeset))
          {
               char buffer[1024];
               int readlen;
               _input->read(buffer, 1024);
               readlen = _input->gcount();

               int writelen = write(_gpg_input_fd, buffer, readlen);

               if (writelen <= 0)
               {
                    close(_gpg_input_fd);
                    _gpg_input_fd = -1;
               }
               else
               {
                    for (int i = 0; i < readlen - writelen; i++)
                         _input->putback(buffer[writelen + i]);
               }

               if (!_input->gcount())
               {
                    if (_debug) cerr << "finished writing data to gpg" << endl;
                    close(_gpg_input_fd);
                    _gpg_input_fd = -1;
               }
          }
          if (_output && _gpg_output_fd != -1 && FD_ISSET(_gpg_output_fd, &readset))
          {
               char buffer[1024];
               int readlen;

               readlen = read(_gpg_output_fd, buffer, 1024);
               if (readlen <= 0)
               {
                    if (_debug) cerr << "finished reading from gpg" << endl;
                    close(_gpg_output_fd);
                    _gpg_output_fd = -1;
		    *_output << ends;
               }
               buffer[readlen] = '\0';
	       if (_debug) cerr << "Read line from gpg \"" << buffer << "\"" << endl;
               *_output << buffer;
          }
          if (_status_fd != -1 && FD_ISSET(_status_fd, &readset))
          {
               // Leave space for \0 at the end of the string
               int len = read(_status_fd, _status_buffer + _status_offset, 1023 - _status_offset);

               if (len <= 0)
               {
                    if (_debug) cerr << "finished reading gpg status" << endl;
                    close(_status_fd);
                    _status_fd = -1;
               }
               else
               {
                    _status_offset += len;
                    _status_buffer[_status_offset] = '\0';
               }
               break;
          }
     }
     if (_status_fd == -1 && _gpg_output_fd == -1 && _gpg_input_fd == -1)
     {
          int child_status;
          if (_debug) cerr << "all IO done, waiting for gpg to exit" << endl;
	  cleanup();
          return false;
     }
     return true;
}

bool GPGInterface::process_event()
{
     char* nl = strchr(_status_buffer, '\n');
     bool got_message = false;

     if (!nl)
          return false;
     *nl++ = '\0';
     if (_debug) cerr << "Got status line: \"" << _status_buffer << "\"" << endl;

     if (strncmp(_status_buffer, "[GNUPG:] ", 9) != 0)
     {
          cerr << "Got garbage for status from gpg" << endl;
	  _error = errStatusProto;
          return false;
     }
     char *arg = strchr(_status_buffer + 9, ' ');
     if (arg)
     {
          *arg++ = '\0';
          strncpy(_status_arg, arg, 1024);
     }

     char *cmd = _status_buffer + 9;
     // FILL IN BELOW
     if (strcmp(cmd, "SHM_INFO") == 0)
     {
          _status_type = gpgShmInfo;
          got_message = true;
     }
     else if (strcmp(cmd, "SHM_GET") == 0)
     {
          _status_type = gpgShmGet;
          got_message = true;
     }
     else if (strcmp(cmd, "SHM_GET_BOOL") == 0)
     {
          _status_type = gpgShmGetBool;
          got_message = true;
     }
     else if (strcmp(cmd, "SHM_GET_HIDDEN") == 0)
     {
          _status_type = gpgShmGetHidden;
          got_message = true;
     }
     else if (strcmp(cmd, "NEED_PASSPHRASE") == 0)
     {
          _status_type = gpgNeedPass;
          got_message = true;
     }
     else if (strcmp(cmd, "GOOD_PASSPHRASE") == 0)
     {
          _status_type = gpgGoodPass;
	  got_message = true;
     }
     else if (strcmp(cmd, "BAD_PASSPHRASE") == 0)
     {
	  _status_type = gpgBadPass;
	  got_message = true;
     }
     else if (strcmp(cmd, "MISSING_PASSPHRASE") == 0)
     {
	  _status_type = gpgMissingPass;
	  got_message = true;
     }
     else if (strcmp(cmd, "NEED_PASSPHRASE_SYM") == 0)
     {
	  _status_type = gpgNeedPassSym;
	  got_message = true;
     }
     else if (strcmp(cmd, "SIG_ID") == 0)
     {
          _status_type = gpgSigID;
	  got_message = true;
     }
     else if (strcmp(cmd, "GOODSIG") == 0)
     {
	  _status_type = gpgGoodSig;
	  got_message = true;
     }
     else if (strcmp(cmd, "BADSIG") == 0)
     {
	  _status_type = gpgBadSig;
          got_message = true;
     }
     else if (strcmp(cmd, "ERRSIG") == 0)
     {
	  _status_type = gpgErrSig;
	  got_message = true;
     }
     else if (strcmp(cmd, "VALIDSIG") == 0)
     {
	  _status_type = gpgValidSig;
	  got_message = true;
     }
     else if (strcmp(cmd, "SIGEXPIRED") == 0)
     {
          _status_type = gpgSigExpired;
	  got_message = true;
     }
     else if (strcmp(cmd, "SIG_CREATED") == 0)
     {
	  _status_type = gpgSigCreated;
	  got_message = true;
     }
     else if (strcmp(cmd, "ENC_TO") == 0)
     {
          _status_type = gpgEncTo;
	  got_message = true;
     }
     else if (strcmp(cmd, "BEGIN_DECRYPTION") == 0)
     {
	  _status_type = gpgBeginDecryption;
          got_message = true;
     }
     else if (strcmp(cmd, "DECRYPTION_OKAY") == 0)
     {
	  _status_type = gpgDecryptionOkay;
	  got_message = true;
     }
     else if (strcmp(cmd, "END_DECRYPTION") == 0)
     {
	  _status_type = gpgEndDecryption;
	  got_message = true;
     }
     else if (strcmp(cmd, "DECRYPTION_FAILED") == 0)
     {
	  _status_type = gpgDecryptionFailed;
	  got_message = true;
     }
     else if (strcmp(cmd, "BEGIN_ENCRYPTION") == 0)
     {
	  _status_type = gpgBeginEncryption;
	  got_message = true;
     }
     else if (strcmp(cmd, "END_ENCRYPTION") == 0)
     {
	  _status_type = gpgEndEncryption;
	  got_message = true;
     }
     else if (strcmp(cmd, "GOODMDC") == 0)
     {
	  _status_type = gpgGoodMdc;
	  got_message = true;
     }
     else if (strcmp(cmd, "BADMDC") == 0)
     {
	  _status_type = gpgBadMdc;
	  got_message = true;
     }
     else if (strcmp(cmd, "ERRMDC") == 0)
     {
	  _status_type = gpgErrMdc;
	  got_message = true;
     }
     else if (strcmp(cmd, "BADARMOR") == 0)
     {
	  _status_type = gpgBadArmor;
	  got_message = true;
     }
     else if (strcmp(cmd, "TRUST_ULTIMATE") == 0)
     {
	  _status_type = gpgTrustUltimate;
	  got_message = true;
     }
     else if (strcmp(cmd, "TRUST_FULLY") == 0)
     {
          _status_type = gpgTrustFull;
          got_message = true;
     }
     else if (strcmp(cmd, "TRUST_MARGINAL") == 0)
     {
          _status_type = gpgTrustMarginal;
          got_message = true;
     }
     else if (strcmp(cmd, "TRUST_NEVER") == 0)
     {
          _status_type = gpgTrustNever;
          got_message = true;
     }
     else if (strcmp(cmd, "TRUST_UNDEFINED") == 0)
     {
          _status_type = gpgTrustUndefined;
          got_message = true;
     }
     else if (strcmp(cmd, "KEYREVOKED") == 0)
     {
	  _status_type = gpgKeyRevoked;
	  got_message = true;
     }
     else if (strcmp(cmd, "SESSION_KEY") == 0)
     {
	  _status_type = gpgSessionKey;
	  got_message = true;
     }
     else if (strcmp(cmd, "NO_SECKEY") == 0)
     {
	  _status_type = gpgNoSecKey;
	  got_message = true;
     }
     else if (strcmp(cmd, "NO_PUBKEY") == 0)
     {
	  _status_type = gpgNoPubKey;
	  got_message = true;
     }
     else if (strcmp(cmd, "IMPORTED") == 0)
     {
	  _status_type = gpgImported;
	  got_message = true;
     }
     else if (strcmp(cmd, "IMPORT_RES") == 0)
     {
	  _status_type = gpgImportRes;
	  got_message = true;
     }
     else if (strcmp(cmd, "RSA_OR_IDEA") == 0)
     {
	  _status_type = gpgRsaIdea;
	  got_message = true;
     }
     else if (strcmp(cmd, "NODATA") == 0)
     {
	  _status_type = gpgNoData;
	  got_message = true;
     }
     else if (strcmp(cmd, "USERID_HINT") == 0)
     {
	  _status_type = gpgUserIDHint;
	  got_message = true;
     }
     else
     {
	  cerr << "FIXME: Got unknown status code " << cmd << endl;
     }

     char *ch;
     for (ch = _status_buffer; *nl; nl++, ch++)
          *ch = *nl;
     *ch = '\0';
     _status_offset = strlen(_status_buffer);

     return got_message;
}

bool GPGInterface::shm_init()
{
     int proto, pid, shmid, size, locked;
     if (sscanf(_status_arg, "pv=%d pid=%d shmid=%d sz=%d lz=%d", &proto, &pid, &shmid, &size, &locked) != 5)
     {
          cerr << "Error parsing gpg shared memory information: \"" << _status_arg << "\"" << endl;
          return false;
     }
     if (proto != 1 || pid != _gpg_pid || size == 0)
     {
          cerr << "Invalid shm info" << endl;
          return false;
     }
     if ((_shm_buffer = (char *) shmat(shmid, NULL, 0)) == (char *) -1)
     {
         cerr << "Couldn't attach to gpg shared memory" << endl;
         return false;
     }
     return true;
}

bool GPGInterface::shm_write_str(const char* str)
{
     int offset, len;

     if (_shm_buffer[2] != 1 || _shm_buffer[3] != 0)
          return false;

     offset = (_shm_buffer[0] << 8) | _shm_buffer[1];
     len = strlen(str);

     _shm_buffer[offset]     = len >> 8;
     _shm_buffer[offset + 1] = len & 0xff;

     // Copy the data to shm
     memcpy(_shm_buffer + offset + 2, str, len);

     // set reply trigger
     _shm_buffer[3] = 1;

     // wakeup GPG
     kill(_gpg_pid, SIGUSR1);

     return true;
}

bool GPGInterface::shm_write_bool(bool value)
{
     int offset, len;

     if (_shm_buffer[2] != 1 || _shm_buffer[3] != 0)
          return false;

     offset = (_shm_buffer[0] << 8) | _shm_buffer[1];
     _shm_buffer[offset + 0] = 0;
     _shm_buffer[offset + 1] = 1;

     /* yesno flag */
     _shm_buffer[offset + 2] = value ? 1 : 0;

     /* set reply trigger flag */
     _shm_buffer[3] = 1;

     /* wakeup GnuPG process */
     kill(_gpg_pid, SIGUSR1);
     return true;
}

void GPGInterface::set_armor(bool armor)
{
     _armor = armor;
}

GPGInterface::SigInfo::SigInfo()
     : _valid(false), _timestamp(0)
{
}
const string& GPGInterface::SigInfo::get_fingerprint() const
{
     return _fingerprint;
}

const GPGInterface::KeyInfo& GPGInterface::SigInfo::get_key() const
{
     return _key;
}

unsigned long GPGInterface::SigInfo::get_timestamp() const
{
     return _timestamp;
}

void GPGInterface::SigInfo::reset()
{
     _timestamp = 0;
     _valid = false;
     _fingerprint = "";
     _key.reset();
}

GPGInterface::KeyInfo::KeyInfo()
     : _expired(false)
{
}

GPGInterface::KeyInfo::KeyInfo(string& keyid, string& userid)
     : _userid(userid), _keyid(keyid), _expired(false)
{
}

const string& GPGInterface::KeyInfo::get_keyid() const
{
     return _keyid;
}

const string& GPGInterface::KeyInfo::get_userid() const
{
     return _userid;
}

const list<string>& GPGInterface::KeyInfo::get_aliases() const
{
     return _aliases;
}

bool GPGInterface::KeyInfo::expired() const
{
     return _expired;
}

void GPGInterface::KeyInfo::reset()
{
     _expired = false;
     _userid = "";
     _keyid = "";
}

GPGInterface::DecryptInfo::DecryptInfo()
     : _valid(false), _has_sig(false)
{
}

void GPGInterface::DecryptInfo::reset()
{
     _valid = false;
     _has_sig = false;
     _sig.reset();
}

bool GPGInterface::send_key(const string& keyid)
{
     list<string> args;

     _error = errNone;
     _input = NULL;
     _output = NULL;

     args.push_back("--send-key");
     args.push_back(keyid);

     if (!execute(args))
	  return false;

     bool send_done = false, send_ok = true;

     while (!send_done)
     {
	  if (!wait_event())
          {
               if (_exit_status != 0)
                    send_ok = false;
               send_done = true;
               continue;
          }

          while (process_event() && !send_done)
	  {
               switch (_status_type)
               {
               case gpgShmInfo:
                    if (!shm_init())
                    {
                         _error = errShm;
                         send_ok = false;
                         send_done = true;
                         cerr << "Couldn't initialize shared memory: " << g_strerror(errno) << endl;
                    }
                    break;
	       default:
		    break;
	       }
	  }
     }
     return send_ok;
}

void GPGInterface::cleanup()
{
     // Make sure the process is cleaned up properly
     if (_status_fd != -1)
     {
          close(_status_fd);
	  _status_fd = -1;
     }
     if (_gpg_input_fd != -1)
     {
          close(_gpg_input_fd);
	  _gpg_input_fd = -1;
     }
     if (_gpg_output_fd != -1)
     {
          close(_gpg_output_fd);
	  _gpg_output_fd = -1;
     }

     if (_running && _gpg_pid)
     {
          int child_status = 0;
	  if (waitpid(_gpg_pid, &child_status, WNOHANG) <= 0)
	  {
               cerr << "Killing PID " << _gpg_pid << endl;
               kill(_gpg_pid, SIGTERM);
               waitpid(_gpg_pid, &child_status, 0);
	  }

          _status_type = gpgExit;
          _exit_status = WEXITSTATUS(child_status);

	  // Detach from the shared memory
	  shmdt(_shm_buffer);

	  _running = false;
	  _gpg_pid = 0;
     }
}

void GPGInterface::parse_trust_letter(char letter, KeyInfo& key)
{
     switch (letter)
     {
     case '-':
	  key._trust = GPGInterface::trustUndefined;
	  break;
     case 'e':
	  key._expired = true;
	  break;
     case 'q':
	  key._trust = GPGInterface::trustUndefined;
	  break;
     case 'n':
	  key._trust = GPGInterface::trustNever;
	  break;
     case 'm':
	  key._trust = GPGInterface::trustMarginal;
	  break;
     case 'f':
	  key._trust = GPGInterface::trustFull;
	  break;
     case 'u':
	  key._trust = GPGInterface::trustUltimate;
	  break;
     }
}
