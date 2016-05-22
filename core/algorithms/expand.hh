
#pragma once

#include "Algorithm.hh"

class expand : public Algorithm {
	public:
		expand(const Kernel&, Ex&);
		
		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	private:
		iterator mx_first, mx_last, ii_first, ii_last;
		bool     one_index;
};

