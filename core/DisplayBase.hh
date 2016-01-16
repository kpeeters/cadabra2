
#pragma once

#include "Props.hh"
#include "Storage.hh"

/// \ingroup display
///
/// Base class for all display classes.  A key difficulty with
/// printing is to figure out when to print additional brackets for
/// objects which would otherwise not render correctly. For example, a
/// sum inside a product need brackets, but it does not need brackets
/// when it is given as an argument to a function, because then the
/// brackets are already there from the function call.

class DisplayBase {
	public:
		DisplayBase(const Properties&, const Ex&);

		void output(std::ostream&);
		void output(std::ostream&, Ex::iterator);

		virtual void dispatch(std::ostream&, Ex::iterator)=0;

	protected:
		/// Determine if a node needs extra brackets around it. Uses context from the
		/// parent node if necessary.
		bool needs_brackets(Ex::iterator it);

		const Ex&    tree;
		const Properties& properties;

};
