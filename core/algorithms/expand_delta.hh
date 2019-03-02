
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class expand_delta : public Algorithm {
		public:
			expand_delta(const Kernel&, Ex&);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;
		};

	}
