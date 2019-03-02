
#include "Cleanup.hh"
#include "algorithms/expand_diracbar.hh"
#include "properties/DiracBar.hh"
#include "properties/Spinor.hh"
#include "properties/GammaMatrix.hh"

using namespace cadabra;

expand_diracbar::expand_diracbar(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool expand_diracbar::can_apply(iterator it)
	{
	const DiracBar *db=kernel.properties.get<DiracBar>(it);
	if(db) {
		if(*tr.begin(it)->name=="\\prod") {
			sibling_iterator ch=tr.begin(tr.begin(it));
			const GammaMatrix *gam=kernel.properties.get<GammaMatrix>(ch);
			if(gam) {
				++ch;
				const Spinor *sp=kernel.properties.get_composite<Spinor>(ch);
				if(sp && ++ch==tr.end(tr.begin(it))) return true;
				}
			}
		}
	return false;
	}

Algorithm::result_t expand_diracbar::apply(iterator& it)
	{
	// \bar{\prod{\gamma_{a b} \epsilon}} -> \prod{\bar{\epsilon} \gamma_{a b}}

	sibling_iterator prodnode=tr.begin(it);
	sibling_iterator gamnode =tr.begin(prodnode);
	sibling_iterator spinnode=gamnode;
	++spinnode;

	iterator newprod=tr.wrap(it, str_node("\\prod"));
	multiply(newprod->multiplier, *prodnode->multiplier);
	multiply(newprod->multiplier, *it->multiplier);
	one(prodnode->multiplier);
	one(it->multiplier);
	tr.move_after(it, (iterator)gamnode);
	tr.flatten(prodnode);
	tr.erase(prodnode);

	// n + (n-1) + .. 1 = 1/2 n (n+1) signs, of which the first 'n' are Minkowski.

	unsigned int n=tr.number_of_children(gamnode);
	if(n*(n+1)/2 % 2 != 0)
		flip_sign(newprod->multiplier);

	it=newprod;
	cleanup_dispatch(kernel, tr, it);

	return result_t::l_applied;
	}
