
#pragma once

#include "Algorithm.hh"

class keep_terms : public Algorithm {
	public:
		keep_terms(Kernel&, exptree&, const std::vector<int>& terms);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

	private:
		std::vector<int> terms_;
};

