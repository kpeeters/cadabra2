
#include "lower_free_indices.hh"

using namespace cadabra;

lower_free_indices::lower_free_indices(const Kernel& k, Ex& tr, bool lower_)
	: Algorithm(k, tr), lower(lower_)
	{
	}

bool lower_free_indices::can_apply(iterator )
	{
	return true;
	}

Algorithm::result_t lower_free_indices::apply(iterator& it)
	{
	auto res = result_t::l_no_action;

	auto sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(sib->fl.parent_rel==(lower?str_node::p_super:str_node::p_sub)) {
			auto indices = kernel.properties.get<Indices>(sib, true);
			if(indices && indices->position_type==Indices::position_t::free) {
				sib->fl.parent_rel=(lower?str_node::p_sub:str_node::p_super);
				res=result_t::l_applied;
				}
			}
		++sib;
		}

	return res;
	}
