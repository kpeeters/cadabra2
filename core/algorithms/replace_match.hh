
#pragma once

#include "Algorithm.hh"

class replace_match : public Algorithm {
	public:
		replace_match(const Kernel&, Ex&);
		
		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
};

