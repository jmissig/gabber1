/* jabberoo.hh
 * Jabber client library
 *
 * Original Code Copyright (C) 1999-2001 Dave Smith (dave@jabber.org)
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
 * Contributor(s): Julian Missig
 *
 * This Original Code has been modified by IBM Corporation. Modifications 
 * made by IBM described herein are Copyright (c) International Business 
 * Machines Corporation, 2002.
 *
 * Date             Modified by     Description of modification
 * 01/20/2002       IBM Corp.       Updated to libjudo 1.1.1
 * 2002-03-05       IBM Corp.       Updated to libjudo 1.1.5
 * 2002-07-09       IBM Corp.       Added Roster::getSession()
 */

#ifndef INCL_JABBEROO_H
#define INCL_JABBEROO_H

#ifdef WIN32
#pragma warning (disable:4786)
#pragma warning (disable:4503)
#include <windows.h>
#define snprintf _snprintf
#define strcasecmp _stricmp
#include <string.h>
#endif

#include <time.h>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <set>
#include <algorithm>
#include <iostream>
using namespace std;

#include <jutil.hh>
#include <judo.hpp>
using namespace judo;

#include <sigc++/signal_system.h>
#include <sigc++/object_slot.h>
using namespace SigC;

/**
 * Core namespace for the object-oriented Jabber client library.
 */
namespace jabberoo
{
     // Predeclarations
     class Message;
     class Presence;
     class Roster;
     class Session;

     const int ERR_UNAUTHORIZED = 401;

     /***********XCP CLASSES**********/
     class XCP 
	  {
	  public:
	       const string& getMessage() const { return _msg; }
	       XCP(const char* msg) : _msg(msg) {} 
	       XCP() {}
	  private:
	       string _msg;
	  };

     class XCP_NotImplemented : public XCP 
	  {
	  public:
	       XCP_NotImplemented( const char * msg ) : XCP( msg ) { }
	  };


     /**
      * A Jabber Packet with standard attributes.
      * This class has accessors and modifiers for several common Jabber attributes and elements.
      */
     class Packet
	  {
	  public:
	       /**
		* Jabber Packet constructor with a base element of name.
		* Create a Jabber Packet with a base element of name.
		* @param name A string base element name.
		*/
	       Packet(const string& name);

	       /**
		* Jabber Packet constructor based upon a judo::Element
		* Create a Jabber Packet based off of a judo::Element.
		* @param t A judo::Element
		*/
	       Packet(const Element& t);

	       // Common ops
	       /**
		* from attribute on the base element.
		* @return The string in the from attribute on the base element, NULL if the from attribute is nonexistent.
		*/
	       const string getFrom()    const;

	       /**
		* to attribute on the base element.
		* @return The string in the to attribute on the base element, NULL if the to attribute is nonexistent.
		*/
	       const string getTo()      const;

	       /**
		* id attribute on the base element.
		* @return The string in the id attribute on the base element, NULL if the id attribute is nonexistent.
		*/
	       const string getID() const;

	       /**
		* error subelement of the base element.
		* @return The string in the error subelement of the base element, NULL if the error subelement is nonexistent.
		*/
	       const string getError() const;

	       /**
		* code attribute on the error subelement.
		* @return The int in the code attribute on the error subelement, 0 if the code attribute or the error subelement is nonexistent.
		*/
	       const int getErrorCode() const;

	       /**
		* XML string form of the Jabber Packet.
		* @return The XML forming the Jabber Packet as a string.
		*/
	       const string toString()      const;

	       /**
		* XML judo::Element form of the Jabber Packet.
		* @return The XML forming the Jabber Packet as a judo::Element.
		* @see judo::Element
		*/
	       const Element&   getBaseElement() const;

	       /**
		* from attribute on the base element.
		* @param from The JabberID to go in the from attribute on the base element, normally set by the server.
		*/
	       void setFrom(const string& from);

	       /**
		* to attribute on the base element.
		* @param to The JabberID to send this packet to.
		*/
	       void setTo(const string& to);

	       /**
		* id attribute on the base element.
		* @param id The string id attribute on the base element.
		*/
	       void setID(const string& id);


	       /**
		* Add x element to the packet.
		* Adds an x (extension) element to the Jabber Packet.
		* x elements should have an xmlns attribute.
		* @see judo::Element
		* @see judo::Element::putAttrib()
		* @return A pointer to the newly created judo::Element.
		*/
	       Element* addX();

	       /**
		* Add x element to the packet.
		* Adds an x (extension) element to the Jabber Packet with
		* the given namespace.
		* @see judo::Element
		* @see judo::Element::putAttrib()
		* @return A pointer to the newly created judo::Element.
		*/
	       Element* addX(const string& tnamespace);

	       /**
		* Get the x (extension) element which has an xmlns of tnamespace.
		* @see judo::Element
		* @param tnamespace The namespace of the x element to return.
		* @return A pointer to the x element.
		*/
	       Element* findX(const string& tnamespace) const;

	       /**
		* Erase an x element from the packet.
		* Erases an x (extension) element to the Jabber Packet with
		* the given namespace.
		* @see judo::Element
		* @param tnamespace The namespace of the x element to return.
		*/
	       void eraseX(const string& tnamespace);

	       /**
		* Access to the base element
		* @see judo::Element
		* @return A reference to the base element of the Jabber Packet as a judo::Element
		*/
	       Element& getBaseElement();

	  protected:
	       Element _base;
	  };


     /**
      * A Jabber Message.
      * This class implements most of the methods needed to deal with a Jabber Message.
      * @see jabberoo::Packet
      */
     class Message
	  : public Packet
	  {
	  public:
	       // Make sure numTypes is updated in jabberoo-message.cc if you add
	       // a new Message Type
	       /**
		* Number of message types.
		*/
	       static const unsigned int numTypes;

	       /**
		* Message type.
		* This is a hint to clients as to how the message should be displayed.
		*/
	       enum Type {
		    mtNormal,    /**< A normal message (email, icq). */
		    mtError,     /**< An error message. */
		    mtChat,      /**< A chat message (AIM, IRC /msg's). */
		    mtGroupchat, /**< A groupchat message (IRC). */
		    mtHeadline   /**< A headline message (stock ticker, news ticker). */
	       };

	       /**
		* Construct a message based upon the given judo::Element.
		* @see judo::Element
		* @param t A judo::Element which should have a message base element
		*/
	       Message(const Element& t);

	       /**
		* Construct a message to a JabberID with a body.
		* Note that setting a body here is oh-so-slightly faster than calling
		* setBody(), since setBody() checks for a pre-existing body element.
		* @see Message::Type
		* @param jid The string of a JabberID to which this message is addressed.
		* @param body The string body of a message. Can be NULL.
		* @param mtype The Message::Type of the message. Defaults to mtNormal.
		*/
	       Message(const string& jid, const string& body, Type mtype = mtNormal);

	       // Modifiers
	       /**
		* Sets the body of the message.
		* @param body The string body of a message.
		*/
	       void  setBody(const string& body);

	       /**
		* Sets the subject of the message.
		* @param subject The string subject of the message.
		*/
	       void  setSubject(const string& subject);

	       /**
		* Sets the message thread.
		* The message thread is used by clients to relate messages to one another.
		* @param thread A string representing the thread, there is no rule as to how it should be generated.
		*/
	       void  setThread(const string& thread);
	       
	       /**
		* Sets the message type
		* @param mtype The Message::Type of the message. Defaults to mtNormal.
		*/
	       void  setType(Message::Type mtype);

	       /**
		* Request the delivered message event.
		* If the receiving client supports message events, a message with a message:x:event containing
		* a delivered element will be returned once this message has been delivered.
		*/
	       void requestDelivered();

	       /**
		* Request the displayed message event.
		* If the receiving client supports message events, a message with a message:x:event containing
		* a displayed element will be returned once this message has been displayed.
		*/
	       void requestDisplayed();

	       /**
		* Request the composing message event.
		* If the receiving client supports message events, a message with a message:x:event containing
		* a composing element will be returned when the receiving user begins composing a reply.
		*/
	       void requestComposing();

	       // Accessors
	       /**
		* Get the message body.
		* @return A string containing the message body.
		*/
	       const string getBody()     const;
	       /**
		* Get the message subject.
		* @return A string containing the message subject.
		*/
	       const string getSubject()  const;
	       /**
		* Get the message thread.
		* @return A string containing the message thread.
		*/
	       const string getThread()   const;
	       /**
		* Get the Message::Type.
		* @see Message::Type
		*/
	       Type         getType()     const;
	       /**
		* Get the date and time the message was sent with a given format.
		* @param format The format of the date and time.
		*/
	       const string getDateTime(const string& format = "") const;

	       // Factory methods
	       /**
		* Create a message in response to this message.
		* @param body The body of the reply message.
		* @return The new message.
		*/
	       Message replyTo(const string& body) const;

	       /**
		* Create a message event stating that this Message has been delivered.
		* It is up to the client to determine whether this message should actually
		* be generated and sent.
		* @return The delivered message event which should be sent.
		*/
	       Message Message::delivered() const;

	       /**
		* Create a message event stating that this Message has been displayed.
		* It is up to the client to determine whether this message should actually
		* be generated and sent.
		* @return The displayed message event which should be sent.
		*/	       
	       Message Message::displayed() const;

	       /**
		* Create a message event stating that this Message is being replied to.
		* It is up to the client to determine whether this message should actually
		* be generated and sent.
		* @return The composing message event which should be sent.
		*/
	       Message Message::composing() const;

	       // Static class methods
	       /**
		* Sets the date and time format.
		* @param format A string date and time format.
		*/
	       static void setDateTimeFormat(const string& format);
	       /**
		* Get the date and time format.
		*/
	       static const string& getDateTimeFormat();
	  protected:
	       // Specialized reply-to constructor
	       Message(const Message& m, const string& body);

	       static string translateType(Type mtype);
	       static Type   translateType(const string& mtype);
	  private:
	       Type  _type;
	       time_t _timestamp;
	       static string _dtFormat;
	  };

     /**
      * A Jabber Presence Packet.
      * This class implements most of the methods needed to deal with a Jabber Presence Packet.
      * @see jabberoo::Packet
      */
     class Presence
	  : public Packet
	  {
	  public:
	       /**
		* Presence type of the Packet.
		*/
	       enum Type {
		    ptSubRequest,   /**< A subscription request. */
		    ptUnsubRequest, /**< An unsubscription request. */
		    ptSubscribed,   /**< Subscription confirmed/allowed/approved. */
		    ptUnsubscribed, /**< Subscription revoked/denied. */
		    ptAvailable,    /**< Available presence (standard online presence). */
		    ptUnavailable,  /**< Unavailable presence (offline). */
		    ptError,        /**< Error (offline). */
		    ptInvisible     /**< Invisible presence, others see you as offline. */
	       };

	       /**
		* How the Presence should be displayed.
		* In addition to simply being available/unavailable, show hints at how a 
		* user's presence should be displayed in clients.
		*/
	       enum Show {
		    stInvalid, /**< Not a valid show type. */
		    stOffline, /**< Show user as offline. */
		    stOnline,  /**< Show user as online. */
		    stChat,    /**< Show user as wanting to chat. */
		    stAway,    /**< Show user as away. */
		    stXA,      /**< Show user as extended away. */
		    stDND      /**< Show user as Do Not Disturb. */
	       };

	       /**
		* Construct a Presence Packet based upon the given judo::Element.
		* @see judo::Element
		* @see jabberoo::Packet
		* @param t A judo::Element which should have a presence base element
		*/
	       Presence(const Element& t);

	       /**
		* Construct a Presence Packet based upon given values
		* @see jabberoo::Packet
		* @param jid The JabberID to send this presence to. An empty string sends to everyone who has the proper subscription.
		* @param ptype The Presence::Type of presence to send.
		* @param stype The Presence::Show for the presence, Presence::stInvalid leaves it blank.
		* @param status The status message for the presence. Can be an empty string.
		* @param priority The priority of this presence. Should be a string of an int which is 0 or greater. Empty string sets it to 0.
		*/
	       Presence(const string& jid, Type ptype, Show stype = stInvalid, const string& status = "", const string& priority = "0");

	       // Modifiers
	       /**
		* Set the presence type.
		* @see Presence::Type
		* @param ptype The Presence::Type of presence to send.
		*/
	       void setType(Presence::Type ptype);
	       /**
		* Set the presence status message.
		* @param status The status message for the presence. Can be an empty string.
		*/
	       void setStatus(const string& status);
	       /**
		* Set the presence show.
		* @see Presence::Show
		* @param stype The Presence::Show for the presence.
		*/
	       void setShow(Presence::Show stype);
	       /**
		* Set the presence priority.
		* @param priority The priority of this presence. Should be a string of an int.
		*/
	       void setPriority(const string& priority);

	       // Accessors
	       /**
		* Get the presence type.
		* @see Presence::Type
		* @return The Presence::Type of the Presence Packet.
		*/
	       Type          getType()     const;
	       /**
		* Get the presence show.
		* @see Presence::Show
		* @return The Presence::Show of the Presence Packet.
		*/
	       Show          getShow()     const;
	       /**
		* Get the presence status message.
		* In subscription request Presence Packets, the status message is the request reason.
		* @return The status message in the Presence Packet.
		*/
	       const string  getStatus()   const;
	       /**
		* Get the Presence::Show as a string.
		* @see Presence::Show
		* @return The string version of the Presence::Show of the Presence Packet.
		*/
	       const string  getShow_str() const;
	       /**
		* Get the priority of the presence.
		* The priority determines which resource messages should default to sending to
		* if the sender has logged in multiple times. If two resources have the same
		* priority, the most recently logged in resource is the default.
		* @return Priority as an int, 0 or greater. 0 if nonexistent.
		*/
	       int           getPriority() const;

	  protected:
	       static string translateType(Type ptype);
	       static Type   translateType(const string& ptype);
	       static string translateShow(Show stype);
	       static Show   translateShow(Type ptype, const string& stype);

	  private:
	       Show   _show;
	       Type   _type;
	       int    _priority;
	  };


     /**
      * Presence database.
      * This class keeps track of and handles all Presence packets received.
      * This class plus the Roster class are crucial for clients which want rosters.
      * @see Presence
      * @see Roster
      */
     class PresenceDB
	  : public SigC::Object
     {
     public:
	  typedef list<Presence>::iterator             iterator;
	  typedef list<Presence>::const_iterator       const_iterator;
	  typedef pair<const_iterator, const_iterator> range;
	  /**
	   * This is thrown if a JID is not in the database.
	   * @see equal_range
	   * @see findExact
	   * @see find
	   */
	  class XCP_InvalidJID : public XCP{};
	  	       
	  /**
	   * Create a PresenceDB for a given Session.
	   * This will begin keeping track of all Presence packets received in a given Session.
	   * @param s The Session.
	   * @see Session
	   */
	  PresenceDB(Session& s);
     public:
	  /**
	   * Insert a Presence Packet into the database.
	   * @param p The Presence Packet to insert.
	   */
	  void           insert(const Presence& p);
	  /**
	   * Remove a Presence Packet from the database.
	   * Currently this does *not* handle multiple entries with different resources.
	   * This function will remove all Presence Packets which match the user@host.
	   * @param jid The JabberID to remove.
	   */
	  void           remove(const string& jid);
	  /**
	   * Clear the database. Erases all entries.
	   */
	  void		 clear();
	  /**
	   * This function will throw XCP_InvalidJID if the JID is not found.
	   * @see XCP_InvalidJID
	   * @return range
	   */
	  range          equal_range(const string& jid) const;
	  /**
	   * Find Presence Packets from all JabberIDs which have the given user@host.
	   * This function will throw XCP_InvalidJID if the JID is not found.
	   * @return A const iterator of Presence Packets.
	   */
	  const_iterator find(const string& jid) const;
	  /**
	   * Find the Presence Packet for an exact user@host/resource JabberID.
	   * This function will throw XCP_InvalidJID if the JID is not found.
	   * @return A Presence Packet.
	   */
          Presence       findExact(const string& jid) const;
	  /**
	   * Whether the PresenceDB contains a given user@host.
	   */
	  bool           contains(const string& jid) const;
	  /**
	   * Whether the default Presence for user@host is available.
	   */
	  bool           available(const string& jid) const;
     private:
	  typedef map<string, list<Presence>, jutil::CaseInsensitiveCmp > db;
	  db::const_iterator find_or_throw(const string& jid) const;
	  Session&   _Owner;
	  db         _DB;
     };

     /**
      * A Jabber Roster.
      * A Roster is a list of users, or Roster Items.
      */
     class Roster
	  : public SigC::Object
	  {
	  public:
	       /**
		* The Subscription you have in relation to this Roster Item.
		* The Subscription determines who can see whose Presence.
		*/
               enum Subscription {
                    rsNone,  /**< Neither of you can see the other's presence. */
		    rsTo,    /**< You can see their presence, but they cannot see yours. */
		    rsFrom,  /**< You cannot see their presence, but they can see yours. */
		    rsBoth,  /**< Both of you can see each other's presence. */
		    rsRemove /**< This Roster Item is being removed. */
               };

	       // Exceptions
	       class XCP_InvalidJID : public XCP{};

	       /**
		* An individual Roster Item.
		*/
               class Item {
               public:
                    // Initializers
		    /**
		     * Create a Roster Item from a judo::Element.
		     * @param t A judo::Element.
		     * @see judo::Element
		     */
		    Item(const Element& t);
		    /**
		     * Create a Roster Item for a particular Roster, given a judo::Element.
		     * @param r A Roster.
		     * @param t A judo::Element.
		     * @see Roster
		     * @see judo::Element
		     */
                    Item(Roster& r, const Element& t);
		    /**
		     * Create a Roster Item based on a JabberID and a nickname.
		     * @param jid The JabberID.
		     * @param nickname The nickname.
		     */
		    Item(const string& jid, const string& nickname);
		    /**
		     * Roster Item destructor.
		     */
		    ~Item();
               public:
		    // Group modifiers
		    /**
		     * Add this Roster Item to a group.
		     * Roster Items can be in multiple groups.
		     * @param group The name of the group.
		     */
                    void addToGroup(const string& group);
		    /**
		     * Delete this Roster Item from a group.
		     * Roster Items can be in multiple groups.
		     * @param group The name of the group.
		     */
                    void delFromGroup(const string& group);
		    /**
		     * Remove this Roster Item from all groups it is in.
		     */
                    void clearGroups();
		    /**
		     * Set the nickname for this Roster Item.
		     * @param nickname The nickname.
		     */
		    void setNickname(const string& nickname);
		    /**
		     * Set the JabberID for this Roster Item.
		     * This will NOT modify an existing Roster Item.
		     * By changing the JabberID, you are essentially
		     * creating a new Roster Item.
		     * @param jid The JabberID.
		     */
		    void setJID(const string& jid);
        
                    // Info ops
		    /**
		     * Whether or not this Roster Item is online.
		     */
                    bool             isAvailable() const;
		    /**
		     * Get the nickname for this Roster Item.
		     */
                    string           getNickname() const;
		    /**
		     * Get the JabberID for this Roster Item.
		     */
                    string           getJID() const;
		    /**
		     * Get the Subscription to this Roster Item.
		     * @see Subscription
		     */
                    Subscription     getSubsType() const;
		    /**
		     * Whether or not this Roster Item is pending a subscription approval.
		     */
		    bool             isPending() const;

                    // Group pseudo-container ops/iterators
		    typedef set<string>::const_iterator iterator;
		    /**
		     * Get the first group this item belongs to.
		     * @return An iterator to the first group.
		     */
		    iterator begin() const { return _groups.begin(); }
		    /**
		     * Get the last group this item belongs to.
		     * @return An iterator to the end group.
		     */
		    iterator end()   const { return _groups.end(); }
		    /**
		     * Whether this item has any groups.
		     * @return False if this item belongs to any groups.
		     */
		    bool     empty() const { return _groups.empty(); }

	       protected:
                    // Update handlers
                    void update(Roster& r, const string& jid, const Presence& p, Presence::Type prev_type);
		    bool update(const Element& t);
                    bool update(Roster& r, const Element& t);
               private:
		    friend class Roster;
		    int          _rescnt;
                    set<string>  _groups;
                    Subscription _type;
		    bool         _pending;
                    string       _nickname;
                    string       _jid;
               };
	       
	       typedef map<string, Item, jutil::CaseInsensitiveCmp> ItemMap;

	       // Non-const iterators
	       typedef jutil::ValueIterator<ItemMap::iterator, Item > iterator;
	       /**
		* Get the first Roster::Item
		* @return An iterator to the first Roster Item.
		*/
	       iterator begin() { return _items.begin(); }
	       /**
		* Get the last Roster::Item
		* @return An iterator to the end Roster Item.
		*/
	       iterator end() { return _items.end(); }

	       // Const iterators
	       typedef jutil::ValueIterator<ItemMap::const_iterator, const Item> const_iterator;
	       /**
		* Get the first Roster::Item
		* @return A const iterator to the first Roster Item.
		*/
	       const_iterator begin() const { return _items.begin(); }
	       /**
		* Get the last Roster::Item
		* @return A const iterator to the end Roster Item.
		*/
	       const_iterator end() const { return _items.end(); }

               // Intializers
	       /**
		* Create a Roster for a given Session.
		* @param s The Session.
		* @see Session
		*/
               Roster(Session& s);

	       // Accessors
	       /**
		* Get the Session this Roster is using
		* @return The Session.
		*/
	       Session& getSession() { return _owner; }

	       // Operators
	       /**
		* Get a Roster Item in the Roster given a JabberID.
		*/
	       const Roster::Item& operator[](const string& jid) const;
	       /**
		* Add a Roster Item to the Roster.
		*/
	       Roster& operator<<(const Item& i) { update(i); return *this; }
          public:
               void reset();

               // Information
	       /**
		* Whether or not this Roster contains a certain JabberID.
		* @param jid The JabberID.
		*/
               bool containsJID(const string& jid) const;

               // Update ops
	       /**
		* Add a Roster Item to the Roster given a judo::Element.
		* This is how you push Items as they're received.
		* @param t The judo::Element
		*/
               void update(const Element& t);	   // Roster push
	       /**
		* Update the Presence for the appropriate Roster Items.
		* @param p The Presence.
		* @param prev_type The previous Presence::Type for the Roster Item the Presence refers to.
		*/
               void update(const Presence& p, Presence::Type prev_type);     // Roster presence
	       /**
		* Add a Roster Item.
		* This is generally used when an application wants to
		* add or modify Roster Items. Note that duplicate Roster Items
		* for a particular JabberID cannot exist, the previous one for 
		* the JabberID will be overwritten.
		* @param i The Roster::Item to add/modify.
		*/
	       void update(const Item& i);	   // Roster item add/modify

               // Control ops
	       /**
		* Remove a Roster Item based on JabberID.
		* @param jid The JabberID.
		*/
               void deleteUser(const string& jid); /* Remove the user w/ JID */
	       /**
		* Fetch the Roster from the server.
		*/
               void fetch() const;                 /* Retrieve roster from server */             

               // Translation
               static string translateS10N(Subscription stype);
               static Subscription translateS10N(const string& stype);
               static string filterJID(const string& jid);

	       // Item/Group access -- HACK
               const map<string, set<string> >& getGroups() const { return _groups;} 
          public:
	       /**
		* This signal is emitted whenever the Roster display should be refreshed.
		*/
               Signal0<void, Marshal<void> >                      evtRefresh;
	       /**
		* This signal is emitted whenever the Presence for a user changes.
		* @param jid The JabberID for the Presence.
		* @param available Whether or not this user is online.
		* @param prev_type The previous Presence::Type of this user.
		*/
	       Signal3<void, const string&, bool, Presence::Type, Marshal<void> > evtPresence;
          private:
	       friend class Item;
	       void mergeItemGroups(const string& itemjid, const set<string>& oldgrp, const set<string>& newgrp);
               void removeItemFromGroup(const string& group, const string& jid);
	       void removeItemFromAllGroups(const Item& item);
	       void deleteAgent(const Element& iq);
               ItemMap                   _items;
               map<string, set<string> > _groups;
               Session&                  _owner;
     };

     typedef Slot1<void, const Element&> ElementCallbackFunc;

     /**
      * A session with a Jabber server.
      * This class provides common operations needed for raw communication between the client and the server.
      */
     class Session : 
	  public ElementStreamEventListener, public ElementStream, public SigC::Object
	  {
	  public:
	       /**
		* The authorization type.
		*/
	       enum AuthType
	       { 
		    atPlaintextAuth, /**< Send the password as plaintext (plaintext). */
		    atDigestAuth,    /**< Send a hash of the password (digest). */
		    atAutoAuth,      /**< Automatically determine authorization type, defaulting to the most secure. */
		    at0kAuth         /**< Do not send any password information over the wire (zero knowledge). */
	       };

	       /**
		* The connection state.
		*/
	       enum ConnectionState
	       {
		    csNotConnected, /**< Not connected. */
		    csCreateUser,   /**< Create a new user. */
		    csAuthReq,      /**< Auhtorization request. */
		    csAwaitingAuth, /**< Awaiting authorization. */
		    csConnected     /**< Connected. */
	       };

	       // Initializers
	       /**
		* Construct a Session.
		* This constructs a Session.
		* @see push();
		* @see evtTransmitXML
		*/
	       Session();
	       /**
		* Deconstruct a Session.
		* This frees memory the Session was using.
		*/
	       virtual ~Session();

	       // Helper ops
	       /**
		* Read from the Session.
		* Reads data from the session into a const char* buffer.
		*/
	       Session& operator>>(const char* buffer) { push(buffer, strlen(buffer)); return *this; } 
	       /**
		* Send a Packet.
		* Sends a Packet through the session.
		*/
	       Session& operator<<(const Packet& p) { evtTransmitPacket(p); evtTransmitXML(p.toString().c_str()); return *this;}
	       /**
		* Send text.
		* Sends raw text through the session. Usually, this should be well-formed XML.
		*/
	       Session& operator<<(const char* buffer) { evtTransmitXML(buffer); return *this;}
	  public:
	       // Connectivity ops
	       /**
		* Log into a server and authenticate.
		* @see AuthType
		* @param server The server to connect to.
		* @param atype The authentication type.
		* @param username The username to use.
		* @param resource The resource of this session.
		* @param password The password for this username.
		* @param createuser Whether to attempt to create a new user.
		*/
	       void connect(const string& server, AuthType atype, 
			    const string& username, const string& resource, const string& password, 
			    bool createuser = false);

	       /**
		* Disconnect from the server.
		* @return Whether </stream:stream> was actually sent.
		*/
	       bool disconnect();

	       /**
		* Get the ConnectionState.
		* @return The current ConnectionState.
		*/
	       ConnectionState getConnState() { return _ConnState; }

	       // Misc ops
	       /**
		* Push raw XML to the session.
		* The socket connector should call this function to push raw XML through to Jabberoo.
		* @param data Character data to give to the session.
		* @param datasz Size of the character data to give to the session.
		*/
	       virtual void push(const char* data, int datasz);
	       /**
		* Register an iq callback.
		* The callback will be called once an iq message with the given id is received.
		* @param id The id of the iq message which was sent.
		* @param f The function to call.
		*/
	       void registerIQ(const string& id, ElementCallbackFunc f);
	       /**
		* Query a namespace on a specific JabberID.
		* The callback will be called once a response to the namespace query is received.
		* @param nspace The namespace to query.
		* @param f The function to call.
		* @param to The JabberID to query. If empty, it queries the server this Session is connected to.
		*/
	       void queryNamespace(const string& nspace, ElementCallbackFunc f, const string& to = "");

	       // Structure accessors
	       /**
		* Get the Roster.
		* @see Roster
		* @return The Roster.
		*/
	       const Roster& roster() const;
	       /**
		* Get the Roster.
		* @see Roster
		* @return The Roster.
		*/
	       Roster&       roster();

	       /**
		* Get the PresenceDB.
		* @see PresenceDB
		* @return The PresenceDB.
		*/
	       const PresenceDB& presenceDB() const;
	       /**
		* Get the PresenceDB.
		* @see PresenceDB
		* @return The PresenceDB.
		*/
	       PresenceDB&       presenceDB();

	       // Property accessors
	       /**
		* Get the AuthType which was used.
		* @return The AuthType which was used to connect.
		*/
	       AuthType      getAuthType() const;
	       /**
		* Get the username which was used.
		* @return The username which was used to connect.
		*/
	       const string& getUserName() const;

               // ID/Auth ops
	       /**
		* Get the next available id
		* Using this function ensures that ids are not repeated in the same Session.
		* @return A string containing a valid id.
		*/
	       string getNextID();
	       /**
		* Get the digest hash.
		* @return The hash of the password to be used for authentication.
		*/
	       string getDigest();

	  public:
	       // XML transmission signals
	       /**
		* This event is emitted when there is an error parsing the XML received.
		* Jabberoo catches libjudo's ParserError exception, disconnects the session,
		* and then triggers this event.
		* @param error_code The error code as defined by libjudo
		* @param error_msg The error message as defined by libjudo
		* @see XML_Error
		* @see push
		*/
	       Signal2<void, int, const string&, Marshal<void> > evtXMLParserError;
	       /**
		* This event is emitted when XML is transmitted.
		* This should be hooked up to a function in the socket connector which sends the character data.
		* @param XML The XML which is being sent.
		*/
	       Signal1<void, const char*, Marshal<void> >        evtTransmitXML;
	       /**
		* This event is emitted when XML is received.
		* @param XML The XML which was received.
		*/
	       Signal1<void, const char*, Marshal<void> >        evtRecvXML;
	       /**
		* This event is emitted when a Packet is transmitted.
		* @param p The Packet which is being sent.
		*/
	       Signal1<void, const Packet&, Marshal<void> >      evtTransmitPacket;
	       // Connectivity signals
	       /**
		* This event is emitted when the connection to the server has been established.
		* @see Session::connect()
		* @param t The stream header element which was used immediately after connection.
		*/
	       Signal1<void, const Element&, Marshal<void> >         evtConnected;
	       /**
		* This event is emitted when the connection to the server has been discontinued.
		* This will be emitted whether the connection was due to error or not.
		* @see Session::disconnect()
		*/
	       Signal0<void, Marshal<void> >                     evtDisconnected;
	       // Basic signals
	       /**
		* This event is emitted when a Message is received.
		* @see jabberoo::Message
		* @param m The Message.
		*/
	       Signal1<void, const Message&, Marshal<void> >     evtMessage;
	       /**
		* This event is emitted when an IQ element is received.
		* @see judo::Element
		* @param iq The iq element's judo::Element.
		*/ 
	       Signal1<void, const Element&, Marshal<void> >         evtIQ;
	       /**
		* This event is emitted when a Presence element which appears to be from the server 
		* is received. This probably is an error, such as specifying some invalid type.
		* @see jabberoo::Presence
		* @param p The Presence.
		*/
	       Signal1<void, const Presence&, Marshal<void> >    evtMyPresence;
	       /**
		* This event is emitted when a Presence element is received.
		* @see jabberoo::Presence
		* @param p The Presence.
		* @param previous_type The Presence::Type of the previous Presence received from this user.
		*/
	       Signal2<void, const Presence&, Presence::Type, Marshal<void> >    evtPresence;
	       /**
		* This event is emitted when a Presence subscription request is received.
		* @see jabberoo::Presence
		* @param p The Presence subscription request.
		*/
	       Signal1<void, const Presence&, Marshal<void> >    evtPresenceRequest;
	       // Misc signals
	       /**
		* This event is emitted when an unknown XML element is received.
		* @see judo::Element
		* @param t The element's judo::Element
		*/
	       Signal1<void, const Element&, Marshal<void> >         evtUnknownPacket;
	       /**
		* This event is emitted when an authorization error occurs.
		* @param errorcode The error code.
		* @param errormsg The error message.
		*/
	       Signal2<void, int, const char*, Marshal<void> >   evtAuthError;	  
	       /**
		* This event is emitted when a roster is received.
		* Note that this could be simply a roster push or the complete roster.
		* The only reason you'd attach to this is if you want notification
		* of *all* roster changes, whether or not you need to refresh the roster.
		* It probably makes more sense to use jabberoo::Roster::evtRefresh
		* @see jabberoo::Roster::evtRefresh
		*/
	       Signal0<void, Marshal<void> > evtOnRoster;
	       /**
		* This event is emitted when a client version is requested.
		* @param name The name of the client.
		* @param version The version of the client.
		* @param os The operating system the client is running on.
		*/
	       Signal3<void, string&, string&, string&, Marshal<void> > evtOnVersion;
	       /**
		* This event is emitted when idle time is requested.
		* @param seconds The number of seconds, as a string, of idleness.
		*/
	       Signal1<void, string&, Marshal<void> >            evtOnLast;

	       /**
		* This event is emitted when the client machine's local time is requested.
		* @param localUTF8Time Local time, given in a nice human-readable string.
		* @param UTF8TimeZone Client's time zone
		*/
	       Signal2<void, string&, string&, Marshal<void> >            evtOnTime;
	  protected:
	       // Authentication handler
	       void authenticate();
	       void OnAuthTypeReceived(const Element& t);
	       void sendLogin(Session::AuthType atype, const Element* squery);

	       // ElementStream events
	       virtual void onDocumentStart(Element* t);
	       virtual void onElement(Element* t);
	       virtual void onCDATA(CDATA* c);
	       virtual void onDocumentEnd();

	       // Basic packet handlers
	       void handleMessage(Element& t);
	       void handlePresence(Element& t);
	       void handleIQ(Element& t);

	  private:
	       // Values
	       string          _ServerID;
	       string          _Username;
	       string          _Password;
	       string          _Resource;
	       string          _SessionID;
	       long            _ID;
	       // Internal roster & presence db structures
	       Roster          _Roster;
	       PresenceDB      _PDB;
	       // Enumeration values (states)
	       AuthType        _AuthType;
	       ConnectionState _ConnState;
 	       // Stream header 
	       Element*            _StreamElement;
	       // Wether or not we have received the starting stream tag
	       bool            _StreamStart;
	       // Structures
	       multimap<string, ElementCallbackFunc> _Callbacks;			 /* IQ callback funcs */
	       // Internal IQ handlers
	       void IQHandler_Auth(const Element& t);
	       void IQHandler_CreateUser(const Element& t);
	  };    

     /**
      * JabberIDs consist of a username, a server (host), and a resource.
      * This class provides functionality for obtaining the strings of the pieces.
      * Currently the format is user@server/resource.
      */
     class JID 
	  {
	  public:
	       /**
		* Get the resource part of a JabberID.
		* @return the resource.
		*/
	       static string getResource(const string& jid);
	       /**
		* Get the username and the server (host) from a JabberID.
		* @return The username and server in the form user@server
		*/
	       static string getUserHost(const string& jid);	    
	       /**
		* Get the server from a JabberID.
		* @return The server (host).
		*/
	       static string getHost(const string& jid);
	       /**
		* Get the username from a JabberID.
		* @return The username.
		*/
	       static string getUser(const string& jid);
	       /**
		* Determine whether the username is valid.
		* @return true if the username is valid.
		*/
	       static bool isValidUser(const string& user);
	       /**
		* Determine whether the hostname is valid.
		* @return true if the hostname is valid.
		*/
	       static bool isValidHost(const string& host);
	       /**
		* Compare two JabberIDs.
		* Since the username and server (host) are not case sensitive, but the
		* resource is, this function is available to compare JabberIDs.
		* @return The result of a compare, similar to strcompare
		*/
	       static int compare(const string& ljid, const string& rjid);

	       /**
		* Another way to use the JID::compare function.
		* @see JID::compare
		*/
	       struct Compare {
		    /**
		     * Another way to use the JID::compare function.
		     * @return Whether the JabberIDs are equivalent.
		     */
		    bool operator()(const string& lhs, const string& rhs) const;
	       };
	  };

}

#ifdef WIN32
     char* shahash(const char* str);
#else
extern "C" {
     char* shahash(const char* str);
}
#endif


#endif
