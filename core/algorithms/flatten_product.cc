
#include "algorithms/flatten_product.hh"
#include "properties/PartialDerivative.hh"

using namespace cadabra;


flatten_product::flatten_product(const Kernel& k, Ex& tr)
	: Algorithm(k, tr), make_consistent_only(false), is_diff(false)
	{
	}

bool flatten_product::can_apply(iterator it)
	{
	is_diff=false;
	if(*it->name!="\\prod") 
		 return false;

// FIXME: acting on PartialDerivative has been disabled because
// it really does not make sense anymore.
//		 if(properties::get<PartialDerivative>(it)==0)
//			  return false;
//		 else is_diff=true;

	if(!is_diff && tr.number_of_children(it)==1) return true;
	sibling_iterator facs=tr.begin(it);
	while(facs!=tr.end(it)) {
		const PartialDerivative *pd=kernel.properties.get<PartialDerivative>(facs);
		if((is_diff && pd) || (!is_diff && *facs->name=="\\prod"))
			return true;
		if(is_diff) break;
		++facs;
		}
	return false;
	}

Algorithm::result_t flatten_product::apply(iterator& it)
	{
//	debugout << "acting with flatten_product at " << *it->name << std::endl;
	if(!is_diff && tr.number_of_children(it)==1) {
		tr.begin(it)->fl.bracket = it->fl.bracket;
		multiply(tr.begin(it)->multiplier, *it->multiplier);
		tr.flatten(it);
		it=tr.erase(it);
		pushup_multiplier(it);
		return result_t::l_applied;
		}

	result_t ret=result_t::l_no_action;
	sibling_iterator facs=tr.begin(it);
	str_node::bracket_t btype=facs->fl.bracket;
	while(facs!=tr.end(it)) {
		const PartialDerivative *pd=kernel.properties.get<PartialDerivative>(facs);
		if((is_diff && pd) || (!is_diff && *facs->name=="\\prod")) {
			str_node::bracket_t cbtype=tr.begin(facs)->fl.bracket;
			if(!make_consistent_only || cbtype==str_node::b_none || cbtype==str_node::b_no) {
				sibling_iterator prodch=tr.begin(facs);
				while(prodch!=tr.end(facs)) {
					prodch->fl.bracket=btype;
					++prodch;
					}
				sibling_iterator tmp=facs;
				++tmp;
				tr.flatten(facs);
				multiply(it->multiplier,*facs->multiplier);
				tr.erase(facs);
				pushup_multiplier(it);
				facs=tmp;
				ret=result_t::l_applied;
				}
			else ++facs;
			}
		else ++facs;
		if(is_diff) break;
		}
	return ret;
	}
