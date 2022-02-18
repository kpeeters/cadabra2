#include "nevaluate.hh"

using namespace cadabra;

nevaluate::nevaluate(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool nevaluate::can_apply(iterator it)
	{
	return false;
	}

Algorithm::result_t nevaluate::apply(iterator& it)
	{
	return result_t::l_no_action;
	}
