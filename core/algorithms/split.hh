
#pragma once

#include "Algorithm.hh"

/// \ingroup algorithms
///
/// Split an expression into terms or factors.


class split : public Algorithm {
	public:
		split(Kernel&, Ex&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

