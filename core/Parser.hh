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

namespace cadabra {

   /// \ingroup core
   ///
   /// Class which turns the string output of a `preprocessor`
   /// object and turns it into an Ex expression tree. The output of
   /// `preprocessor` is assumed to be valid and consistent, so the
   /// code here is rather simple.

	class Parser {
		public:
			Parser();
			Parser(std::shared_ptr<Ex>);
			Parser(std::shared_ptr<Ex>, const std::string&);

			void erase();

			void remove_empty_nodes();

			/// Finalise the parsed expression. This function should be
			/// called when no further operator>> calls are going to be made,
			/// and is necessary to ensure that the tree is consistent.
			void finalise();
			bool string2tree(const std::string& inp);

			std::shared_ptr<Ex> tree;
		private:
			Ex::iterator   parts;
			std::u32string str;

			enum mode_t { m_skipwhite=0, m_name=1, m_findchildren=2,
			              m_singlecharname=3, m_backslashname=4,
			              m_childgroup=5, m_initialgroup=6, m_verbatim=7, m_property=8
			};

			void                   advance(unsigned int& i);
			char32_t               get_token(unsigned int i);
			bool                   is_number(const std::u32string& str) const;
			str_node::bracket_t    is_closing_bracket(const char32_t& br) const;
			str_node::bracket_t    is_opening_bracket(const char32_t& br) const;
			str_node::parent_rel_t is_link(const char32_t& ln) const;

			std::vector<mode_t>                 current_mode;
			std::vector<str_node::bracket_t>    current_bracket;
			std::vector<str_node::parent_rel_t> current_parent_rel;
		};

	}

std::istream& operator>>(std::istream&, cadabra::Parser&);


//std::ostream& operator<<(std::ostream&, Parser&);
