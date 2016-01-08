
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include <ostream>
#include <map>

typedef uint32_t kunichar;

/// \ingroup display
///
/// Class to display expressions to the terminal. This is used in the
/// PythonCdb bridge to provide 'str' type python printing for Ex
/// objects. The cadabra2 command line application uses this class for
/// all its output. This class does not necessarily produce output which
/// is readable by sympy; for that, use the DisplaySympy class (as an 
/// example, partial derivatives will display using a LaTeX notation using
/// the present class, but be printed as 'sympy.diff' by the DisplaySympy
/// class).

class DisplayTerminal {
	public:
		DisplayTerminal(const Properties&, const Ex&);

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
		void print_components(std::ostream&, Ex::iterator);

		bool children_have_brackets(Ex::iterator ch) const;

		std::map<std::string, std::string> symmap;
};

const char *unichar(kunichar c);
