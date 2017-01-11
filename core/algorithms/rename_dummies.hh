
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class rename_dummies : public Algorithm {
		public:
			rename_dummies(const Kernel&, Ex&);
			
			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);
	};

}
