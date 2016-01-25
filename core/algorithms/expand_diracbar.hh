
#pragma once

#include "Algorithm.hh"

class expand_diracbar : public Algorithm {
	public:
		expand_diracbar(Kernel&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;		
};

