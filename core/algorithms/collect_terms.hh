
#pragma once

#include "Algorithm.hh"

/// \ingroup algorithms
///
/// Collect symbolically equal terms in a sum.

class collect_terms : public Algorithm {
	public:
		collect_terms(Kernel&, exptree&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		void  fill_hash_map(iterator);
		void  fill_hash_map(sibling_iterator, sibling_iterator);
	private:
		result_t collect_from_hash_map();
		void remove_zeroed_terms(sibling_iterator, sibling_iterator);

		typedef std::multimap<hashval_t, sibling_iterator> term_hash_t;
		typedef term_hash_t::iterator                      term_hash_iterator_t;

		term_hash_t term_hash;
};

