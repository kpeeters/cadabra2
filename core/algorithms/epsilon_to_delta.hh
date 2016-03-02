
#pragma once

#include "Algorithm.hh"
#include <vector>

class epsilon_to_delta : public Algorithm {
	public:
		epsilon_to_delta(const Kernel&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
	private:
		std::vector<Ex::iterator> epsilons;
		int                       signature;
		Ex                        repdelta;
};

