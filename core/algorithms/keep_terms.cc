
#include "Cleanup.hh"
#include "algorithms/keep_terms.hh"

keep_terms::keep_terms(Kernel& k, exptree& tr, std::vector<int> terms)
	: Algorithm(k, tr), terms_(terms)
	{
	}

bool keep_terms::can_apply(iterator it)
	{
	if(*it->name!="\\sum") return false;
	return true;
	}

Algorithm::result_t keep_terms::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	int count=0;
	sibling_iterator walk=tr.begin(it);
	while(walk!=tr.end(it)) {
		if(std::find(terms_.begin(), terms_.end(), count)==terms_.end()) {
			node_zero(walk);
			res=result_t::l_applied;
			}
		++count;
		++walk;
		}

	cleanup_dispatch(kernel, tr, it);

	return res;
	}

