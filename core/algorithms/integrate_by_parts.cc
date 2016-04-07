
#include "algorithms/integrate_by_parts.hh"
#include "Cleanup.hh"

integrate_by_parts::integrate_by_parts(const Kernel& k, Ex& tr, Ex& derivative)
	: Algorithm(k, tr)
	{
	}

bool integrate_by_parts::can_apply(iterator st)
	{
	if(*st->name=="\\int") return true;
	return false;
	}

Algorithm::result_t integrate_by_parts::apply(iterator& it)
	{
	result_t ret=result_t::l_no_action;

	auto sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(sib->fl.parent_rel==str_node::p_none) {
			// Have found the integrand.
			if(*sib->name=="\\sum") {
				auto term=tr.begin(sib);
				while(term!=tr.end(sib)) {
					iterator ti(term);
					auto res=handle_term(ti);
					if(res==result_t::l_applied)
						ret=res;
					++term;
					}
				}
			else {
				iterator ti(sib);
				ret=handle_term(ti);
				}
			break;
			}
		++sib;
		}

	return ret;
	}

Algorithm::result_t integrate_by_parts::handle_term(iterator& it)
	{
	for(auto& pat: kernel.properties.pats) {
		if(pat.first->name()=="Coordinate") {
			std::cerr << pat.second->obj << std::endl;
			}
		}
	return result_t::l_no_action;
	}
