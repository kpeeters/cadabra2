
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class lower_free_indices : public Algorithm {
		public:
			lower_free_indices(const Kernel&, Ex&, bool lower);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		private:
			bool lower;
		};

	}
