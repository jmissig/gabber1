
#include "jabberoo.hh"
using namespace jabberoo;

#include <iostream>
using namespace std;

void addItem(Element& query, const char* jid, const char* name)
{
     Element* item = query.addElement("item");
     item->putAttrib("jid", jid);
     item->putAttrib("name", name);
}

void processItem(const Roster::Item& i)
{
     cout << i.getJID() << endl;
}

int main(int argc, char** argv)
{
     Element query("query");

     addItem(query, "foo@bar.org", "foo");
     addItem(query, "bar@bar.org", "bar");
     addItem(query, "joe@bar.org", "joe");

     Session s;
     Roster r(s);
     r.update(query);

     for_each(r.begin(), r.end(), &processItem);

     for (Roster::iterator i = r.begin(); i != r.end(); i++)
     {
	  cout << i->getJID() << endl;
     }

     return 0;
}

