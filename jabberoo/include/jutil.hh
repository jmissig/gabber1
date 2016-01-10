/********************************************************************************
 *   Jabberoo/Judo -- C++ Jabber Library                                        *
 *                                                                              * 
 *   Copyright (C) 1999-2000 Dave Smith (dave@jabber.org)                       *
 *                                                                              *
 *   This library is free software; you can redistribute it and/or              *
 *   modify it under the terms of the GNU Lesser General Public                 *
 *   License as published by the Free Software Foundation; either               *
 *   version 2.1 of the License, or (at your option) any later version.         *
 *                                                                              *
 *   This library is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU          *
 *   Lesser General Public License for more details.                            *
 *                                                                              *
 *   You should have received a copy of the GNU Lesser General Public           *
 *   License along with this library; if not, write to the Free Software        *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  *
 ********************************************************************************
 */
#ifndef INCL_JUTIL_H
#define INCL_JUTIL_H

#include <iterator>
#include <string>
using namespace std;

namespace jutil
{
     template<class Iter, class Value>
     class ValueIterator : public Iter {
     public:
	  // Iterator traits (req. for use w/ std::algos)
	  typedef bidirectional_iterator_tag iterator_category;
	  typedef Value                      value_type;
	  typedef ptrdiff_t                  difference_type;
	  typedef value_type*                pointer;
	  typedef value_type&                reference;

	  ValueIterator() : Iter() {}
	  ValueIterator(const Iter& i) 
	       : Iter(i) {}
	  ValueIterator(const ValueIterator& i) 
	       : Iter(i) {}

	  reference operator*() const
	       {
		    return Iter::operator*().second;
	       }

	  pointer operator->() const
	       {
		    return &(Iter::operator*().second);
	       }

     };

     string getTimeStamp();

     struct CaseInsensitiveCmp {
	  bool operator()(const string& lhs, const string& rhs) const;
     };
}

#endif
