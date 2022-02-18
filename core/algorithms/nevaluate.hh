
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class nevaluate : public Algorithm {
		public:
			nevaluate(const Kernel&, Ex&);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;
	};

}
