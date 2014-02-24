
#pragma once

#include "Algorithm.hh"

class distribute : public Algorithm {
	public:
		distribute(Kernel&, exptree&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

