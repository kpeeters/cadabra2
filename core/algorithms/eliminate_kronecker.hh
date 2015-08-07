
#pragma once

#include "Algorithm.hh"

class eliminate_kronecker : public Algorithm {
	public:
		eliminate_kronecker(Kernel&, Ex&);
		
		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
};

