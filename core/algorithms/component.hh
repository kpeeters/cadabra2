
#pragma once

#include "Algorithm.hh"

class component : public Algorithm {
	public:
		component(Kernel&, exptree& ex, exptree& rules);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};
