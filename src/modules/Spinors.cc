
#include "Spinors.hh"

spinorsort::spinorsort(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool spinorsort::can_apply(iterator it) 
	{
	const Spinor *sp1=properties::get_composite<Spinor>(it);
	const DiracBar *db=properties::get<DiracBar>(it);

	// FIXME: make sure that the parent is a product
	if(sp1 && sp1->majorana && db) {
		iterator par=tr.parent(it);
		if(tr.is_valid(par)==false || *par->name!="\\prod") 
			return false;
		one=it;
		it.skip_children();
		++it;
		const Spinor *sp2=properties::get_composite<Spinor>(it);
		if(sp2) {
			if(sp2->majorana==false) {
				txtout << "spinorsort: first spinor not Majorana." << std::endl;
				return false;
				}
			two=it;
			gammamat=tr.end();
			return true;
			}
		const GammaMatrix *gam=properties::get_composite<GammaMatrix>(it);
		if(gam) {
			gammamat=it;
			it.skip_children();
			++it;
			sp2=properties::get_composite<Spinor>(it);
			if(sp2) {
				if(sp2->majorana==false) {
					txtout << "spinorsort: second spinor not Majorana." << std::endl;
					return false;
					}
				two=it;
				return true;
				}
			}
		}
	return false;
	}

algorithm::result_t spinorsort::apply(iterator& it)
	{
	int num1, num2;
	const SortOrder     *so1=properties::get_composite<SortOrder>(one,num1);
	const SortOrder     *so2=properties::get_composite<SortOrder>(two,num2);
	
	if(so1!=0 && so1==so2) {
		if(num1>num2) {
			int numind=0;
			if(gammamat!=tr.end())
				numind=tr.number_of_indices(gammamat);
			int sign=1;
			if(((numind*(numind+1))/2)%2 == 0)
				sign*=-1;

			// Are we dealing with commuting or anti-commuting spinors?
			int cmp=subtree_compare(one, two);
			int ordersign=exptree_ordering::can_swap(one, two, cmp, true /* ignore implicit indices */);
			sign*=ordersign;

			// Now flip the symbols and the sign, if necessary.
			sibling_iterator tru1=tr.begin(one);
			tr.swap(tru1, two);
			if(sign==-1) {
				flip_sign(one->multiplier);
				pushup_multiplier(one);
				}
			expression_modified=true;
			return l_applied;
			}
		else return l_no_action;
		}
	else return l_no_action;
	}

