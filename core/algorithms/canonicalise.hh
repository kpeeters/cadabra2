#pragma once

#include "Algorithm.hh"
#include "properties/Indices.hh"
#include <vector>

namespace cadabra {

	/// \ingroup algorithms
	///
	/// Canonicalise the index structure of a tensorial expression.

	class canonicalise : public Algorithm {
		public:
			canonicalise(const Kernel&, Ex&);

			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);

			std::vector<std::vector<int> > generating_set;
			bool                           reuse_generating_set;

		private:
			// Sub-algorithms needed before going to the full fledged canonicaliser.
			// All return true if they have modified the expression.
			bool remove_traceless_traces(iterator&);
			bool remove_vanishing_numericals(iterator&);
			bool only_one_on_derivative(iterator index1, iterator index2) const;

			Indices::position_t  position_type(iterator) const;
			//		void collect_dummy_info(const index_map_t&, const index_position_map_t&,
			//										std::vector<int>&, std::vector<int>&);
		};

	}
