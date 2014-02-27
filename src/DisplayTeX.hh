
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include <ostream>

// Class to handle display of expressions using LaTeX notation.

class DisplayTeX {
	public:
		DisplayTeX(const Properties&, exptree&);

		void output(std::ostream&);

	private:
		str_node::parent_rel_t previous_parent_rel_, current_parent_rel_;
		str_node::bracket_t    previous_bracket_, current_bracket_;

		void print_multiplier(std::ostream&, exptree::iterator);
		void print_opening_bracket(std::ostream&, str_node::bracket_t, str_node::parent_rel_t);
		void print_closing_bracket(std::ostream&, str_node::bracket_t, str_node::parent_rel_t);
		void print_parent_rel(std::ostream&, str_node::parent_rel_t, bool first);
		void print_children(std::ostream&, exptree::iterator, int skip=0);

		std::string texify(const std::string&) const;

		exptree&          tree;
		const Properties& properties;
};
