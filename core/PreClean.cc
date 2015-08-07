
#include "PreClean.hh"
#include "Cleanup.hh"

void pre_clean_dispatch(Kernel& kernel, Ex& ex, Ex::iterator& it)
	{
	if(*it->name!="1" && it->is_unsimplified_rational()) cleanup_rational(kernel, ex, it);
	
	if(*it->name=="\\frac")     cleanup_frac(kernel, ex, it);
	else if(*it->name=="\\sub") cleanup_sub(kernel, ex, it);
	}

void pre_clean_dispatch_deep(Kernel& k, Ex& tr)
	{
	return cleanup_dispatch_deep(k, tr, &pre_clean_dispatch);
	}

void cleanup_rational(Kernel& k, Ex& tr, Ex::iterator& st)
	{
	multiplier_t num(*st->name);
	st->name=name_set.insert("1").first;
	multiply(st->multiplier,num);
	}

void cleanup_frac(Kernel& k, Ex& tr, Ex::iterator& st)
	{
	// Catch \frac{} nodes with one argument; those are supposed to be read as 1/(...).
	if(tr.number_of_children(st)==1) {
		tr.insert(tr.begin(st), str_node("1"));
		}

	assert(tr.number_of_children(st)>1);
	Ex::sibling_iterator it=tr.begin(st);
	multiplier_t rat;

	bool allnumerical=true;
	rat=*(it->multiplier);
	if(it->is_rational()==false) 
		allnumerical=false;

	one(it->multiplier);
	++it;
	while(it!=tr.end(st)) {
		if(*it->multiplier==0) {
			// CHECK: do these zeroes get handled correctly elsewhere?
			return;
			}
		rat/=*it->multiplier;
		one(it->multiplier);
		if(it->is_rational()==false) allnumerical=false;
		++it;
		}
	if(allnumerical) { // can remove the \frac altogether
		tr.erase_children(st);
		st->name=name_set.insert("1").first;
		}
	else { // just remove the all-numerical child nodes
		it=tr.begin(st);
		++it;
		while(it!=tr.end(st)) {
			if(it->is_rational()) 
				it=tr.erase(it);
			else ++it;
			}
		if(tr.number_of_children(st)==1) {
			tr.begin(st)->fl.bracket=st->fl.bracket;
			tr.begin(st)->fl.parent_rel=st->fl.parent_rel;
			multiply(tr.begin(st)->multiplier, *st->multiplier);
			tr.flatten(st);
			st=tr.erase(st);
			}
		}
//	expression_modified=true;
	multiply(st->multiplier, rat);
//	pushup_multiplier(st);
//	return l_applied;
	}

void cleanup_sub(Kernel& k, Ex& tr, Ex::iterator& it)
	{
	assert(tr.number_of_children(it)>1); // To guarantee that we have really cleaned up that old stuff.

	it->name=name_set.insert("\\sum").first;
	Ex::sibling_iterator sit=tr.begin(it);

	// Make sure that all terms have the right sign, and zeroes are removed.
	if(*sit->multiplier==0) sit=tr.erase(sit);
	else                    ++sit;

	while(sit!=tr.end(it)) {
		if(*sit->multiplier==0)
			sit=tr.erase(sit);
		else {
			flip_sign(sit->multiplier);
			++sit;
			}
		}

	// Single-term situation: remove the \sum.
	assert(tr.number_of_children(it)>0);
	if(tr.number_of_children(it)==1) {
		sit=tr.begin(it);
		sit->fl.parent_rel=it->fl.parent_rel;
		sit->fl.bracket=it->fl.bracket;
		multiply(sit->multiplier, *it->multiplier);
		tr.flatten(it);
		it=tr.erase(it);
		}
	}

