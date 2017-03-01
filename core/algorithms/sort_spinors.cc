
#include "algorithms/sort_spinors.hh"
#include "properties/GammaMatrix.hh"
#include "properties/Spinor.hh"
#include "properties/DiracBar.hh"
#include "properties/SortOrder.hh"

using namespace cadabra;

sort_spinors::sort_spinors(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool sort_spinors::can_apply(iterator it) 
	{
	const Spinor *sp1=kernel.properties.get_composite<Spinor>(it);
	const DiracBar *db=kernel.properties.get<DiracBar>(it);

	// FIXME: make sure that the parent is a product
	if(sp1 && sp1->majorana && db) {
		iterator par=tr.parent(it);
		if(tr.is_valid(par)==false || *par->name!="\\prod") 
			return false;
		one=it;
		it.skip_children();
		++it;
		const Spinor *sp2=kernel.properties.get_composite<Spinor>(it);
		if(sp2) {
			if(sp2->majorana==false) 
				throw ArgumentException("sort_spinors: first spinor not Majorana.");
			two=it;
			gammamat=tr.end();
			return true;
			}
		const GammaMatrix *gam=kernel.properties.get_composite<GammaMatrix>(it);
		if(gam) {
			gammamat=it;
			it.skip_children();
			++it;
			sp2=kernel.properties.get_composite<Spinor>(it);
			if(sp2) {
				if(sp2->majorana==false) 
					throw ArgumentException("sort_spinors: second spinor not Majorana.");
				two=it;
				return true;
				}
			}
		}
	return false;
	}

Algorithm::result_t sort_spinors::apply(iterator& it)
	{
	int num1, num2;
	const SortOrder     *so1=kernel.properties.get_composite<SortOrder>(one,num1);
	const SortOrder     *so2=kernel.properties.get_composite<SortOrder>(two,num2);
	
	if(so1!=0 && so1==so2) {
		if(num1>num2) {
			int numind=0;
			if(gammamat!=tr.end())
				numind=number_of_indices(gammamat);
			int sign=1;
			if(((numind*(numind+1))/2)%2 == 0)
				sign*=-1;

			// Are we dealing with commuting or anti-commuting spinors?
			Ex_comparator comp(kernel.properties);
			auto cmp = comp.equal_subtree(one, two);//			auto cmp=subtree_compare(&kernel.properties, one, two);
			int ordersign=comp.can_swap(one, two, cmp, true /* ignore implicit indices */);
			sign*=ordersign;

			// Now flip the symbols and the sign, if necessary.
			sibling_iterator tru1=tr.begin(one);
			tr.swap(tru1, two);
			if(sign==-1) {
				flip_sign(one->multiplier);
				pushup_multiplier(one);
				}
			return result_t::l_applied;
			}
		else return result_t::l_no_action;
		}
	else return result_t::l_no_action;
	}

