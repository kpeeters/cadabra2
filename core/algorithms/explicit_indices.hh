
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class explicit_indices : public Algorithm {
		public:
			explicit_indices(const Kernel&, Ex&);

			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);

		private:
			index_map_t                             ind_free_sum, ind_dummy_sum;
			index_map_t                             added_this_term;
			std::map<const Indices *, Ex::iterator> index_lines, first_index, last_index;         // for the current term

			void handle_factor(sibling_iterator& factor, bool trace_it);
		};

	}
