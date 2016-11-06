
#include "Cleanup.hh"
#include "Functional.hh"
#include "algorithms/factor_out.hh"
#include "algorithms/sort_product.hh"
#include <map>

factor_out::factor_out(const Kernel& k, Ex& e, Ex& args, bool right)
	: Algorithm(k, e), to_right(right)
	{
	cadabra::do_list(args, args.begin(), [&](Ex::iterator arg) {
			to_factor_out.push_back(Ex(arg));
			return true;
			}
		);
	}

/// Check if the expression is a sum with more than one term
bool factor_out::can_apply(iterator st)
	{
	if(*st->name=="\\sum")  return true;
	return false;
	}

Algorithm::result_t factor_out::apply(iterator& it)
	{
	result_t result=result_t::l_no_action;

	// For every term in the sum, we look at the factors in the product
	// (or at the single object if there is no product). If this factor
	// needs to be factored out, we determine if it can be moved all the
	// way to the left of the expression. If so, move the object to
	// a 'factored_out' temporary, and take out of the tree. Rinse/repeat.
	// What's left at the end is two objects: the stuff factored out,
	// and the rest. Look up if we already have 'the stuff factored out'.
	// If not, create new. If so, add this term.

	Ex_comparator comparator(kernel.properties);

	typedef std::pair<Ex, std::vector<Ex> > new_term_t;
	std::vector<new_term_t> new_terms;

	auto term=tr.begin(it);
	while(term!=tr.end(it)) {
		auto next_term=term;
		++next_term;

		iterator prod=term;
		prod_wrap_single_term(prod);

		Ex collector("\\prod"); // collect all factors that we have taken out

		// Insert a dummy symbol at the very front or back.
		iterator dummy;
		if(to_right) dummy = tr.append_child(prod, str_node("dummy"));
		else         dummy = tr.prepend_child(prod, str_node("dummy"));

		// Look at all factors in turn and determine if they should be taken out.
		if(to_right) {
			auto fac=tr.end(prod);
			auto next=fac;
			--next;
			do {
				fac=next;
				--next;
				for(size_t i=0; i<to_factor_out.size(); ++i) {
					auto match=comparator.equal_subtree(fac, to_factor_out[i].begin());
					if(match==Ex_comparator::subtree_match) {
						int sign=comparator.can_move_adjacent(prod, dummy, fac, false);
						if(sign!=0) {
							collector.append_child(collector.begin(), iterator(fac));
							multiply(prod->multiplier, sign);
							next=tr.erase(fac);
							result=result_t::l_applied;
							break;
							}
						}
					}
				} while(fac!=tr.begin(prod));
			}
		else {
			auto fac=tr.begin(prod);
			while(fac!=tr.end(prod)) {
				auto next=fac;
				++next;
				for(size_t i=0; i<to_factor_out.size(); ++i) {
					auto match=comparator.equal_subtree(fac, to_factor_out[i].begin());
					if(match==Ex_comparator::subtree_match) {
						int sign=comparator.can_move_adjacent(prod, dummy, fac, true);
						if(sign!=0) {
							collector.append_child(collector.begin(), iterator(fac));
							multiply(prod->multiplier, sign);
							next=tr.erase(fac);
							result=result_t::l_applied;
							break;
							}
						}
					}
				fac=next;
				}
			}
		
		tr.erase(dummy);
		if(tr.number_of_children(prod)==0)
			tr.append_child(prod, str_node("1"));

		// std::cerr << "product after factoring out " << Ex(prod) << std::endl;

		if(collector.number_of_children(collector.begin())!=0) { 
			// The stuff factored out of this term is in 'collector'. See if we have 
			// factored out that thing before. Because we may not always have collected
			// factors in the same order (the original expression may not have had
			// its product sorted), we first sort the collector product.

			sort_product sp(kernel, collector);
			auto coltop=collector.begin();
			if(sp.can_apply(coltop))
				sp.apply(coltop);
			multiply(prod->multiplier, *coltop->multiplier);
			one(coltop->multiplier);
			
			// Scan through the things factored out so far.
			bool found=false;
			for(auto& nt: new_terms) {
				if(nt.first==collector) { // have that factored out already, add the other factors
					nt.second.push_back(Ex(prod));
					found=true;
					break;
					}
				}
			// We hadn't factored this bit out before, make a new term.
			if(!found) {
				std::vector<Ex> v;
				v.push_back(Ex(prod));
				new_term_t nt(collector, v);
				new_terms.push_back(nt);
				}

			// All info is now in new_terms; can remove the original.
			tr.erase(prod);
			}
		else {
			prod_unwrap_single_term(prod);
			}
		term=next_term;
		}

	// Everything has been collected now into new_terms. Expand those out
	// into a proper sum of products.

	for(auto& nt: new_terms) {
		auto prod = tr.append_child(it, nt.first.begin());
		if(nt.second.size()==1) { // factored, but only one term found.
			auto top = nt.second[0].begin(); // prod node
			if(to_right) {
				auto ins = tr.end(top);
				--ins;
				while(tr.is_valid(ins)) {
					tr.prepend_child(prod, iterator(ins));
					--ins;
					}
				}
			else {
				auto ins = tr.begin(top);
				while(ins!=tr.end(top)) {
					tr.append_child(prod, iterator(ins));
					++ins;
					}
				}
			multiply(prod->multiplier, *(nt.second[0].begin()->multiplier));
// FIXME: append_children has a BUG! Messes up the tree. But it is needed to
// handle terms where the sub-factor is not a simple element.
//			tr.append_children(prod, nt.second[0].begin(top), nt.second[0].end(top));

			cleanup_dispatch(kernel, tr, prod);
			}
		else {
			iterator sum;
			if(to_right)
				sum = tr.prepend_child(prod, str_node("\\sum"));
			else
				sum = tr.append_child(prod, str_node("\\sum"));
			for(auto& term: nt.second) { 
				auto tmp = tr.append_child(sum, term.begin());
				cleanup_dispatch(kernel, tr, tmp);
				}
			}
		}
	
	// std::cerr << "end of factor_out: \n" << Ex(it) << std::endl;

	return result;
	}

