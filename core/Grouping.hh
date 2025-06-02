
#pragma once

#include "Storage.hh"
#include <functional>

namespace cadabra {

	/// \ingroup algorithms
	///
	/// Given an ExNode iterator, find all expressions in its
	/// range which are equal up to a numerical multiplier.
	/// Return a structure which summarises these equivalencies,

	typedef std::map<Ex::iterator, std::pair<multiplier_t, Ex::sibling_iterator>, Ex::iterator_base_less> equiv_map_t;
	typedef std::function<bool(const Ex&, Ex::iterator, Ex::iterator)> equiv_fun_t;
	
	equiv_map_t group_by_equivalence(const Ex&, Ex::sibling_iterator first, Ex::sibling_iterator last);
	equiv_map_t group_by_equivalence(const Ex&, Ex::iterator comma_top);

	equiv_map_t group_by_equivalence(const Ex&, Ex::sibling_iterator first, Ex::sibling_iterator last, equiv_fun_t&);
	equiv_map_t group_by_equivalence(const Ex&, Ex::iterator comma_top, equiv_fun_t&);

}
