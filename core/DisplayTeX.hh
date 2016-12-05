
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include "DisplayBase.hh"
#include <ostream>

typedef uint32_t kunichar;

/// \ingroup display
///
/// Class to handle display of expressions using LaTeX notation. This
/// is a very non-local bit of logic, in the sense that it will try to
/// look around the property assignments in order to figure out the
/// best way to print any given object. See the DisplayTeX::dispatch
/// method to see how this works in more detail.

class DisplayTeX : public DisplayBase {
	public:
		DisplayTeX(const Kernel&, const Ex&);

	protected:
		virtual bool needs_brackets(Ex::iterator it) override;

	private:
		// FIXME: just make this take the multiplier, instead of the node. Then mult can go away.
		void print_multiplier(std::ostream&, Ex::iterator, int mult=1);
		void print_opening_bracket(std::ostream&, str_node::bracket_t, str_node::parent_rel_t);
		void print_closing_bracket(std::ostream&, str_node::bracket_t, str_node::parent_rel_t);
		void print_parent_rel(std::ostream&, str_node::parent_rel_t, bool first);
		void print_children(std::ostream&, Ex::iterator, int skip=0);

		std::string texify(std::string) const;

		bool print_star=false;
		bool tight_star=false;
		bool tight_plus=false;
		bool utf8_output=false;
		bool latex_spacing=true;
		bool latex_linefeeds=true;             // to prevent buffer overflows in TeX
		bool extra_brackets_for_symbols=false; // wrap extra {} around symbols to ensure typesetting safety
		
		int bracket_level=0;

		/// For every object encountered, dispatch will figure out the
		/// most appropriate way to convert it into a LaTeX
		/// expression. This may be done by simply looking at the
		/// object's name (e.g. \prod will print as a product) but may
		/// also involve looking up properties and deciding on the best
		/// course of action based on the attached properties.

		virtual void dispatch(std::ostream&, Ex::iterator) override;

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
		void print_commutator(std::ostream& str, Ex::iterator it, bool comm);
		void print_components(std::ostream&, Ex::iterator);
		void print_conditional(std::ostream&, Ex::iterator);
		void print_relation(std::ostream&, Ex::iterator);
		void print_indexbracket(std::ostream&, Ex::iterator);
		void print_other(std::ostream& str, Ex::iterator it);

		bool children_have_brackets(Ex::iterator ch) const;

		/// Determine whether the indicated object 'obj', when acting on
		/// the single argument 'arg', reads as an operator, and therefore
		/// 'arg' does not need to be wrapped in brackets. 
		/// Example: the tree \partial{A} can be displayed as '\partial A'
		/// (or rather, '\partial{A}', instead of '\partial(A)'. 
	  
		bool reads_as_operator(Ex::iterator obj, Ex::iterator arg) const;

		/// Map from Cadabra symbols to LaTeX symbols (for some typographic cleanup).
		std::map<std::string, std::string> symmap;

		/// List of operators which need all arguments fed with curly brackets, not 
		/// round ones (e.g. \sqrt).
		std::set<std::string> curly_bracket_operators;

};

const char *unichar(kunichar c);
