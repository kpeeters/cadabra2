
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include <ostream>

// Class to handle display of expressions using LaTeX notation.

typedef uint32_t kunichar;

class DisplayTeX {
	public:
		DisplayTeX(const Properties&, const Ex&);

		void output(std::ostream&);
		void output(std::ostream&, Ex::iterator);

	private:
		void print_multiplier(std::ostream&, Ex::iterator);
		void print_opening_bracket(std::ostream&, str_node::bracket_t, str_node::parent_rel_t);
		void print_closing_bracket(std::ostream&, str_node::bracket_t, str_node::parent_rel_t);
		void print_parent_rel(std::ostream&, str_node::parent_rel_t, bool first);
		void print_children(std::ostream&, Ex::iterator, int skip=0);

		std::string texify(const std::string&) const;

		const Ex&    tree;
		const Properties& properties;

		bool print_star=false;
		bool tight_star=false;
		bool tight_plus=false;
		bool utf8_output=false;
		bool latex_spacing=true;
		bool latex_linefeeds=true;             // to prevent buffer overflows in TeX
		bool extra_brackets_for_symbols=false; // wrap extra {} around symbols to ensure typesetting safety
		
		int bracket_level=0;

		void dispatch(std::ostream&, Ex::iterator);
		void print_productlike(std::ostream&, Ex::iterator, const std::string& inbetween);
		void print_sumlike(std::ostream&, Ex::iterator);
		void print_fraclike(std::ostream&, Ex::iterator);
		void print_commalike(std::ostream&, Ex::iterator);
		void print_arrowlike(std::ostream&, Ex::iterator);
		void print_powlike(std::ostream&, Ex::iterator);

		bool children_have_brackets(Ex::iterator ch) const;
};

const char *unichar(kunichar c);
