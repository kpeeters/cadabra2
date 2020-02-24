
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class component : public Algorithm {
	public:
		component(const Kernel&, Ex& ex, Ex& rules);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
	};

}
