
#pragma once

#include "Algorithm.hh"

namespace cadabra {

class explicit_indices : public Algorithm {
	public:
		explicit_indices(const Kernel&, Ex&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

}
