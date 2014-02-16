
#pragma once

#include "Algorithm.hh"

class prodcollectnum : public algorithm {
	public:
		prodcollectnum(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

