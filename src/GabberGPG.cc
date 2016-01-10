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
 * GabberGPG
 * Author Brandon Lees <brandon@sci.brooklyn.cuny.edu>
 */

#include "GabberConfig.hh"

#include "GabberGPG.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "GPGInterface.hh"

#include <signal.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-window-icon.h>

using namespace GabberUtil;

GabberGPG::GabberGPG()
     : _enabled(false), _gotpass(false)
{
     _enabled = G_App->getCfg().gpg.enabled;

     // add a handler for Presences so we can verify the signatures and get the keyid
     // to add to the KeyMap
     G_App->getSession().evtPresence.connect(slot(this, &GabberGPG::verify_presence));

     // add SIGPIPE handler
     struct sigaction act;
     act.sa_handler = GabberGPG::sigpipe_handler;
     sigemptyset(&act.sa_mask);
     act.sa_flags = 0;

     if (sigaction(SIGPIPE, &act, NULL) < 0)
          cerr << "Couldn't add sigpipe handler" << endl;

}

GabberGPG::~GabberGPG()
{
}

GPGInterface::Error GabberGPG::sign(GPGInterface::SignType type, const string& source, string& dest)
{
     // stringstreams are so much better but earlier versions
     // of libstdc++ don't have them
#ifdef HAVE_STD_SSTREAM
     istringstream istr(source.c_str());
     stringstream ostr;
#else
     istrstream istr(source.c_str());
     strstream ostr;
#endif

     // Initialize the GPGInterface
     GPGInterface signer(G_App->getCfg().gpg.secretkeyid, _passphrase, _armor);
     signer.gpgNeedPassphrase.connect(slot(this, &GabberGPG::get_passphrase));

     cerr << "Signing \"" << source << "\"" << endl;
     if (!signer.sign(type, &istr, &ostr))
          return signer.getError();

     // strip the pgp header/trailer and assign the result to the dest string
     dest = strip_gpg_header(&ostr);

     return GPGInterface::errOK;
}

GPGInterface::Error GabberGPG::encrypt(const string& recipient, const string& source, string& dest, bool sign)
{
#ifdef HAVE_STD_SSTREAM
     istringstream istr(source.c_str());
     stringstream ostr;
#else
     istrstream istr(source.c_str());
     strstream ostr;
#endif

     // Initialise the GPGInterface to encrypt
     GPGInterface encrypter(G_App->getCfg().gpg.secretkeyid, _passphrase, _armor);
     encrypter.gpgNeedPassphrase.connect(slot(this, &GabberGPG::get_passphrase));

     // Do the encryption, return the error on failure
     if (!encrypter.encrypt(recipient, &istr, &ostr, sign))
          return encrypter.getError();

     // strip the pgp header/trailer
     dest = strip_gpg_header(&ostr);
     return GPGInterface::errOK;
}

GPGInterface::Error GabberGPG::decrypt(GPGInterface::DecryptInfo& decrypt, const string& source, string& dest)
{
#ifdef HAVE_STD_SSTREAM
     stringstream istr;
     ostringstream ostr;
#else
     strstream istr;
     ostrstream ostr;
#endif

     GPGInterface decrypter(G_App->getCfg().gpg.secretkeyid, _passphrase, _armor);
     decrypter.gpgNeedPassphrase.connect(slot(this, &GabberGPG::get_passphrase));

     // Jabber doesn't use the pgp header/footer but gpg needs it to decrypt so add if to the message
     istr << "-----BEGIN PGP MESSAGE-----" << endl;
     istr << "Version: GnuPG" << endl;
     istr << endl;
     istr << source;
     istr << "-----END PGP MESSAGE-----" << endl;

     if (!decrypter.decrypt(decrypt, &istr, &ostr))
          return decrypter.getError();

     dest = ostr.str();
     return GPGInterface::errOK;
}

GPGInterface::Error GabberGPG::verify(GPGInterface::SigInfo& sig, const string& signature, const string& message)
{
#ifdef HAVE_STD_SSTREAM
     stringstream istr;
#else
     strstream istr;
#endif
     char filename[L_tmpnam];
     
     if (tmpnam(filename) == NULL)
     {
	  cerr << "Couldn't open temporary file " << filename << endl;
	  return GPGInterface::errPipe;
     }
     ofstream of(filename);

     // We use detached signatures to sign things in Jabber but the only way to verify them is to
     // have the data in a file, so write the message out to a temporary file while we verify it
     of << message;
     of.close();

     // add the pgp header/footer
     istr << "-----BEGIN PGP MESSAGE-----\nVersion: GnuPG\n\n";
     istr << signature << endl;
     istr << "-----END PGP MESSAGE-----\n";

     GPGInterface verifyer(G_App->getCfg().gpg.secretkeyid, _passphrase, _armor);

     // Verify the signature
     if (!verifyer.verify_detached(sig, filename, &istr))
     {
	  if (unlink(filename) < 0) cerr << "Error unlinking " << filename << endl;
          return verifyer.getError();
     }

     // get rid of the temporary file
     if (unlink(filename) < 0) cerr << "Error unlinking " << filename << endl;
     return GPGInterface::errOK;
}

// Import a Key from a string
GPGInterface::Error GabberGPG::import_key(GPGInterface::KeyInfo& info, const string& key)
{
#ifdef HAVE_STD_SSTREAM
     istringstream istr(key.c_str());
#else
     istrstream istr(key.c_str());
#endif

     GPGInterface importer;

     if (!importer.import_key(info, &istr))
          return importer.getError();

     return GPGInterface::errOK;
}

GPGInterface::Error GabberGPG::export_key(string& dest)
{
     return export_key(dest, G_App->getCfg().gpg.secretkeyid);
}

// export the key with the given keyid.  If none is gived, the public key for the local keyid is exported to dest
GPGInterface::Error GabberGPG::export_key(string& dest, const string& keyid)
{
#ifdef HAVE_STD_SSTREAM
     ostringstream ostr;
#else
     ostrstream ostr;
#endif

     GPGInterface exporter;
     exporter.set_armor(_armor);

     if (!exporter.export_key(&ostr, keyid))
          return exporter.getError();

     dest = ostr.str();
     return GPGInterface::errOK;
}

// Get key information from the key given in the string key
GPGInterface::Error GabberGPG::key_info_from_key(GPGInterface::KeyInfo& info, const string& key)
{
#ifdef HAVE_STD_SSTREAM
     istringstream istr(key.c_str());
#else
     istrstream istr(key.c_str());
#endif

     GPGInterface infoer;

     if (!infoer.key_info_from_key(info, &istr))
          return infoer.getError();

     return GPGInterface::errOK;
}

// Get key info for a key already on the keyring with id keyid
GPGInterface::Error GabberGPG::key_info_from_id(GPGInterface::KeyInfo& info, const string& keyid)
{
     GPGInterface infoer;

     if (!infoer.key_info_from_id(info, keyid))
          return infoer.getError();

     return GPGInterface::errOK;
}

// Get a list of all public keys on the keyring
GPGInterface::Error GabberGPG::get_keys(list<GPGInterface::KeyInfo>& keys)
{
     GPGInterface exporter;

     if (!exporter.get_keys(keys))
          return exporter.getError();

     return GPGInterface::errOK;
}

// Get a list of all secret keys on the keyring
GPGInterface::Error GabberGPG::get_secret_keys(list<GPGInterface::KeyInfo>& keys)
{
     GPGInterface exporter;

     if (!exporter.get_secret_keys(keys))
	  return exporter.getError();

     return GPGInterface::errOK;
}

// KeyMap functions

// find the keyid for a JID
string& GabberGPG::find_jid_key(const string& jid)
{
      map<string, string>::iterator it = G_App->getCfg().gpg.keymap.find(jabberoo::JID::getUserHost(jid));
      if (it == G_App->getCfg().gpg.keymap.end())
	   throw GPG_InvalidJID();
      return it->second;
}

// Add the keyid to the Keymap for the given JID
void GabberGPG::add_jid_keyid(const string& jid, const string& keyid)
{
     // don't add empty key ids
     if (keyid.empty())
	  return;

     map<string, string>::iterator it = G_App->getCfg().gpg.keymap.find(jabberoo::JID::getUserHost(jid));
     // If an entry already exists for the JID, erase it so it can be replaced with the
     // new keyid
     if (it != G_App->getCfg().gpg.keymap.end())
	  G_App->getCfg().gpg.keymap.erase(it);

     G_App->getCfg().gpg.keymap.insert(make_pair(jabberoo::JID::getUserHost(jid), keyid));
}

// Import a key to the user's keyring then add the keyid to the KeyMap
void GabberGPG::add_jid_key(const string& jid, const string& key)
{
     GPGInterface::Error ok;
     GPGInterface::KeyInfo ki;
     if (import_key(ki, key) != GPGInterface::errOK)
     {
          // Possible cause of failed import is already having the key imported so just
          // get the info for the key
          key_info_from_key(ki, key);
     }
     add_jid_keyid(jid, ki.get_keyid());
}

// Strips the GPG armor header and footer off the message since they are not used in Jabber
string GabberGPG::strip_gpg_header(istream *ostr)
{
     char buffer[4096];
     bool have_message = false;
     string message;

     while (ostr->getline(buffer, 4096))
     {
	  if (have_message && strncmp("-----", buffer, 5) == 0)
	       // Strip the trailing \n since it isn't needed (none of them are but I'm not sure if
	       // other versions of pgp are as indifferent to \n in the armor as gpg
	       return message.substr(0, message.rfind("\n"));
	  else if (have_message)
	  {
	       message += buffer;
	       message += "\n";
	  }
	  // A blank line separates the header from the message
	  else if (*buffer == '\0')
	       have_message = true;
     }
     return "";
}

void GabberGPG::verify_presence(const jabberoo::Presence& p, const jabberoo::Presence::Type prev)
{
     if (!enabled())
          return;

     // Check for a signature in the presence
     Element* x = p.findX("jabber:x:signed");
     if (x && !p.getBaseElement().cmpAttrib("type", "error"))
     {
	  GPGInterface::SigInfo info;
	  string status = p.getStatus();
	  string sig = x->getCDATA();

	  cerr << "Got signed presence from " << p.getFrom() << endl;
	  if (verify(info, sig, status) != GPGInterface::errOK)
	  {
	       cerr << "Got invalid signature in presence from " << p.getFrom() << endl;
	       return;
	  }
	  // The presence had a valid signature so add the keyid to the Keymap
	  cerr << "Adding keyid for " << p.getFrom() << " to keymap with id " << info.get_key().get_keyid() << endl;
	  add_jid_keyid(p.getFrom(), info.get_key().get_keyid());
     }
}

// determines if GPG is enabled and usable
bool GabberGPG::enabled()
{
     if (!_enabled)
	  return false;

     if (!find_gpg())
	  return false;

     return true;
}

// tries to find the where gpg is located by searching through the PATH
bool GabberGPG::find_gpg()
{
     GPGInterface find;
     if (find.getError() == GPGInterface::errFindGPG)
          return false;

     return true;
}

GPGInterface::Error GabberGPG::send_key(const string& keyid)
{
     // Initialise the GPGInterface
     GPGInterface sender;

    // send the key, return the error on failure
    if (!sender.send_key(keyid))
         return sender.getError();

    return GPGInterface::errOK;
}

void GabberGPG::sigpipe_handler(int signo)
{
     cerr << "got SIGPIPE" << endl;
}

GPGInterface::Error GabberGPG::verify_clear(GPGInterface::SigInfo& sig, const string& signature, string& message)
{
#ifdef HAVE_STD_SSTREAM
     istringstream istr(signature.c_str());
     ostringstream ostr;
#else
     istrstream istr(signature.c_str());
     ostrstream ostr;
#endif

     GPGInterface verifyer(G_App->getCfg().gpg.secretkeyid, _passphrase, _armor);

     // Verify the signature
     if (!verifyer.verify(sig, &istr, &ostr))
          return verifyer.getError();

     message = ostr.str();
     return GPGInterface::errOK;
}

bool GabberGPG::get_passphrase(string& pass)
{
     GPGPassDialog* dlg = manage(new GPGPassDialog);

     bool ok = dlg->run(_gotpass, pass);
     // If user pressed ok, set gotpass so we know if we can display an error messgage if the
     // passphrase was incorrect
     if (ok)
     {
	  _gotpass = true;
	  _passphrase = pass;
     }

// Holy crap... why was this left in?
//     cerr << "gotpass = " << _gotpass << " passphrase = " << _passphrase << endl;
     return ok;
}

void GabberGPG::refresh_passphrase()
{
     _gotpass = false;
     _enabled = true;
}

void GabberGPG::enable()
{
     if (G_App->getCfg().gpg.enabled)
	  _enabled = true;
}

///////////////////////////
//// GPGInfo Dialog
/////////////////////////////

GPGInfoDialog::GPGInfoDialog(const string& keyid, bool valid)
     : BaseGabberDialog("GPGInfo_dlg")
{
     main_dialog(_thisDialog);
     // Pixmaps
     string pix_path = ConfigManager::get_PIXPATH();
     string window_icon = pix_path + "gnome-gpg.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();

     GabberGPG& gpg = G_App->getGPG();
     GPGInterface::KeyInfo info;
     bool error = false;

     cerr << "Looking up info for keyid " << keyid << endl;
     if (valid)
     {
	  getLabel("GPGInfo_Encrypted_lbl")->set_text(_("The signature was valid"));
	  getWidget<Gnome::Pixmap>("GPGInfo_Encrypted_pix")->load(string(ConfigManager::get_SHAREDIR()) + "gpg-signed.xpm");
     }
     else
     {
	  getWidget<Gnome::Pixmap>("GPGInfo_Encrypted_pix")->load(string(ConfigManager::get_SHAREDIR()) + "gpg-badsigned.xpm");
	  getLabel("GPGInfo_Encrypted_lbl")->set_text(_("The signature was not valid"));
     }

     if (keyid.empty() || gpg.key_info_from_id(info, keyid) != GPGInterface::errOK)
	  getLabel("GPGInfo_Encrypted_lbl")->set_text(_("Could not determine key information"));
     else
     {
	  getLabel("GPGInfo_Encrypted_lbl")->set_text(_("This message was signed")); 
          getLabel("GPGInfo_UserID_lbl")->set_text(info.get_userid());
	  getLabel("GPGInfo_KeyID_lbl")->set_text(info.get_keyid());
	  string aliases;
	  list<string>::const_iterator it = info.get_aliases().begin();
	  for ( ; it != info.get_aliases().end(); it++)
	       aliases += *it + "\n";
	  getLabel("GPGInfo_Aliases_lbl")->set_text(aliases);
     }

     getButton("GPGInfo_OK_btn")->clicked.connect(slot(this, &BaseGabberWindow::close));
     show();
}

GPGInfoDialog::~GPGInfoDialog()
{}

///////////////////////////
//// GPGPass Dialog
/////////////////////////////

GPGPassDialog::GPGPassDialog()
     : BaseGabberDialog("GPGPass_dlg", true)
{
     main_dialog(_thisDialog);
     // Pixmaps
     string pix_path = ConfigManager::get_PIXPATH();
#ifdef GABBER_WINICON
     string window_icon = pix_path + "gnome-gpg.xpm";
     gnome_window_icon_set_from_file(_thisWindow->gtkobj(),window_icon.c_str());
     gnome_window_icon_init();
#endif

     _frmError = getWidget<Gtk::Frame>("GPGPass_Error_frm");
     _entPass  = getEntry("GPGPass_Passphrase_txt");
     _entPass  ->activate.connect(slot(this, &GPGPassDialog::on_Passphrase_activate));
     _thisDialog->set_focus_child(*_entPass);
}

GPGPassDialog::~GPGPassDialog()
{
}

bool GPGPassDialog::run(bool error, string& pass)
{
     // If there was an error with the previous passphrase, display the error frame
     if (error)
	  _frmError->show();

     gint button = _thisDialog->run();
     // If the user didn't press OK return
     if (button != 1) {
	  close();
	  return false;
     }
     pass = _entPass->get_text();

     close();
     return true;
}

void GPGPassDialog::on_Passphrase_activate()
{
     getButton("GPGPass_OK_btn")->clicked();
}
