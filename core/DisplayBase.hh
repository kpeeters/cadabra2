
#pragma once

#include "Props.hh"
#include "Storage.hh"

namespace cadabra {

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
		DisplayBase(const Kernel&, const Ex&);

		void output(std::ostream&);
		void output(std::ostream&, Ex::iterator);

		virtual void dispatch(std::ostream&, Ex::iterator)=0;

	protected:
		/// Determine if a node needs extra brackets around it. Uses context from the
		/// parent node if necessary. Has to be implemented in a derived class, because
      /// the answer depends on the printing method (e.g. `(a+b)/c` needs brackets
		/// when printed like this, but does not need brackets when printed as
		/// `\frac{a+b}{c}`).

		virtual bool needs_brackets(Ex::iterator it)=0;

		const Ex&     tree;
		const Kernel& kernel;

};

}
