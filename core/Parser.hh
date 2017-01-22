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

#include <string>
#include <vector>
#include <iostream>

#include "Storage.hh"

/// \ingroup core
///
/// Parser module, which takes the string output of the
/// preprocessor.hh module and turns it into an Ex expression
/// tree. The output of preprocessor.hh is assumed to be
/// valid and consistent, so the code here is rather simple.



namespace cadabra {

class Parser { 
	public:
		Parser();
		Parser(std::shared_ptr<Ex>);
		Parser(std::shared_ptr<Ex>, const std::string&);		
	  
		void erase();

		void remove_empty_nodes();

		// Finalise the parsed expression. This function should be
		// called when no further operator>> calls are going to be made,
		// and is necessary to ensure that the tree is consistent.
		void finalise();
		bool string2tree(const std::string& inp);

		std::shared_ptr<Ex> tree;
	private:
		Ex::iterator parts;
		std::string       str;

		enum mode_t { m_skipwhite, m_name, m_findchildren, 
						  m_singlecharname, m_backslashname, 
						  m_childgroup, m_initialgroup, m_verbatim, m_property };

		void                   advance(unsigned int& i);
		unsigned char          get_token(unsigned int i);
		bool                   is_number(const std::string& str) const;
		str_node::bracket_t    is_closing_bracket(const unsigned char& br) const;
		str_node::bracket_t    is_opening_bracket(const unsigned char& br) const;
		str_node::parent_rel_t is_link(const unsigned char& ln) const;

		std::vector<mode_t>                 current_mode;
		std::vector<str_node::bracket_t>    current_bracket;
		std::vector<str_node::parent_rel_t> current_parent_rel;
};

}

std::istream& operator>>(std::istream&, cadabra::Parser&);


//std::ostream& operator<<(std::ostream&, Parser&);
