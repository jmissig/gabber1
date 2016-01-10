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
 * GabberGPG
 * Author Brandon Lees <brandon@sci.brooklyn.cuny.edu>
 */


#ifndef	INCL_GABBER_GPG
#define	INCL_GABBER_GPG

#include "jabberoo.hh"

#include "BaseGabberWindow.hh"
#include "GPGInterface.hh"

#include <unistd.h>

class GabberGPG
     : public SigC::Object
{
public:
     GabberGPG();
     ~GabberGPG();

     enum GPGInfo { gpgNone, gpgValidSigned, gpgInvalidSigned, gpgValidEncrypted, gpgInvalidEncrypted };

     class GPG_InvalidJID {};

     GPGInterface::Error sign(GPGInterface::SignType type, const string& message, string& output);
     GPGInterface::Error encrypt(const string& recipient, const string& source, string& dest, bool sign = false);
     GPGInterface::Error decrypt(GPGInterface::DecryptInfo& decrypt, const string& source, string& dest);
     // This verifies a detached signature
     GPGInterface::Error verify(GPGInterface::SigInfo& sig, const string& signature, const string& message);
     // This verifies a normal or clear signature
     GPGInterface::Error verify_clear(GPGInterface::SigInfo& sig, const string& signature, string& message);
     GPGInterface::Error import_key(GPGInterface::KeyInfo& info, const string& key);
     GPGInterface::Error key_info_from_key(GPGInterface::KeyInfo& info, const string& key);
     GPGInterface::Error key_info_from_id(GPGInterface::KeyInfo& info, const string& keyid);
     GPGInterface::Error get_keys(list<GPGInterface::KeyInfo>& keys);
     GPGInterface::Error get_secret_keys(list<GPGInterface::KeyInfo>& keys);
     GPGInterface::Error send_key(const string& keyid);

     // If a key isn't provided then _local_key is used
     GPGInterface::Error export_key(string& key);
     GPGInterface::Error export_key(string& key, const string& keyid);

     void set_armor(bool armor) { _armor = armor; }

     // Keymap functions
     string& find_jid_key(const string& jid);
     void add_jid_keyid(const string& jid, const string& keyid);
     void add_jid_key(const string& jid, const string& key);

     // Used to determine if GPG can be used
     // enabled means that we have it and can use it.  Found means it's accessible.  
     bool enabled();
     bool find_gpg();
     void refresh_passphrase();
     void disable() { _enabled = false; }
     void enable();

     // Signal handler for SIGPIPE
     static void sigpipe_handler(int signo);
private:
     void verify_presence(const jabberoo::Presence& p, const jabberoo::Presence::Type prev);
     string strip_gpg_header(istream *);
     bool get_passphrase(string& pass);

     bool _armor;
     // Is GPG support enabled?  If so, is it functional?
     bool _enabled;

     // Passphrase
     string _passphrase;
     // Have we trie to get the passphrase before?  If so then the previosu one was incorrect.  
     bool _gotpass;
};

///////////////////////////
// GPGInfo Dialog
///////////////////////////

class GPGInfoDialog
     : public BaseGabberDialog
{
public:
     GPGInfoDialog(const string& keyid, bool valid);
     ~GPGInfoDialog();
};

///////////////////////////
// GPGPass Dialog
///////////////////////////

class GPGPassDialog
     : public BaseGabberDialog
{
public:
     GPGPassDialog();
     ~GPGPassDialog();
     bool run(bool error, string& pass);
protected:
     void on_Passphrase_activate();
private:
     Gtk::Frame* _frmError;
     Gtk::Entry* _entPass;
};

#endif
