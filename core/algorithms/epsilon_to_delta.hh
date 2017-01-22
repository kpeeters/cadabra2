
#pragma once

#include "Algorithm.hh"
#include <vector>

namespace cadabra {

class epsilon_to_delta : public Algorithm {
	public:
		epsilon_to_delta(const Kernel&, Ex&, bool reduce_);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
	private:
		bool                      reduce;
		std::vector<Ex::iterator> epsilons;
		int                       signature;
		Ex                        repdelta;
};

}
