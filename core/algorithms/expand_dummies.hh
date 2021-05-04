
#pragma once

#include <set>
#include "Algorithm.hh"

namespace cadabra {

	class expand_dummies : public Algorithm {
		public:
			expand_dummies(const Kernel& kernel, Ex& ex, const Ex* components = nullptr);

			virtual bool can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

        private:
            void enumerate_patterns();
            void fill_components(Ex::iterator it);

            Ex_comparator comp;
            const Ex* components;
            std::set<Ex> component_patterns;
		};

	}
