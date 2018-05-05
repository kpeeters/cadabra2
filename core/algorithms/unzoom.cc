
#include "Cleanup.hh"
#include "algorithms/unzoom.hh"
#include "algorithms/substitute.hh"

using namespace cadabra;

unzoom::unzoom(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	traverse_ldots=true;
	}

bool unzoom::can_apply(iterator it) 
	{
	if(*it->name=="\\ldots")  return true;
	return false;
	}

Algorithm::result_t unzoom::apply(iterator& it)
	{
	it=tr.flatten_and_erase(it);
	return result_t::l_applied;
	}

