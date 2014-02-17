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

class Ex {
	public:
		Ex(const Ex&); 
		Ex(std::string);
		std::string get() const;
		void append(std::string);
		std::string to_string() const;

		Ex& operator=(const Ex&);

	private:
		std::string ex;
		exptree     tree;
};
