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

#ifndef INCL_FT_INTERFACE_HH
#define INCL_FT_INTERFACE_HH

#include <fstream>
#include "jabberoo.hh"

#include "BaseGabberWindow.hh"
#include "RosterView.hh"
#include "TCPTransmitter.hh"

#include <gnome--.h>

class PrettyJID;

/*******************
* Recv dialog
*******************/
class FTRecvView :
    public BaseGabberDialog
{
public:
    FTRecvView(const judo::Element& t);
    ~FTRecvView();
    void render(const judo::Element& baseElement);
protected:
    void on_Remove_clicked();
    void on_Cancel_clicked();
    void on_Accept_clicked();

    void init();
private:
     string                 _jid;
    Gtk::CList*             _clistFiles;
    Gnome::FileEntry*       _gfeSaveTo;
};

/********************
* Send Dialog
********************/
class FTSendDlg :
    public BaseGabberDialog
{
public:
    FTSendDlg(const std::string& jid);
    ~FTSendDlg();
protected:
    void on_file_entry_changed();
    void on_Send_clicked();
    void on_Cancel_clicked();
private:
    PrettyJID*               _pjid;
    Gnome::Entry*             _gentFromIP;
    Gtk::Entry*               _entFromIP;
    Gtk::Text*                _txtDescription;
    Gnome::FileEntry*         _gfeFile;
    string		              _jid;
};

/***********************
 * Transfer dialog
 **********************/
class FTTransferDlg :
    public BaseGabberWindow
{
public:
    // Sending startup
    FTTransferDlg(const std::string& jid, const std::string& file,
                  const std::string& description,
                  const std::string& from_ip);
    // Recving startup
    FTTransferDlg(const std::string& jid, const std::string& url, 
		  const std::string& save_path);
    ~FTTransferDlg();
protected:
    void on_Cancel_clicked();

    void on_transmitter_connected();
    void on_transmitter_accepted();
    void on_transmitter_disconnected();
    void on_transmitter_error(const std::string& emsg);
    void on_transmitter_sendmore();
    void on_transmitter_datasent(int sz);
    void on_transmitter_data(const char* data, int sz);

    gint on_errormsg_closed();

    gint on_Expand_clicked(GdkEventButton* button);
private:
    TCPTransmitter*     _Transmitter;
    std::string         _path;
    Gtk::Label*         _lblStatus;
    Gtk::Label*         _lblSizeLeft;
    Gtk::Label*         _lblSpeed;
    Gtk::ProgressBar*   _pbarProgress;
    fstream*            _file;
    unsigned int        _content_length;
    unsigned int        _total;
    time_t              _startTime;
    bool                _do_data;
    bool                _is_server;
    bool                _info_hidden;
    bool                _cleanup;

    void handleSend(const char* data, int sz);
    void handleRecv(const char* data, int sz);
    void UpdateProgress(unsigned int sz);
};

#endif // INCL_FT_INTERFACE_HH
