
#include "algorithms/epsilon_to_delta.hh"
#include "algorithms/reduce_delta.hh"
#include "properties/EpsilonTensor.hh"
#include "properties/Metric.hh"

using namespace cadabra;

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
	std::multimap<std::string,Ex::iterator> emap;
	sibling_iterator it=tr.begin(st);
	while(it!=tr.end(st)) {
		const EpsilonTensor *eps=kernel.properties.get<EpsilonTensor>(it);
		if(eps) emap.insert(std::pair<std::string,Ex::iterator>(eps->index_set_name,it));
		++it;
		}
	signature=1;
	std::multimap<std::string,Ex::iterator>::iterator eit=emap.begin();
	while(epsilons.size()<2 && eit!=emap.end()) {
		if(emap.count(eit->first)>1) {
			epsilons.push_back(eit->second);
			++eit;
			epsilons.push_back(eit->second);
			const EpsilonTensor *eps=kernel.properties.get<EpsilonTensor>(eit->second);
			if(eps->metric.begin()!=eps->metric.end()) {
				const Metric *met=kernel.properties.get<Metric>(eps->metric.begin());
				if(met)
					signature=met->signature;
				}
			if(eps->krdelta.begin()!=eps->krdelta.end())
				repdelta=eps->krdelta;
			}
		++eit;
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

	// std::cerr << Ex(st) << std::endl;

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
	//	std::cerr << tr.number_of_children(epsilons[1]) << std::endl;
	//	std::cerr << " -> " << *st->multiplier << " * " << combin::fact(multiplier_t(tr.number_of_children(epsilons[1]))) << std::endl;
	multiply(st->multiplier, combin::fact(multiplier_t(tr.number_of_children(epsilons[1]))));
	//	std::cerr << "A:" << *st->multiplier << std::endl;
	multiply(st->multiplier, signature);
	//	std::cerr << "B:" << *st->multiplier << std::endl;

	iterator gend=tr.replace(epsilons[1], rep.begin());
	//	std::cerr << "B2:" << *st->multiplier << std::endl;

	if(reduce) {
		//		std::cerr << "reducing" << std::endl;
		reduce_delta rg(kernel, tr);
		if(rg.can_apply(gend))
			rg.apply(gend);
		if(*gend->multiplier==0) {
			zero(st->multiplier);
			return result_t::l_applied;
			}
		}

	if(*gend->multiplier!=1) {
		//		std::cerr << "B3:" << *st->multiplier << std::endl;
		//		std::cerr << "B3:" << *gend->multiplier << std::endl;
		multiply(tr.parent(gend)->multiplier, *gend->multiplier);
		one(gend->multiplier);
		//		std::cerr << "C:" << *st->multiplier << std::endl;
		}

	if(tr.number_of_children(st)==1) {
		multiply(tr.begin(st)->multiplier, *st->multiplier);
		tr.flatten(st);
		st=tr.erase(st);
		}

	//	std::cerr << Ex(st) << std::endl;

	return result_t::l_applied;
	}

