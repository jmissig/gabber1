#include "jabberoo.hh"
using namespace jabberoo;

#include <iostream>
using namespace std;

int main(int argc, char** argv)
{
     string jid = "user@host.com/resource";
     cerr << "JID: " << jid << endl
	  << "getResource: " << JID::getResource(jid) << endl
	  << "getUserHost: " << JID::getUserHost(jid) << endl
	  << "getHost: " << JID::getHost(jid) << endl;

     string jid1 = "jabber@groups.jabber.org/Dizzyd";
     string jid2 = "user@host.com";
     string jid3 = "host.com/resource";
     string jid4 = "host.com";
     cerr << "1: " << jid1 << "\t" << JID::getHost(jid1) << endl
	  << "2: " << jid2 << "\t" << JID::getHost(jid2) << endl
	  << "3: " << jid3 << "\t" << JID::getHost(jid3) << endl
	  << "4: " << jid4 << "\t" << JID::getHost(jid4) << endl;

     return 0;
}
