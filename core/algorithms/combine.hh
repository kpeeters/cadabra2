
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class combine : public Algorithm {
		public:
			combine(const Kernel&, Ex&);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		private:
			typedef std::map<nset_t::iterator, iterator> indexlocmap_t;

			indexlocmap_t iloc;
		};

	}
