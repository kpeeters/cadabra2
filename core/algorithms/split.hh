
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	/// \ingroup algorithms
	///
	/// Split an expression into terms or factors.


	class split : public Algorithm {
	public:
		split(const Kernel&, Ex&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
	};

}
