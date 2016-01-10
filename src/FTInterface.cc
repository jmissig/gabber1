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
 * FTInterface
 * Author Thomas "temas" Muldowney <temas@jabber.org>
 */

#include "ghttp.c"
#include "GabberConfig.hh" // for _()

#include "FTInterface.hh"

#include "GabberApp.hh"
#include "GabberUtility.hh"
#include "GabberWidgets.hh"
#include "GabberWin.hh"

#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-util.h> // for g_file_exists()

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace jabberoo;
using namespace GabberUtil;

static std::string printBytes(float bytes)
{
    char string_buffer[32];
    static const char* size_name[] = {"B", "kB", "MB", "GB", "TB"};
    int i = 0;
    float final_size = bytes;

    while ( final_size > 1024 )
    {
        final_size = final_size / 1024.0;
        i++;
    }

    snprintf(string_buffer, 32, "%.1f %s", final_size, size_name[i]);
    return string(string_buffer);
}

// Init function to register view with MessageManager
FTRecvView::FTRecvView(const judo::Element& t) : 
     BaseGabberDialog("FTRecv_dlg"),
     _jid(t.getAttrib("from"))
{
     init();

     render(t);
}

void FTRecvView::init()
{
     // XXX Handle default path better
     _gfeSaveTo = getWidget<Gnome::FileEntry>("FTRecv_SaveTo_FileEntry");
     Gtk::Entry* entry = dynamic_cast<Gtk::Entry*>(_gfeSaveTo->gtk_entry());
     char path[1024];
     entry->set_text(getcwd(path, 1024));
     _clistFiles = getWidget<Gtk::CTree>("FTRecv_Files_clist");
     Gtk::ScrolledWindow* tsw = getWidget<Gtk::ScrolledWindow>("FTRecv_Files_scroll");

     getButton("FTRecv_Remove_btn")->clicked.connect(slot(this, &FTRecvView::on_Remove_clicked));
     getButton("FTRecv_Accept_btn")->clicked.connect(slot(this, &FTRecvView::on_Accept_clicked));
     getButton("FTRecv_Cancel_btn")->clicked.connect(slot(this, &FTRecvView::on_Cancel_clicked));

     PrettyJID* pj = manage(new PrettyJID(_jid, "", PrettyJID::dtNickRes));
     pj->show();
     getWidget<Gtk::HBox>("FTRecv_JIDInfo_hbox")->pack_start(*pj, true, true, 0);

     _thisWindow->set_title(substitute(_("Receive File(s) from %s"), fromUTF8(pj->get_nickname())) + _(" - Gabber"));
}

FTRecvView::~FTRecvView()
{
}

void FTRecvView::render(const judo::Element& baseElement)
{
    Element::const_iterator it = baseElement.begin();
    _clistFiles->freeze();
    Element* child;
    Gtk::Label* l = getLabel("FTRecv_Description_lbl");
    int goodURIs = 0; 
    for (; it != baseElement.end(); it++)
    {
        child = static_cast<Element*>(*it);

        if (child->getType() != judo::Node::ntElement)
            continue;

        if ((child->getName() == "query") && 
            (child->getAttrib("xmlns") == "jabber:iq:oob"))
        {
            const char* data[2];
            string uri = fromUTF8(_clistFiles, child->getChildCData("url"));
            data[0] = uri.c_str();
            if ( ghttp_uri_validate((char*)data[0]) != -1 )
            { 
                ++goodURIs; // This is a good URI.  Count it.
                // XXX something else needs to be done with "desc" CDATA.
                // If there are multiple files, we'll only see desc for the
                // last one:
                l->set_text(fromUTF8(l, child->getChildCData("desc")));
                _clistFiles->rows().push_back(data);
            }
            else  // Mark the URI as invalid (but still display it).
            {
                data[0] = ("INVALID URL: " + uri).c_str();
                _clistFiles->rows().push_back(data);
            }
        }
    }
    _clistFiles->thaw();
    if (goodURIs < 1)
    {  // If all of the URIs are invalid, don't allow the user to attempt a download.
        getButton("FTRecv_Accept_btn")->set_sensitive(false);
        l->set(substitute(_("%s tried to offer files, but all the URLs sent are invalid."), fromUTF8(PrettyJID(_jid, "", PrettyJID::dtNickRes).get_nickname())));
    }
}

void FTRecvView::on_Remove_clicked()
{
    if (_clistFiles->selection().begin() != _clistFiles->selection().end())
    {
        Gtk::CList::Row& row = *_clistFiles->selection().begin();
        _clistFiles->rows().remove(_clistFiles->row(row.get_row_num()));
    }
}

void FTRecvView::on_Accept_clicked()
{
    // XXX Do stuff to start the get -- make sure calls to ghttp_uri_validate() are involved.
    std::string path = _gfeSaveTo->get_full_path(false);
    if (path.empty())
    {
        Gnome::Dialog* d = manage(Gnome::Dialogs::warning(_("Error, a file save path must be specified.")));
		d->set_parent(*_thisWindow);
        d->set_modal(true);
        return;
    }
    Gtk::CList::RowIterator it = _clistFiles->rows().begin();
    for (; it != _clistFiles->rows().end(); it++)
    {
       Gtk::CList::Row& row = *(it);
       std::string url = toUTF8(_clistFiles, row[0].get_text());

       // FIXME: Build a string list of URLs which pass ghttp_uri_validate()
       // FIXME: make FTTransferDlg accept this string list and go through and download them all
       manage(new FTTransferDlg(_jid, url, path));
    }
    close();
}

void FTRecvView::on_Cancel_clicked()
{
    close();
}


/**********************************
* The Send Interface
*********************************/
FTSendDlg::FTSendDlg(const std::string& jid) :
    BaseGabberDialog("FTSend_dlg"), _jid(jid)
{
    _txtDescription = getWidget<Gtk::Text>("FTSend_Description_txt");
    _gentFromIP = getWidget<Gnome::Entry>("FTSend_FromIP_gent");
    _entFromIP = getWidget<Gtk::Entry>("FTSend_FromIP_ent");
    G_App->getCfg().loadEntryHistory(getWidget<Gnome::Entry>("FTSend_FromIP_gent"));
    _gfeFile = getWidget<Gnome::FileEntry>("FTSend_File_gfe");
    Gtk::Entry* entry = dynamic_cast<Gtk::Entry*>(_gfeFile->gtk_entry());
    // Monitor whether the entered file is valid...
    entry->changed.connect(slot(this, &FTSendDlg::on_file_entry_changed));
    entry->set_text("");

    _pjid = manage(new PrettyJID(_jid, "", PrettyJID::dtNick, 128, true));
    _pjid->show();
    getWidget<Gtk::HBox>("FTSend_JIDInfo_hbox")->pack_start(*_pjid, true, true, 0);

    _thisWindow->set_title(substitute(_("Send File to %s"), fromUTF8(_pjid->get_nickname())) + _(" - Gabber"));

    getButton("FTSend_Send_btn")->clicked.connect(slot(this, &FTSendDlg::on_Send_clicked));
    getButton("FTSend_Cancel_btn")->clicked.connect(slot(this, &FTSendDlg::on_Cancel_clicked));

    char ac[1024];
    if (gethostname(ac, sizeof(ac)) < 0) 
    {
        cerr << "Error when getting local host name." << endl;
    }
    else
    {
	_entFromIP->set_text(G_App->getTransmitter().getsockname());
    }
}

FTSendDlg::~FTSendDlg()
{
}

void FTSendDlg::on_file_entry_changed()
{
     Gtk::Entry* entry = dynamic_cast<Gtk::Entry*>(_gfeFile->gtk_entry());

     // Check to see if anything has been entered
     if (entry->get_text_length() > 0)
     {
	  std::string file = _gfeFile->get_full_path(false);

	  if (g_file_exists(file.c_str()))
	  {
	       getButton("FTSend_Send_btn")->set_sensitive(true);
	  }

	  else
	  {
	       getButton("FTSend_Send_btn")->set_sensitive(false);
	  }
     }

     else
     {
	  getButton("FTSend_Send_btn")->set_sensitive(false);
     }
}

void FTSendDlg::on_Send_clicked()
{
    std::string file = _gfeFile->get_full_path(false);

    std::string description = toUTF8(_txtDescription, _txtDescription->get_chars(0, -1));

    manage(new FTTransferDlg(_pjid->get_full_jid(), file, description, _entFromIP->get_text()));

    G_App->getCfg().saveEntryHistory(getWidget<Gnome::Entry>("FTSend_FromIP_gent"));

    close();
}

void FTSendDlg::on_Cancel_clicked()
{
    close();
}

/*************************************
 *
 * The Transfer Interface 
 *
 ************************************/
// Send
FTTransferDlg::FTTransferDlg(const std::string& jid, const std::string& file,
                             const std::string& description,
                             const std::string& from_ip) :
    BaseGabberWindow("FTTransfer_dlg"), _total(0), _do_data(false), 
    _is_server(true), _info_hidden(true), _cleanup(false)
{
     struct stat info;
     stat(file.c_str(), &info);
     _content_length = info.st_size;
     _file = new fstream(file.c_str(), ios::in | ios::binary);
     
     // XXX FINISH ME!
     _Transmitter = new TCPTransmitter;
     
     _Transmitter->evtConnected.connect(slot(this, &FTTransferDlg::on_transmitter_connected));
     _Transmitter->evtAccepted.connect(slot(this, &FTTransferDlg::on_transmitter_accepted));
     _Transmitter->evtDisconnected.connect(slot(this, &FTTransferDlg::on_transmitter_disconnected));
     _Transmitter->evtError.connect(slot(this, &FTTransferDlg::on_transmitter_error));
     _Transmitter->evtDataAvailable.connect(slot(this, &FTTransferDlg::on_transmitter_data));
     _Transmitter->evtDataSent.connect(slot(this, &FTTransferDlg::on_transmitter_datasent));
     _Transmitter->evtCanSendMore.connect(slot(this, &FTTransferDlg::on_transmitter_sendmore));
     getWidget<Gtk::EventBox>("FTTransfer_Expand_evt")->button_press_event.connect(slot(this, &FTTransferDlg::on_Expand_clicked));

    // Set our initial status
    _lblStatus = getWidget<Gtk::Label>("FTTransfer_Status_lbl");
    _lblStatus->set_text(_("Waiting for connection..."));
    // Get and setup the pbar
    _pbarProgress = getWidget<Gtk::ProgressBar>("FTTransfer_Progress_pbar");
    _pbarProgress->configure(0, 0, _content_length);
    // Show where the file is locally
    Gtk::Label* tmp_lbl = getLabel("FTTransfer_Location_lbl");
    tmp_lbl->set_text(fromUTF8(tmp_lbl, file));
    // Show how big the file is
    tmp_lbl = getLabel("FTTransfer_Size_lbl");
    tmp_lbl->set_text(printBytes(_content_length));
    // Size left is initially same as full size
    _lblSizeLeft = getLabel("FTTransfer_SizeRemaining_lbl");
    _lblSizeLeft->set_text(printBytes(_content_length));
    // Get our label for speed info, its glade setting is fine
    _lblSpeed = getLabel("FTTransfer_TransferSpeed_lbl");

    PrettyJID* pj = manage(new PrettyJID(jid, "", PrettyJID::dtNickRes));
    pj->show();
    getHBox("FTTransfer_JIDInfo_hbox")->pack_start(*pj, true, true, 0);

    _thisWindow->set_title(substitute(_("Sending File to %s"), fromUTF8(pj->get_nickname())) + _(" - Gabber"));

    getWidget<Gnome::Pixmap>("FTTransfer_Icon_pix")->load(string(ConfigManager::get_SHAREDIR()) + "glade-send-file.xpm");
     
    getButton("FTTransfer_Stop_btn")->clicked.connect(slot(this, &FTTransferDlg::on_Cancel_clicked));

    // Listen on all hosts for a random port
    _Transmitter->listen("", G_App->getCfg().filetransfer.port);

    char port[8];
    char hostname[1024];
    // Get the port, hostname, and file
    snprintf(port, 8, "%d", _Transmitter->getPort());
    std::string::size_type p1 = file.rfind("/");
    std::string base_file = file.substr(p1, string::npos);
    _path = "GET " + base_file + " HTTP/1.0";
    std::string url("http://");
    url += from_ip;
    url += ":";
    url += port + base_file;
    // Send out the iq set
    Element iq("iq");
    iq.putAttrib("type", "set");
    iq.putAttrib("to", jid);
    iq.putAttrib("id", G_App->getSession().getNextID());
    Element* query = iq.addElement("query");
    query->putAttrib("xmlns", "jabber:iq:oob");
    query->addElement("url", url);
    query->addElement("desc", description);

    G_App->getSession() << iq;
}

// Recv
FTTransferDlg::FTTransferDlg(const std::string& jid,
			     const std::string& url, 
                             const std::string& save_path) :
     BaseGabberWindow("FTTransfer_dlg"), _content_length(0), _total(0),
     _do_data(false), _is_server(false), _info_hidden(true), _cleanup(false)
{
     _Transmitter = new TCPTransmitter;
     
     _Transmitter->evtConnected.connect(slot(this, &FTTransferDlg::on_transmitter_connected));
     _Transmitter->evtDisconnected.connect(slot(this, &FTTransferDlg::on_transmitter_disconnected));
     _Transmitter->evtError.connect(slot(this, &FTTransferDlg::on_transmitter_error));
     _Transmitter->evtDataAvailable.connect(slot(this, &FTTransferDlg::on_transmitter_data));
     
     // URL Pieces
     // http://box5.net:25322/~temas/jabber_test.txt
     // [method]://[host]:[port]/[path.../[filename]]
     // With raw IPv6 addresses the host is in square brackets.
     std::string host;
     unsigned int port;
     
     // We have to have a method
     // XXX Better error checking
     string::size_type p1 = url.find("://");
     std::string method = url.substr(0, p1);
     p1 += 3; // position of host
     string::size_type p2;
     string::size_type p3 = url.find("/", p1);
     // check for square brackets
     string::size_type obr = url.find("[", p1);
     if (obr != std::string::npos)
     { // square brackets found
	  string::size_type cbr = url.find("]", obr);
	  if (cbr != std::string::npos)
	  {
               host = url.substr(obr+1, cbr-obr-1);
	       p2 = url.find(":", cbr);
	       if (p2 != std::string::npos) {
	       std::string sport = url.substr(p2 + 1, p3 - p2 -1);
	       port = atoi(sport.c_str());
	       } else {
	            port = 80;
	       }
	  } else {
               cerr << "URI can not be parsed!" << endl;
	       host = "";
	       port = 80;
	  }
     } else { // no square brackets
          p2 = url.find(":", p1);
	  if (p2 != std::string::npos)
	  { // we have a port
	       host = url.substr(p1, p2 - p1);
	       std::string sport = url.substr(p2 + 1, p3 - p2 -1);
	       port = atoi(sport.c_str());
	  } else {
               host = url.substr(p1, p3 - p1);
	       port = 80;
	  }
     }
     //
     // Get the path and cut out the filename
     _path = url.substr(p3, std::string::npos - p3);
     std::string filename;
     if (_path.empty())
     {
	  _path = "/";
     } else {
	  p1 = _path.rfind("/");
	  filename = _path.substr(p1 + 1, string::npos);
     }
     if (filename.empty())
	  filename = "index";
     filename = save_path + "/" + filename;

     // debug
     cout << "Parsed URI:" << endl <<
	     "Host " << host << endl <<
	     "Port " << port << endl <<
	     "File " << filename << endl;
     
     // Construct our file stream
     _file = new fstream(filename.c_str(), ios::out | ios::binary);
     
     getWidget<Gtk::Label>("FTTransfer_Direction_lbl")->set_text(_("Receiving from"));
     // Get our status and progress bar
     _lblStatus = getWidget<Gtk::Label>("FTTransfer_Status_lbl");
     _pbarProgress = getWidget<Gtk::ProgressBar>("FTTransfer_Progress_pbar");
     // Show where the file is locally
     Gtk::Label* l = getLabel("FTTransfer_Location_lbl");
     l->set_text(fromUTF8(l, filename));
     // Show how big the file is
     getWidget<Gtk::Label>("FTTransfer_Size_lbl")->set_text(_("Pending..."));
     // Size left is initially same as full size
     _lblSizeLeft = getWidget<Gtk::Label>("FTTransfer_SizeRemaining_lbl");
     _lblSizeLeft->set_text(_("Pending..."));
     // Get our label for speed info, it's glade setting is fine
     _lblSpeed = getWidget<Gtk::Label>("FTTransfer_TransferSpeed_lbl");
     
     PrettyJID* pj = manage(new PrettyJID(jid, "", PrettyJID::dtNickRes));
     pj->show();
     getHBox("FTTransfer_JIDInfo_hbox")->pack_start(*pj, true, true, 0);
     
     _thisWindow->set_title(substitute(_("Receiving File from %s"), fromUTF8(pj->get_nickname())) + _(" - Gabber"));
     
     getWidget<Gnome::Pixmap>("FTTransfer_Icon_pix")->load(string(ConfigManager::get_SHAREDIR()) + "glade-receive-file.xpm");
     
     getButton("FTTransfer_Stop_btn")->clicked.connect(slot(this, &FTTransferDlg::on_Cancel_clicked));
     getWidget<Gtk::EventBox>("FTTransfer_Expand_evt")->button_press_event.connect(slot(this, &FTTransferDlg::on_Expand_clicked));

     _Transmitter->connect(host, port, (method=="https") ? true : false, false);
     
}

FTTransferDlg::~FTTransferDlg()
{
     delete _file;
     delete _Transmitter;
}

void FTTransferDlg::on_Cancel_clicked()
{
     _cleanup = true;
     _file->close();
     _Transmitter->disconnect();
}

void FTTransferDlg::on_transmitter_connected()
{
     _lblStatus->set_text(_("Connected, requesting file..."));
     std::string hdr("GET " + _path + " HTTP/1.0\r\n\r\n");
     _Transmitter->send(hdr.c_str());
}

void FTTransferDlg::on_transmitter_accepted()
{
     _lblStatus->set_text(_("Accepting connection..."));
}

gint FTTransferDlg::on_errormsg_closed()
{
     close();
     return 0;
}

void FTTransferDlg::on_transmitter_disconnected()
{
     _file->close();
     cerr << "Total: " << _total << " Len: " << _content_length << " State: " << _cleanup << endl;
     if ((_total == _content_length) || _cleanup)
     {
	  close();
     }
     else
     {
	  _lblStatus->set_text(_("Error, transfer unexpectedly ended."));
	  Gnome::Dialog* d = manage(Gnome::Dialogs::error(_("The file transfer has ended unexpectedly.\nThe received portion of the file has been saved.")));
		d->set_parent(*_thisWindow);
	  d->close.connect(slot(this, &FTTransferDlg::on_errormsg_closed));
     }
}

void FTTransferDlg::on_transmitter_error(const std::string& emsg)
{
     cerr << "ERROR!!" << endl;
     cerr << emsg << endl;
     _Transmitter->disconnect();
}

void FTTransferDlg::on_transmitter_sendmore()
{
     char buf[1024];
     int len = 0;
     if (!_file->eof())
     {
	  _file->read(buf, 1024);
	  len = _file->gcount();
	  _Transmitter->sendsz(buf, len);
	  _Transmitter->needSend(true);
     }
}

void FTTransferDlg::on_transmitter_datasent(int sz)
{
     if (_is_server)
     {
	  if (_do_data)
	       UpdateProgress(sz);
     }
}

void FTTransferDlg::on_transmitter_data(const char* data, int sz)
{
     if (_is_server)
     {
	  handleSend(data, sz);
     } else {
	  handleRecv(data, sz);
     }
}

gint FTTransferDlg::on_Expand_clicked(GdkEventButton* button)
{
     Gtk::Arrow* arrow = getWidget<Gtk::Arrow>("FTTransfer_Expand_arw");
     Gtk::Table* table = getWidget<Gtk::Table>("FTTransfer_Info_tbl");
     
     if (_info_hidden)
     {
	  getLabel("FTTransfer_Expand_lbl")->set_text(_("Hide details"));
	  _info_hidden = false;
	  table->show();
	  arrow->set(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
     }
     else
     {
	  getLabel("FTTransfer_Expand_lbl")->set_text(_("Show details"));
	  _info_hidden = true;
	  table->hide();
	  arrow->set(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
     }

     return 1;
}

void FTTransferDlg::handleSend(const char* data, int sz)
{
     if (!_do_data)
     {
          // Make sure we are handling a GET call
          if (strncasecmp(data, "GET", 3) != 0)
          {
              _Transmitter->send("HTTP/1.0 400 Bad Request\r\n");
              _Transmitter->disconnect();
          }

	  _lblStatus->set_text(_("Sending..."));
	  _Transmitter->send("HTTP/1.0 200 OK\r\n");
	  char cl_buf[10];
	  snprintf(cl_buf, 10, "%d", _content_length);
	  _Transmitter->send("Content-Length: ");
	  _Transmitter->send(cl_buf);
      _Transmitter->send("\r\n");
	  _Transmitter->send("Content-Type: text/plain\r\n\r\n");
	  
	  _do_data = true;
	  time(&_startTime);

	  char buf[1024];
	  int len = 0;
	  if (!_file->eof())
	  {
	       _file->read(buf, 1024);
	       len = _file->gcount();
	       _Transmitter->sendsz(buf, len);
	  }
	  _Transmitter->needSend(true);
	  
    }
}

void FTTransferDlg::handleRecv(const char* data, int sz)
{
     if(_content_length == 0)
     {
	  _lblStatus->set_text(_("Receiving data..."));
	  char *loc = strstr(data, "Content-Length: ");
	  char *data_loc = strstr(data, "\r\n\r\n");
	  unsigned int dsz = sz - ((data_loc + 4) - data);
	  
	  // It's junk we don't care about
	  if (loc == NULL && data_loc == NULL)
	       return;
	  if (loc != NULL)
	  {
	       loc += 16;
	       char *brkloc = strchr(loc, '\r');
	       *brkloc = '\0';
	       
	       // Set our content length
	       _content_length = atoi(loc);
	       // Set the labels to the correct values
	       getWidget<Gtk::Label>("FTTransfer_Size_lbl")->set_text(printBytes(_content_length));
	       // remaining is currently the same as the full size
	       getWidget<Gtk::Label>("FTTransfer_SizeRemaining_lbl")->set_text(printBytes(_content_length));
	       
	       // reset the progress bar
	       _pbarProgress->configure(0, 0, _content_length);
	       
	       if (brkloc == data_loc)
	       {
		    _do_data = true;
		    time(&_startTime);
	       }
	  }
	  
	  // If we are ready to start flag it so and make sure we don't 
	  // dump good data
	  if (data_loc != NULL)
	  {
	       _do_data = true;
	       time(&_startTime);
	       if (dsz > 0)
	       {
		    data_loc += 4;
		    _file->write(data_loc, dsz);
		    UpdateProgress(dsz);
	       }
	  }

	  return;
     }
     
     if (!_do_data)
     {
	  char *data_loc = strstr(data, "\r\n\r\n");
	  unsigned int dsz = sz - ((data_loc - data) + 4);
	  if (data_loc != NULL)
	  {
	       _do_data = true;
	       time(&_startTime);
	       if (dsz > 0)
	       {
		    data_loc += 4;
		    _file->write(data_loc, dsz);
		    UpdateProgress(dsz);
	       }
	  }
	  return;
     }

     _file->write(data, sz);
     UpdateProgress(sz);
}

static std::string _itos(float i)
{
     char buf[16];
     snprintf(buf, 16, "%.0f", i);
     return string(buf);
}

void FTTransferDlg::UpdateProgress(unsigned int sz)
{
     // Update the size and labels representing it
     _total += sz;
     _pbarProgress->set_value(_total);
     _pbarProgress->draw(NULL);
     
     // See if we're done
     if (_total == _content_length)
     {
	  _file->close();
	  _lblStatus->set_text(_("Complete."));
	  _Transmitter->disconnect();
     } 
     else if (_total > _content_length) 
     {
	  _lblStatus->set_text(_("Error, received more data than expected."));
	  _Transmitter->disconnect();
     }
     else
     {
	  
	  // eww, nasty redraw hack
	  _lblSizeLeft->set_text("");
	  _lblSizeLeft->set_text(printBytes(_content_length - _total));
	  _lblSizeLeft->draw(NULL);
	  
	  // How long have we been working on this?
	  time_t now, elapsed;
	  time(&now);
	  elapsed = now - _startTime;
	  
	  // Damn you divide by zero
	  if (elapsed > 0)
	  {
	       float speed = (float)_total / (float)elapsed;
	       float time_left = (float)(_content_length - _total)/ speed;
	       
	       //This is an icky hack to redraw better
	       _lblSpeed->set_text("");
	       // Update the speed
	       _lblSpeed->set_text(substitute(_("%s/second"), printBytes(speed)));
	       _lblSpeed->draw(NULL);
	       
	       // How much longer is left
	       std::string status;
	       if (time_left < 60) 
	       {
		    status = _("Under one minute remaining.");
	       }
	       else if (time_left > 3600)
	       {
		    float hours = (time_left / 3600);
		    time_left -= (hours * 3600);
		    status = substitute(_("%s hours, %s minutes remaining."), _itos(hours), _itos(time_left / 60.0));
	       }
	       else
	       {
		    status = substitute(_("%s minutes remaining."), _itos(time_left / 60.0));
	       }
	       _lblStatus->set_text("");
	       _lblStatus->set_text(status);
	  }
     }
}
