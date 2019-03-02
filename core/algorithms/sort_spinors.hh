
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class sort_spinors : public Algorithm {
		public:
			sort_spinors(const Kernel&, Ex&);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

			sibling_iterator one, gammamat, two;
		};

	}


