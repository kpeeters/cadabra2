
#pragma once

#include <set>
#include "Algorithm.hh"

namespace cadabra {

	class expand_dummies : public Algorithm {
		public:
			expand_dummies(const Kernel& kernel, Ex& ex, const Ex* components = nullptr, bool zero_missing_components = true);

			virtual bool can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		private:
			void enumerate_patterns();
			void fill_components(Ex::iterator it);

			Ex_comparator comp;
			const Ex* components;
			std::vector<Ex> component_patterns;
			bool zero_missing_components;
		};

	}
