
#pragma once

#include "Algorithm.hh"

class expand_power : public Algorithm {
	public:
		expand_power(const Kernel&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;		
};
