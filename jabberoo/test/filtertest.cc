#include <string>
#include <iostream>
using namespace std;

#include "jabberoo.hh"
#include "jabberoox.hh"
using namespace jabberoo;

int main(int argc, char** argv)
{
     Element t("query");
     t.putAttrib("xmlns", "jabber:iq:filter");
     Element* r = t.addElement("rule");
     r->putAttrib("name", "default");
     r->addElement("unavailable");
     r->addElement("offline");

     FilterList f(t);     
     cerr << "Filter list: " << f.toXML() << endl;

     Filter& f1 = *f.begin();
     cerr << "Filter 1: " << f1.Name() << "\t" << f1.toXML() << endl;

     Filter f2(f1);
     cerr << "Filter 2: " << f2.Name() << "\t" << f2.toXML() << endl;

     return 0;
}
