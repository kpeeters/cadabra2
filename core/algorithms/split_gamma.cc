
#include "Cleanup.hh"
#include "algorithms/split_gamma.hh"
#include "algorithms/join_gamma.hh"

split_gamma::split_gamma(const Kernel& k, Ex& e, bool ob)
	: Algorithm(k, e), on_back(ob)
	{
	}

bool split_gamma::can_apply(iterator it)
	{
	if(kernel.properties.get<GammaMatrix>(it))
		if(tr.number_of_children(it)>1) {
			return true;
			}
	return false;
	}

Algorithm::result_t split_gamma::apply(iterator& it)
	{
	// Make a new expression which is the 'join' of the result which we want.
	Ex work;
	work.set_head(str_node("\\expression"));
	iterator prodnode=work.append_child(work.begin(), str_node("\\prod"));
	iterator firstgam, secondgam;
	if(on_back) {
		firstgam =work.append_child(prodnode, it);
		secondgam=work.append_child(prodnode, *it);
		}
	else {
		secondgam=work.append_child(prodnode, *it);
		firstgam =work.append_child(prodnode, it);
		}
	sibling_iterator specind;
	if(on_back) {
		specind=work.end(firstgam);
		--specind;
		}
	else specind=work.begin(firstgam);
	work.append_child(secondgam, (iterator)(specind));
	work.erase(specind);
	
	join_gamma jn(kernel, tr, true, true);
	const GammaMatrix *gm1=kernel.properties.get<GammaMatrix>(it);
	jn.gm1=gm1;
	jn.apply(prodnode);

	// Replace maximally-antisymmetric gamma with the product
   // in which one gamma is back-split.
	iterator maxgam=work.begin(prodnode);
	if(on_back) {
		specind=work.end(maxgam);
		--specind;
		}
	else specind=work.begin(maxgam);
	iterator newprod=work.insert(maxgam, str_node("\\prod"));
	sibling_iterator fr=maxgam, to=fr;
	++to;
	maxgam=work.reparent(newprod, fr, to);
	iterator splitgam;
	if(on_back) splitgam=work.append_child(newprod, *maxgam);
	else        splitgam=work.insert(maxgam, *maxgam);
	work.append_child(splitgam,(iterator)(specind)); 
	work.erase(specind);

	// Flip signs on all other terms.
	sibling_iterator other=work.begin(prodnode);
	++other;
	while(other!=work.end(prodnode)) {
		flip_sign(other->multiplier);
		++other;
		}
	it=tr.replace(it, prodnode);

	cleanup_dispatch(kernel, tr, it);
	return result_t::l_applied;
	}

