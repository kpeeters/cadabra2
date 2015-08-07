
#pragma once

#include "Algorithm.hh"

class prodcollectnum : public Algorithm {
	public:
		prodcollectnum(Kernel&, Ex&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

