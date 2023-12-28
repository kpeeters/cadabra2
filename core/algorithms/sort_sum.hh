
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class sort_sum : public Algorithm {
		public:
			sort_sum(const Kernel&, Ex&, unsigned int runsize = 32);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		private:
			bool should_swap(iterator obj, int subtree_comparison) const;
			bool should_swap(iterator obj1, iterator obj2, int subtree_comparison) const;
			unsigned int runsize;
			bool insertionSort(sibling_iterator, unsigned int);
			bool timSort(iterator&);
			bool merge(sibling_iterator, unsigned int, unsigned int, unsigned int);
			unsigned int calcMinRun(unsigned int);



		};

	}
