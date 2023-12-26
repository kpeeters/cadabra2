
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class sort_sum : public Algorithm {
		public:
			sort_sum(const Kernel&, Ex&, int algochoice = 0);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		private:
			bool should_swap(iterator obj, int subtree_comparison) const;
			bool should_swap(iterator obj1, iterator obj2, int subtree_comparison) const;
			int algochoice;
			bool insertionSort(sibling_iterator, unsigned int);
			bool timSort(iterator&);
			bool merge(sibling_iterator, unsigned int, unsigned int, unsigned int);
			unsigned int calcMinRun(unsigned int);



		};

	}
