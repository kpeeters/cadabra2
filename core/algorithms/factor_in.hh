
#pragma once

#include "Algorithm.hh"

namespace cadabra {

/// \ingroup algorithms

class factor_in : public Algorithm {
	public:
		factor_in(const Kernel&, Ex&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	protected:
	   Ex& factors;
		std::set<Ex, tree_exact_less_for_indexmap_obj> factnodes; // objects to be taken in brackets;  
                                             // FIXME: use patterns
		bool      compare_restricted(iterator one, iterator two) const;
		bool      compare_prod_nonprod(iterator prod, iterator nonprod) const;

		// Calculate the hash value excluding factors given in the argument list
		hashval_t calc_restricted_hash(iterator it) const;
		void      fill_hash_map(iterator);

		typedef std::multimap<hashval_t, Ex::sibling_iterator> term_hash_t;
		typedef term_hash_t::iterator                          term_hash_iterator_t;

		term_hash_t term_hash;
};

}
