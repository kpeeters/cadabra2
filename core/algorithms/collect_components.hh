
#pragma once

#include "Algorithm.hh"

namespace cadabra {

/// \ingroup algorithms
///
/// Collect \components terms inside a sum, merging their substitution rules.

class collect_components : public Algorithm {
	public:
		collect_components(const Kernel&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
};

}
