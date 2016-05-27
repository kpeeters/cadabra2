
#include "Cleanup.hh"
#include "algorithms/expand_power.hh"

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
	if(num<=1) 
		return result_t::l_no_action;

	iterator prodn=tr.insert(argument,str_node("\\prod"));

	// If the current \pow is inside a sum, do not discard the bracket
	// type on \pow but copy it onto each generated \prod element.
	if(tr.is_head(it)==false && *tr.parent(it)->name=="\\sum") 
		prodn->fl.bracket=it->fl.bracket;
	
	sibling_iterator beg=argument;
	sibling_iterator nd=beg;
	++nd;
   argument=tr.reparent(prodn,beg,nd);
	tr.erase(exponent);
	tr.flatten(it);
	multiply(prodn->multiplier, *it->multiplier);
	it=tr.erase(it);

	// Now duplicate the factor num-1 times.
	multiplier_t tot=*argument->multiplier;
	for(int i=0; i<num-1; ++i) {
		iterator tmp=tr.append_child(prodn);
		tot *= *argument->multiplier;
		iterator ins=tr.replace(tmp, argument);
		one(ins->multiplier);
		rename_replacement_dummies(ins);
		}
	one(argument->multiplier);
	multiply(prodn->multiplier, tot);

	cleanup_dispatch(kernel, tr, it);

	return result_t::l_applied;
	}
