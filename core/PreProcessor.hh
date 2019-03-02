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

#include <iostream>
#include <string>
#include <vector>

enum { tok_arrow=0xac, tok_unequals=0xad, tok_wedge=0xae, tok_pow=0xaf, tok_set_option=0xb0, tok_declare=0xb1, tok_sequence=0xb2, tok_siblings=0xb3 };

/// \ingroup core
///
/// Preprocessing class which takes infix mathematical notation with all sorts of
/// maths shortcuts and transforms it into a string which is properly formatted
/// in prefix notation. Use the Parser class to turn this string into an Ex expression
/// tree.

class preprocessor {
	public:
		preprocessor();
		friend std::istream& operator>>(std::istream&, preprocessor&);
		friend std::ostream& operator<<(std::ostream&, const preprocessor&);

		void erase();
		void strip_outer_brackets() const;

		const static unsigned char  orders[];

		enum order_labels { order_factorial=0,
		                    order_pow,
		                    order_frac,
		                    order_prod,
		                    order_wedge,
		                    order_minus,
		                    order_plus,
		                    order_dot,
		                    order_equals,
		                    order_unequals,
		                    order_less_than,
		                    order_greater_than,
		                    order_conditions,
		                    order_arrow,
		                    order_set_option,
		                    order_colon,
		                    order_comma,
		                    order_tilde
		                  	};
		// FIXME: we really need a way to associate multiple characters to a single operator,
		// since that would allow for ".." (sequence), ":=" (define), ">=" and so on. The current
		// '.' is a hack and is treated as such: when it occurs there is an additional check for
		// a followup '.'.
		const static char *const    order_names[];
	private:
		void parse_(const std::string&);
		void parse_internal_();
		bool verbatim_;
		bool next_is_product_;
		bool eat_initial_whitespace_;
		bool unwind_(unsigned int tolevel, unsigned int bracketgoal=0, bool usebracket=true) const;
		unsigned char get_token_(unsigned char prev_token);
		void show_and_throw_(const std::string& str) const;

		void         bracket_strings_(unsigned int cb, std::string& obrack, std::string& cbrack) const;
		bool         is_infix_operator_(unsigned char c) const;
		bool         is_link_(unsigned char c) const;
		unsigned int is_opening_bracket_(unsigned char c) const;
		unsigned int is_closing_bracket_(unsigned char c) const;
		unsigned int is_bracket_(unsigned char c) const;
		bool         is_already_bracketed_(const std::string& str) const;
		bool         is_digits_(const std::string& str) const;
		unsigned int current_bracket_(bool deep=false) const;
		void         print_stack() const; // for debuggging purposes

		bool default_is_product_() const;
		unsigned int cur_pos;
		std::string  cur_str;

		// A backslash followed by a bracket is also a bracket (gets code
		// of the bracket plus 128).
		const static unsigned char open_brackets[];
		const static unsigned char close_brackets[];

		class accu_t {
			public:
				accu_t();
				void erase();

				bool                     head_is_generated;  // when infix -> postfix has occurred
				std::string              accu;
				unsigned int             order;
				std::vector<std::string> parts;
				unsigned int             bracket;
				bool                     is_index; // whether the bracket was prefixed with ^ or _
			};
		mutable accu_t               cur;
		mutable std::vector<accu_t>  accus;
	};

std::ostream& operator<<(std::ostream&, const preprocessor&);
std::istream& operator>>(std::istream&, preprocessor&);


