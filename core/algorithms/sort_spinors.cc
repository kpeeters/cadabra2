
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
	const Spinor   *sp1=kernel.properties.get<Spinor>(it);
	const DiracBar *db1=kernel.properties.get<DiracBar>(it);

	// Only act if the node is a Dirac conjugate Majorana spinor.
	if(! (sp1 && sp1->majorana && db1)) return false;

	// Only act if we are inside a product.
	if(tr.is_head(it)) return false;
	
	auto par=tr.parent(it);
	if(*par->name!="\\prod")
		return false;

	one=it;
	gammamat=tr.end();
	two=tr.end();

	sibling_iterator sib=it;
	++sib;
	while(sib!=tr.end(par)) {
		const Spinor      *spinor=kernel.properties.get<Spinor>(sib);
		const GammaMatrix *gamma =kernel.properties.get<GammaMatrix>(sib);

		if(spinor) {
			if(!spinor->majorana)
				throw ArgumentException("sort_spinors: second spinor not Majorana.");
			two=sib;
			return true;
			}
		if(gamma) {
			if(gammamat!=tr.end())
				throw ArgumentException("sort_spinors: need to join_gamma first.");
			gammamat=sib;
			}
		++sib;
		}
	return false;
	}

Algorithm::result_t sort_spinors::apply(iterator& )
	{
	int num1, num2;
	const SortOrder     *so1=kernel.properties.get<SortOrder>(one,num1);
	const SortOrder     *so2=kernel.properties.get<SortOrder>(two,num2);

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

