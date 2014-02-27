
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include <ostream>

// Class to handle display of expressions using LaTeX notation.

typedef uint32_t kunichar;

class DisplayTeX {
	public:
		DisplayTeX(const Properties&, const exptree&);

		void output(std::ostream&);
		void output(std::ostream&, exptree::iterator);

	private:
		str_node::parent_rel_t previous_parent_rel_, current_parent_rel_;
		str_node::bracket_t    previous_bracket_, current_bracket_;

		void print_multiplier(std::ostream&, exptree::iterator);
		void print_opening_bracket(std::ostream&, str_node::bracket_t, str_node::parent_rel_t);
		void print_closing_bracket(std::ostream&, str_node::bracket_t, str_node::parent_rel_t);
		void print_parent_rel(std::ostream&, str_node::parent_rel_t, bool first);
		void print_children(std::ostream&, exptree::iterator, int skip=0);

		std::string texify(const std::string&) const;

		const exptree&    tree;
		const Properties& properties;

		bool print_star=false;
		bool tight_star=false;
		bool tight_plus=false;
		bool utf8_output=false;
		bool latex_spacing=true;
		bool latex_linefeeds=true;             // to prevent buffer overflows in TeX
		bool extra_brackets_for_symbols=false; // wrap extra {} around symbols to ensure typesetting safety
		
		int bracket_level=0;

		void dispatch(std::ostream&, exptree::iterator);
		void print_productlike(std::ostream&, exptree::iterator, const std::string& inbetween);
		void print_sumlike(std::ostream&, exptree::iterator);

		bool children_have_brackets(exptree::iterator ch) const;
};

const char *unichar(kunichar c);
