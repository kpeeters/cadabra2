
#pragma once

#include "Algorithm.hh"

class reduce_sub : public Algorithm {
	public:
		reduce_sub(Kernel&, exptree&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

