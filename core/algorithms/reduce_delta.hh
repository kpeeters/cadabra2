
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class reduce_delta : public Algorithm {
		public:
			reduce_delta(const Kernel&, Ex&);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		private:
			bool one_step_(sibling_iterator);
		};

	}
