
#include "algorithms/collect_components.hh"
#include "algorithms/evaluate.hh"

collect_components::collect_components(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool collect_components::can_apply(iterator st)
	{
	assert(tr.is_valid(st));
	if(*st->name=="\\sum") return true;
	return false;
	}

Algorithm::result_t collect_components::apply(iterator& st)
	{
	result_t res=result_t::l_no_action;
	evaluate eval(kernel, tr, tr);

	sibling_iterator s1=tr.begin(st);

	while(s1!=tr.end(st)) {
		if(*s1->name=="\\components") { // First term found, now collect all others
			sibling_iterator s2=tr.begin(st);
			++s2;
			while(s2!=tr.end(st)) {
				if(*s2->name=="\\components") {
					eval.merge_components(s1,s2);
					s2=tr.erase(s2);
					res=result_t::l_applied;
					}
				else ++s2;
				}
			break;
			}
		++s1;
		}

	if(s1!=tr.end(st)) {
		auto comma=tr.end(s1);
		--comma;
		if(tr.number_of_children(comma)==0)
			node_zero(s1);
		}
	
	return res;
	}

