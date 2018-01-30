
#pragma once

#include "DisplayBase.hh"
#include <ostream>
#include <map>
#include <set>

typedef uint32_t kunichar;

namespace cadabra {

/// \ingroup display
///
/// Class to display expressions in a format that Mathematica can
/// parse. Will throw an exception if a Cadabra Ex object cannot be
/// understood by Mathematica. Can also convert expressions back to
/// Cadabra notation. 

class DisplayMMA : public DisplayBase {
	public:
		DisplayMMA(const Kernel&, const Ex&, bool use_unicode);

      /// Rewrite the output of mathematica back into a notation used by
      /// Cadabra. This in particular involves converting 'Sin' and
      /// friends to '\sin' and so on, as well as converting all the
      /// greek symbols.  Currently only acts node-by-node, does not
      /// do anything complicated with trees.

		void import(Ex&);

		std::string preparse_import(const std::string&);

	protected:
		bool use_unicode;
		
		virtual bool needs_brackets(Ex::iterator it) override;

	private:
		/// Output the expression to a mathematica-readable form. For symbols
		/// which cannot be parsed by mathematica, this can convert to an
		/// alternative, Such rewrites can then be undone by the
		/// 'import' member above.

		void print_multiplier(std::ostream&, Ex::iterator);
		void print_opening_bracket(std::ostream&, str_node::bracket_t);
		void print_closing_bracket(std::ostream&, str_node::bracket_t);
		void print_parent_rel(std::ostream&, str_node::parent_rel_t, bool first);
		void print_children(std::ostream&, Ex::iterator, int skip=0);


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
		void print_components(std::ostream&, Ex::iterator);
		void print_partial(std::ostream& str, Ex::iterator it);
		void print_matrix(std::ostream& str, Ex::iterator it);
		void print_other(std::ostream& str, Ex::iterator it);

		bool children_have_brackets(Ex::iterator ch) const;

		/// Map from Cadabra symbols to Mathematica symbols.
		/// This is a bit tricky because MathLink does not pass
		/// \[Alpha] and friends transparently. So we feed it UTF8
		/// α and so on, but then we get \[Alpha] back, and that
		/// needs to be regex-replaced before we feed it to our
		/// parser as ours does not swallow that kind of bracketing.
		std::map<std::string, std::string>      symmap;
		std::multimap<std::string, std::string> regex_map;		

		/// Map from symbols which have had dependencies added
		/// to an expression containing these dependencies.
		std::map<nset_t::iterator, Ex, nset_it_less> depsyms;
};

const char *unichar(kunichar c);

}
