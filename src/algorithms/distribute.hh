
#pragma once

#include "Algorithm.hh"

class distribute : public algorithm {
	public:
		distribute(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};

