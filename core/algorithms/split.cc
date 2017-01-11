
#include "split.hh"

using namespace cadabra;

split::split(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool split::can_apply(iterator st)
	{
	return true;
	}

Algorithm::result_t split::apply(iterator& sum)
	{
	sibling_iterator sib=tr.begin(sum);
	while(sib!=tr.end(sum)) {
		Ex rep;
		++sib;
		}

	return result_t::l_applied;
	}
