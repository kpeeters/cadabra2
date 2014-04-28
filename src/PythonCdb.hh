/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2014  Kasper Peeters <kasper.peeters@phi-sci.com>

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/


#pragma once

#include <boost/python.hpp>
#include <stdexcept>
#include "Storage.hh"

// Ex is essentially a wrapper around an exptree object, with additional
// functionality make it print nice in Python. It also contains logic
// to replace '@(abc)' nodes in the tree with the Python 'abc' expression.

class Ex {
	public:
		Ex(const Ex&); 
		Ex(std::string);
		std::string get() const;
		void append(std::string);
		std::string str_() const;
		std::string repr_() const;

		Ex& operator=(const Ex&);

		exptree     tree;

	private:
		std::string ex;

		// Pull in any '@(...)' expressions from the Python side.
		void pull_in();
		// Set '_' equal to this object.
		void register_as_last_expression();
		Ex *fetch_from_python(std::string nm);
};

// Property is a templated wrapper around a C++ property object.

class BaseProperty {
	public:
		BaseProperty(const std::string&);
		
		std::string str_() const;
		std::string repr_() const;

		std::string creation_message;
};

template<class T>
class Property : public BaseProperty {
	public:
		Property(Ex *);
};

