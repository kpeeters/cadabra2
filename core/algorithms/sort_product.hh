
#pragma once

#include "Algorithm.hh"

class sort_product : public Algorithm {
	public:
		sort_product(Kernel&, Ex&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

	private:
		bool ignore_numbers_;
};

