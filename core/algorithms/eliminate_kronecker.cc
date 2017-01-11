
#include "Cleanup.hh"
#include "algorithms/eliminate_kronecker.hh"
#include "properties/KroneckerDelta.hh"
#include "properties/Integer.hh"

using namespace cadabra;

eliminate_kronecker::eliminate_kronecker(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool eliminate_kronecker::can_apply(iterator st)
	{
	if(*st->name!="\\prod") 
		if(!is_single_term(st))
			return false;

	return true;
	}

Algorithm::result_t eliminate_kronecker::apply(iterator& st)
	{
	result_t ret=result_t::l_no_action;
	prod_wrap_single_term(st);
	const nset_t::iterator onept=name_set.insert("1").first;

	int looping=0;

	sibling_iterator it=tr.begin(st);
	while(it!=tr.end(st)) { // Loop over all factors in a product, looking for Kroneckers
		bool replaced=false;
		// std::cerr << *it->name << std::endl;
		const KroneckerDelta *kr=kernel.properties.get<KroneckerDelta>(it);
		if(kr && tr.number_of_children(it)==2) {
			// std::cerr << "KD " << Ex(it) << std::endl;
			sibling_iterator ii1=tr.begin(it);
			sibling_iterator ii2=ii1; ++ii2;
			if(subtree_compare(&kernel.properties, ii1, ii2, 1, false, -2, true)==0) { // a self-contracted Kronecker delta
				// std::cerr << "self-contracted delta with " << Ex(ii1) << " = " << Ex(ii2) << std::endl;
				const Integer *itg1=kernel.properties.get<Integer>(ii1, true);
				const Integer *itg2=kernel.properties.get<Integer>(ii2, true);
				if(itg1 && itg2 && ii1->is_rational()==false && ii2->is_rational()==false) {
					if(itg1->from.begin()!=itg1->from.end() && itg2->from.begin()!=itg2->from.end()) {
						if(itg1->difference.begin()->name==onept) {
							multiply(st->multiplier, *itg1->difference.begin()->multiplier);
							it=tr.erase(it);
							}
						else {
							it=tr.replace(it, itg1->difference.begin());
							}
						ret=result_t::l_applied;
						}
					else ++it;
					}
				else ++it;
				}
			else {
				sibling_iterator oi=tr.begin(st);
				++looping;
				// iterate over all factors in the product
				bool doing1=false;
				bool doing2=false;
				while(!replaced && oi!=tr.end(st)) {
					if(oi!=it) { // this is not the delta node
						// compare delta indices with all indices of this object
						index_iterator ind=begin_index(oi);
						while(ind!=end_index(oi)) {
							index_iterator nxt=ind;
							++nxt;
							if(ii1->is_rational()==false && subtree_compare(&kernel.properties, ind, ii1, 1, false, -2, true)==0 ) {
								if(! (replaced && doing2) ) {
									multiplier_t mt=(*ind->multiplier) / (*ii1->multiplier);
									iterator rep=tr.replace_index(ind, ii2);
									rep->fl.parent_rel=ii2->fl.parent_rel; 
									multiply(rep->multiplier, mt);
									replaced=true;
									doing1=true;
									}
								// cannot 'break' here because that would miss cases when the 
								// delta multiplies a sum.
								}
							else if(ii2->is_rational()==false && subtree_compare(&kernel.properties, ind, ii2, 1, false, -2, true)==0) {
								if(! (replaced && doing1) ) {
									multiplier_t mt=(*ind->multiplier) / (*ii2->multiplier);
									iterator rep=tr.replace_index(ind, ii1);
									rep->fl.parent_rel=ii1->fl.parent_rel;
									multiply(rep->multiplier, mt);
									replaced=true;
									doing2=true;
									}
								// no break here either.
								}
							ind=nxt;
							}
						}
					if(!replaced) 
						++oi;
					}
				if(replaced) {
					ret=result_t::l_applied;
					it=tr.erase(it);
					}
				else ++it;
				}
			}
		else ++it;
		}
	
	// the product may have reduced to a single term or even just a constant
//	txtout << "exiting eliminate" << std::endl;
//	prod_unwrap_single_term(st);
//	txtout << st.node << " " << tr.parent(st).node << std::endl;
//	txtout << *st->name << " " << *(tr.parent(st)->name) << std::endl;
	sibling_iterator ff=tr.begin(st);
	if(ff==tr.end(st)) {
		st->name=onept;
		}
	else {
		++ff;
		if(ff==tr.end(st)) {
			tr.begin(st)->fl.bracket=st->fl.bracket;
			tr.begin(st)->fl.parent_rel=st->fl.parent_rel;
			tr.begin(st)->multiplier=st->multiplier;
			tr.flatten(st);
			st=tr.erase(st);
			}
		}
	cleanup_dispatch(kernel, tr, st);
//	cleanup_sums_products(tr, st);
//	txtout << "looped " << looping << std::endl;

	return ret;
	}
