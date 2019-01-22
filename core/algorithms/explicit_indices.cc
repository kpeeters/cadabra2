
#include "explicit_indices.hh"

#include "properties/ImplicitIndex.hh"

using namespace cadabra;

explicit_indices::explicit_indices(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool explicit_indices::can_apply(iterator st)
	{
	// Work on equals nodes, or single terms (not terms in a sum) or 
	// sums, provided the latter are not lhs or rhs of an equals node.
	// All this because when we generate free indices, we need to
	// ensure that all terms and all sides of an equals node use the
	// same index names.
	
	return false;
	}

Algorithm::result_t explicit_indices::apply(iterator& prod)
	{


	return result_t::l_applied;
	}
