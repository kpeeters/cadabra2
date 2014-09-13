
#include "algorithms/prodcollectnum.hh"

prodcollectnum::prodcollectnum(Kernel& k, exptree& tr)
	: Algorithm(k, tr)
	{
	}

bool prodcollectnum::can_apply(iterator it)
	{
	if(*it->name!="\\prod") return false;
	sibling_iterator facs=tr.begin(it);
	while(facs!=tr.end(it)) {
		if(facs->is_rational() || *facs->multiplier!=1)
			return true;
		++facs;
		}
	return false;
	}

Algorithm::result_t prodcollectnum::apply(iterator& it)
	{
	assert(*it->name=="\\prod");
	sibling_iterator facs=tr.begin(it);
	multiplier_t factor=1;
	while(facs!=tr.end(it)) {
		factor*=*facs->multiplier;
		if(facs->is_rational()) {
			multiplier_t tmp; // FIXME: there is a bug in gmp which means we have to put init on next line.
			tmp=(*facs->name).c_str();
			factor*=tmp;
		   facs=tr.erase(facs);
			if(facs==tr.end())
				facs=tr.end(it);
			}
		else {
			one(facs->multiplier);
			++facs;
			}
		}
	multiply(it->multiplier,factor);
	if(tr.number_of_children(it)==1) { // i.e. from '3*4*7*a*9'
		tr.begin(it)->fl.bracket=it->fl.bracket;
		tr.begin(it)->fl.parent_rel=it->fl.parent_rel;
		tr.begin(it)->multiplier=it->multiplier;
		tr.flatten(it);
		it=tr.erase(it);
		}
	else if(tr.number_of_children(it)==0) { // i.e. from '3*4*7*9' 
		it->name=name_set.insert("1").first;
		}
	return l_applied;
	}

