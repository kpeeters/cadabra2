
#pragma once

#include "Algorithm.hh"

class flatten_product : public algorithm {
	public:
		flatten_product(exptree&, iterator);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		bool make_consistent_only, is_diff;
};

