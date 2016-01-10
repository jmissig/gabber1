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


#ifndef	INCL_GPG_INTERFACE
#define	INCL_GPG_INTERFACE

#include "GabberConfig.hh"

#include <unistd.h>

#include <sigc++/signal_system.h>

#include <iostream>
#include <fstream>
#include <string>
#ifdef HAVE_STD_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif
#include <list>
#include <map>
using namespace std;

class GPGInterface
     : SigC::Object
{
public:
     GPGInterface();
     GPGInterface(const string& local_key, const string& passphrase, bool armor = false);
     ~GPGInterface();

     enum SignType {
          sigNormal, sigClear, sigDetach
     };
     enum Status {
          gpgError, gpgExit, gpgNoData,
	  gpgShmInfo, gpgShmGet, gpgShmGetBool, gpgShmGetHidden,
	  gpgNeedPass, gpgNeedPassSym, gpgGoodPass, gpgBadPass, gpgMissingPass,
	  gpgSigID, gpgValidSig, gpgSigExpired, gpgSigCreated, gpgGoodSig, gpgBadSig, gpgErrSig,
          gpgEncTo, gpgBeginDecryption, gpgDecryptionOkay, gpgEndDecryption, gpgDecryptionFailed,
	  gpgBeginEncryption, gpgEndEncryption,
	  gpgGoodMdc, gpgBadMdc, gpgErrMdc, gpgBadArmor,
	  gpgTrustUltimate, gpgTrustMarginal, gpgTrustFull, gpgTrustNever, gpgTrustUndefined,
	  gpgKeyRevoked, gpgSessionKey, gpgNoSecKey, gpgNoPubKey, 
	  gpgImported, gpgImportRes, gpgRsaIdea,
	  gpgUserIDHint
     };
     enum Error {
          errNone, errShm, errPass, errSig, errEnc, errPipe, errFork, errStatusProto, errFindGPG,
	  errPubKey, errSecKey, errOK, errNoData
     };
     enum Trust {
	  trustUltimate, trustFull, trustMarginal, trustNever, trustUndefined
     };

     class KeyInfo
     {
     public:
          KeyInfo();
          KeyInfo(string& keyid, string& userid);

          const string& get_keyid()  const;
          const string& get_userid() const;
	  Trust get_trust() const { return _trust; }
	  const list<string>& get_aliases() const;
          bool  expired() const;

	  void reset();
     private:
          friend class GPGInterface;
          string _userid;
          string _keyid;
	  list<string> _aliases;
          bool   _expired;
	  Trust  _trust;
     };

     class SigInfo
     {
     public:
          SigInfo();

          bool valid() const { return _valid; }
          const string&  get_fingerprint() const;
          const KeyInfo& get_key() const;
          unsigned long  get_timestamp() const;

	  void reset();
     private:
          friend class GPGInterface;
          string  _fingerprint;
          KeyInfo _key;
          bool    _valid;
          unsigned long _timestamp;
     };

     class DecryptInfo
     {
     public:
          DecryptInfo();

	  bool valid()   const { return _valid; }
	  bool has_sig() const { return _has_sig; }
	  const SigInfo& get_sig() const { return _sig; }

	  void reset();
     private:
	  friend class GPGInterface;
	  bool _valid;
	  bool _has_sig;

	  SigInfo _sig;
     };

     bool sign(SignType type, istream* source, ostream* dest);
     bool encrypt(const string& recipient, istream* source, ostream* dest, bool sign = false);
     bool decrypt(DecryptInfo& decrypt, istream* source, ostream* dest);
     bool verify(SigInfo& sig, istream* source, ostream* dest);
     bool verify_detached(SigInfo& sig, const string& filename, istream* source);
     bool import_key(KeyInfo& info, istream* key);
     bool key_info_from_key(KeyInfo& info, istream* key);
     bool key_info_from_id(KeyInfo& info, const string& keyid);
     // If a key isn't provided then _local_key is used
     bool export_key(ostream* key);
     bool export_key(ostream* key, const string& keyid);
     bool get_keys(list<KeyInfo>& keys);
     bool get_secret_keys(list<KeyInfo>& keys);
     // Send a key to the keyserver
     bool send_key(const string& keyid);

     // Key to encrypt/sign stuff with
     void set_local_key(const string& key, const string& passphrase);
     void set_armor(bool armor);
     void set_debug(bool debug) { _debug = debug; }

     // Make sure the everything is properly cleanup up from a run of gpg
     void cleanup();

     Error getError() { return _error; }

     // Signal to get passphrase. Handler should return true to continue or false to abort
     SigC::Signal1<bool, string&> gpgNeedPassphrase;

private:
     bool execute(list<string>args, bool use_shm = true);
     bool wait_event();
     bool process_event();

     void find_gpg();

     bool shm_init();
     bool shm_write_str(const char* str);
     bool shm_write_bool(bool value);

     void parse_trust_letter(char letter, KeyInfo& key);
protected:
     string    _local_key;
     string    _passphrase;

private:
     bool      _running;
     pid_t     _gpg_pid;
     int       _exit_status;
     istream*  _input;
     ostream*  _output;
     bool      _armor;

     bool	_debug;

     int       _status_fd;
     int       _gpg_input_fd;
     int       _gpg_output_fd;
     char*     _shm_buffer;
     char      _status_buffer[1024];
     int       _status_offset;
     char      _status_arg[1024];
     Status    _status_type;
     Error     _error;

     static string    _gpg_path;
};

inline ostream& operator<<(ostream& out, const GPGInterface::KeyInfo& key)
{
     out << "-----KEY INFO-----" << endl;
     out << "KeyID: " << key.get_keyid() << " UserID: " << key.get_userid() <<
          " Expired: " << (key.expired() ? "yes" : "no") << "Trust: ";
     switch (key.get_trust())
     {
     case GPGInterface::trustUndefined:
          out << "Undefined";
          break;
     case GPGInterface::trustNever:
          out << "Never";
          break;
     case GPGInterface::trustMarginal:
          out << "Marginal";
          break;
     case GPGInterface::trustFull:
          out << "Full";
          break;
     case GPGInterface::trustUltimate:
          out << "Ultimate";
          break;
     }
     out << endl;

     return out;
}

inline ostream& operator<<(ostream& out, const GPGInterface::SigInfo& sig)
{
     out << "-----SIGNATURE INFO-----" << endl;
     out << "Valid: " << (sig.valid() ? "yes" : "no") << " Fingerprint: " << sig.get_fingerprint() << endl;
     out << "Timestamp: " << sig.get_timestamp() << endl;
     out << sig.get_key();
     return out;
}

inline ostream& operator<<(ostream& out, const GPGInterface::DecryptInfo& decrypt)
{
     out << "-----DECRYPT INFO-----" << endl;
     out << "Valid: " << (decrypt.valid() ? "yes" : "no") << " Has Signature: ";
     out << (decrypt.has_sig() ? "yes" : "no") << endl;
     if (decrypt.has_sig())
          out << decrypt.get_sig();

     return out;
}

#endif
