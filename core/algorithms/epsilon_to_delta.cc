
#include "algorithms/epsilon_to_delta.hh"
#include "algorithms/reduce_delta.hh"
#include "properties/EpsilonTensor.hh"
#include "properties/Metric.hh"

epsilon_to_delta::epsilon_to_delta(const Kernel& k, Ex& tr, bool r)
	: Algorithm(k, tr), reduce(r)
	{
	}

bool epsilon_to_delta::can_apply(iterator st)
	{
	if(*st->name!="\\prod")
		return false;

	epsilons.clear();
	// Find the two epsilon tensors in the product.
	sibling_iterator it=tr.begin(st);
	signature=1;
	while(it!=tr.end(st)) {
		const EpsilonTensor *eps=kernel.properties.get<EpsilonTensor>(it);
		if(eps) {
			epsilons.push_back(it);
			// FIXME: what if the epsilons are not all the same type?
			if(eps->metric.begin()!=eps->metric.end()) {
				const Metric *met=kernel.properties.get<Metric>(eps->metric.begin());
				if(met) 
					signature=met->signature;
				}
			if(eps->krdelta.begin()!=eps->krdelta.end())
				repdelta=eps->krdelta;
			}
		++it;
		}
	if(epsilons.size()<2)
		return false;

	if(repdelta.begin()==repdelta.end())
		return false;
	else repdelta.erase_children(repdelta.begin());

	return true;
	}

Algorithm::result_t epsilon_to_delta::apply(iterator& st)
	{
	Ex       rep(repdelta);
	iterator delta=rep.begin();

	sibling_iterator eps1=tr.begin(epsilons[0]);
	sibling_iterator eps2=tr.begin(epsilons[1]);
	while(eps1!=tr.end(epsilons[0])) {
		rep.append_child(delta, *eps1);
		rep.append_child(delta, *eps2);
		++eps1;
		++eps2;
		}
	multiply(st->multiplier, *epsilons[0]->multiplier);
	multiply(st->multiplier, *epsilons[1]->multiplier);
	tr.erase(epsilons[0]);
	std::cerr << tr.number_of_children(epsilons[1]) << std::endl;
	std::cerr << " -> " << *st->multiplier << " * " << combin::fact(multiplier_t(tr.number_of_children(epsilons[1]))) << std::endl;
	multiply(st->multiplier, combin::fact(multiplier_t(tr.number_of_children(epsilons[1]))));

	multiply(st->multiplier, signature);

	iterator gend=tr.replace(epsilons[1], rep.begin());

	if(reduce) {
		reduce_delta rg(kernel, tr);
		if(rg.can_apply(gend))
			rg.apply(gend);
		if(*gend->multiplier==0) {
			zero(st->multiplier);
			return result_t::l_applied;
			}
		}
	
	if(*gend->multiplier!=1) {
		multiply(tr.parent(gend)->multiplier, *gend->multiplier);
		gend->multiplier=rat_set.insert(1).first;
		}

	if(tr.number_of_children(st)==1) {
		multiply(tr.begin(st)->multiplier, *st->multiplier);
		tr.flatten(st);
		st=tr.erase(st);
		}

	return result_t::l_applied;
	}

