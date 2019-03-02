
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class take_match : public Algorithm {
		public:
			take_match(const Kernel& k, Ex& e, Ex& r);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		private:
			Ex rules;

			std::vector<sibling_iterator> to_erase;
			std::vector<Ex::path_t>       to_keep;
		};

	}
