
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	/// \ingroup algorithms
	///
	/// Transform a (system of) higher order ODEs to first order, and ensure that
	/// the derivatives are on the lhs with all the other terms on the rhs.

	class first_order_form : public Algorithm {
		public:
			first_order_form(const Kernel&, Ex& odes, Ex& funcs);

			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);

		private:
			Ex functions;

			// Location at which the above functions appear...
			std::vector<Ex::iterator> locations;
			// ... and the order of the derivative on these functions.
			std::vector<int>          orders;
	};
	
}
