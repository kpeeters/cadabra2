
#pragma once

#include "Algorithm.hh"

namespace cadabra {

/// \ingroup algorithms
///
/// Collect symbolically equal factors in a product.

class collect_factors : public Algorithm {
	public:
		collect_factors(const Kernel&, Ex&);
		
		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
	private:
		void fill_hash_map(iterator);

		typedef std::multimap<hashval_t, sibling_iterator> factor_hash_t;
		typedef factor_hash_t::iterator                    factor_hash_iterator_t;

		factor_hash_t factor_hash;
};

}
