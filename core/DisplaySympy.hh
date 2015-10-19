
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include <ostream>
#include <map>

/// \ingroup core
///
/// Class to display expressions in a format that Sympy can parse.

typedef uint32_t kunichar;

class DisplaySympy {
	public:
		DisplaySympy(const Properties&, const Ex&);

		void output(std::ostream&);
		void output(std::ostream&, Ex::iterator);

	private:
		void print_multiplier(std::ostream&, Ex::iterator);
		void print_opening_bracket(std::ostream&, str_node::bracket_t, str_node::parent_rel_t);
		void print_closing_bracket(std::ostream&, str_node::bracket_t, str_node::parent_rel_t);
		void print_parent_rel(std::ostream&, str_node::parent_rel_t, bool first);
		void print_children(std::ostream&, Ex::iterator, int skip=0);

		const Ex&    tree;
		const Properties& properties;

		int bracket_level=0;

		/// For every object encountered, dispatch will figure out the
		/// most appropriate way to convert it into a LaTeX
		/// expression. This may be done by simply looking at the
		/// object's name (e.g. \prod will print as a product) but may
		/// also involve looking up properties and deciding on the best
		/// course of action based on the attached properties.

		void dispatch(std::ostream&, Ex::iterator);

		/// Printing members for various standard constructions,
		/// e.g. print as a list, or as a decorated symbol with
		/// super/subscripts etc. The names reflect the structure of the
		/// output, not necessarily the meaning or name of the object
		/// that is being printed.

		void print_productlike(std::ostream&, Ex::iterator, const std::string& inbetween);
		void print_sumlike(std::ostream&, Ex::iterator);
		void print_fraclike(std::ostream&, Ex::iterator);
		void print_commalike(std::ostream&, Ex::iterator);
		void print_arrowlike(std::ostream&, Ex::iterator);
		void print_powlike(std::ostream&, Ex::iterator);
		void print_intlike(std::ostream&, Ex::iterator);
		void print_equalitylike(std::ostream&, Ex::iterator);

		bool children_have_brackets(Ex::iterator ch) const;

		std::map<std::string, std::string> symmap;
};

const char *unichar(kunichar c);
