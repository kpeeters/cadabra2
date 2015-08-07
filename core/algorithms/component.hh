
#pragma once

#include "Algorithm.hh"

class component : public Algorithm {
	public:
		component(Kernel&, Ex& ex, Ex& rules);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};
