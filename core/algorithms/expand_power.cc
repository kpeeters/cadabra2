
#include "Cleanup.hh"
#include "algorithms/expand_power.hh"

using namespace cadabra;

expand_power::expand_power(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool expand_power::can_apply(iterator it)
	{
	if(*it->name=="\\pow") {
		sibling_iterator exponent=tr.begin(it);
		++exponent;
		if(exponent->is_integer())
			return true;
		}
	return false;
	}

Algorithm::result_t expand_power::apply(iterator& it)
	{
	iterator argument=tr.begin(it);
	sibling_iterator exponent=tr.begin(it);
	++exponent;

	int num=to_long(*exponent->multiplier);
//	if(num<1)
//		return result_t::l_no_action;

	if(num==-1 && *argument->name!="\\prod")
		return result_t::l_no_action;

	if(num==0) {
		node_one(it);
		return result_t::l_applied;
		}
	
	iterator prodn=tr.insert(argument,str_node("\\prod"));

	// If the current \pow is inside a sum, do not discard the bracket
	// type on \pow but copy it onto each generated \prod element.
	if(tr.is_head(it)==false && *tr.parent(it)->name=="\\sum")
		prodn->fl.bracket=it->fl.bracket;

	// Two cases:
	//    (a b c)**2 -> a b c a b c;
	//    (a b c)**(-2) -> a**(-1) b**(-1) c**(-1) a**(-1) b**(-1) c**(-1)
	// So we treat the latter as the former, and then at the end
	// just wrap all factors in a \pow{...}{-1}.
	
	sibling_iterator beg=argument;
	sibling_iterator nd=beg;
	++nd;
	argument=tr.reparent(prodn, beg, nd);
	tr.erase(exponent);
	tr.flatten(it);
	multiply(prodn->multiplier, *it->multiplier);
	it=tr.erase(it);
	
	// Now duplicate the factor abs(num)-1 times.
	multiplier_t tot=*argument->multiplier;
	for(int i=0; i<std::abs(num)-1; ++i) {
		iterator tmp=tr.append_child(prodn);
		tot *= *argument->multiplier;
		iterator ins=tr.replace(tmp, argument);
		one(ins->multiplier);
		rename_replacement_dummies(ins);
		}
	one(argument->multiplier);
	if(num<1)
		multiply(prodn->multiplier, 1/tot);
	else
		multiply(prodn->multiplier, tot);


	// If the argument of the original \pow{...}{...} was a \prod,
	// then we now have a nested product.
	cleanup_dispatch(kernel, tr, it);

	if(num<1) {
		if(*it->name=="\\prod") {
			sibling_iterator sib = tr.begin(it);
			while(sib!=tr.end(it)) {
				auto nxt=sib;
				++nxt;
				auto newpow = tr.wrap(sib, str_node("\\pow"));
				tr.append_child(newpow, str_node("1"))->multiplier = rat_set.insert(-1).first;
				sib=nxt;
				}
			}
		else {
			auto newpow = tr.wrap(it, str_node("\\pow"));
			tr.append_child(newpow, str_node("1"))->multiplier = rat_set.insert(-1).first;
			}
		}


	return result_t::l_applied;
	}
