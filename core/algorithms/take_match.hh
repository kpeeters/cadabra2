
#pragma once

#include "Algorithm.hh"

class take_match : public Algorithm {
	public:
		take_match(const Kernel& k, Ex& e, Ex& r);
		
		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
		
	private:
		Ex& rules;
};

