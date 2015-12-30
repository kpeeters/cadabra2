
#include "Functional.hh"
#include "Cleanup.hh"
#include "algorithms/evaluate.hh"
#include "algorithms/substitute.hh"
#include <functional>

evaluate::evaluate(Kernel& k, Ex& tr, const Ex& c)
	: Algorithm(k, tr), components(c)
	{
	// Preparse the arguments.
//	collect_index_values(ind_values);	
	}

bool evaluate::can_apply(iterator) 
	{
	return true;
	}

Algorithm::result_t evaluate::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	// Descend down the tree. FIXME: docs outdated.
	// Then determine the component values of all terms. Instead of
	// looping over all possible index value sets, we look straight at
	// the substitution rules, and check that these are required for
	// some index values (this is where symmetry arguments should come
	// in as well).
   //
	// FIXME: has this now been superceded by the logic in Compare.cc which
	// takes into account the index_values map there and can match component
	// A_{t r} in the rule to abstract tensor A_{m n} in the expression?
	//
	// In the example, we see A_{m n} in the expression, and we see
	// that the rules for A_{t t} and A_{r r} match this tensor.
	
	
	Ex::post_order_iterator walk=it, last=it;
	++last;
	walk.descend_all();

	do {
		auto nxt=walk;
		++nxt;

		if(*(walk->name)=="\\sum")       handle_sum(walk);
		else if(*(walk->name)=="\\prod") handle_prod(walk);

		walk=nxt;
		} while(walk!=last);

	return res;
	}


void evaluate::handle_sum(iterator it)
	{
	std::cerr << "evaluate::sum" << std::endl;

	index_map_t full_ind_free, full_ind_dummy;

	// First find the values that all indices will need to take.
	classify_indices(it, full_ind_free, full_ind_dummy);
	for(auto i: full_ind_free) {
		const Indices *prop = kernel.properties.get<Indices>(i.second);
		if(prop==0)
			throw ArgumentException("evaluate: Index "+*(i.second->name)+" does not have an Indices property.");

		if(prop->values.size()==0)
			throw ArgumentException("evaluate: Do not know values of index "+*(i.second->name)+".");
		}

	// Iterate over all terms in the sum. These should be of three types: \component nodes,
	// nodes with only free indices, and nodes with internal contractions (e.g. A_{m}^{m}). 
	// The first type can, at this stage, be ignored. The second type needs to be converted
	// into a \component node using the component evaluation rules. The last type needs 
	// separate treatment, and is handled by handle_internal_contraction.

	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		sibling_iterator nxt=sib;
		++nxt;

		handle_factor(sib, full_ind_free);

		sib=nxt;
		}


	// Now all terms are \component nodes. We need to merge these together into a single
	// node.
	auto sib1=tr.begin(it);
	auto sib2=sib1;
	++sib2;
	while(sib2!=tr.end(it)) {
		merge_components(sib1, sib2);
		sib2=tr.erase(sib2);
		}
	tr.flatten_and_erase(it);
	}

void evaluate::handle_factor(sibling_iterator sib, const index_map_t& full_ind_free)
	{
	if(*sib->name=="\\components") return;
	
	// Internal contractions.
	index_map_t ind_free, ind_dummy;
	classify_indices(sib, ind_free, ind_dummy);
	if(ind_dummy.size()>0) {
		std::cerr << "Internal contractions, not yet handled" << std::endl;
		return;
		}
	
	// Attempt to apply each component substitution rule on this term.
	Ex repl("\\components");
	for(auto& ind: ind_free) 
		repl.append_child(repl.begin(), ind.second);
	auto vl = repl.append_child(repl.begin(), str_node("\\comma"));
	cadabra::do_list(components, components.begin(), [&](Ex::iterator c) {
			Ex rule(c);
			Ex obj(sib);
			substitute subs(kernel, obj, rule);
			iterator oit=obj.begin();
			if(subs.can_apply(oit)) {
				auto el = repl.append_child(vl, str_node("\\equals"));
				auto il = repl.append_child(el, str_node("\\comma"));
				auto fi = full_ind_free.begin();
				if(fi==full_ind_free.end()) {
					for(auto& r: subs.comparator.index_value_map) 
						repl.append_child(il, r.second.begin()); 
					}
				else {
					while(fi!=full_ind_free.end()) {
						for(auto& r: subs.comparator.index_value_map) {
							if(fi->first == r.first) {
								repl.append_child(il, r.second.begin()); 
								break;
								}
							}
						auto fiold(fi);
						while(fi!=full_ind_free.end() && fiold->first==fi->first)
							++fi;
						}
					}
				subs.apply(oit);
				repl.append_child(el, obj.begin());
				// FIXME: this wastes time, as we could now simply exit do_list,
				// but there is no mechanism for that.
				}
			});


	tr.move_ontop(iterator(sib), repl.begin());
	}

void evaluate::merge_components(iterator it1, iterator it2)
	{
	sibling_iterator sib1=tr.end(it1);
	--sib1;
	sibling_iterator sib2=tr.end(it2);
	--sib2;

	cadabra::do_list(tr, sib2, [&](Ex::iterator it2) {
			assert(*it2->name=="\\equals");

			// We can now directly compare the lhs of this equals node with the lhs
			// of the equals node of the other components node.

			auto lhs2 = tr.begin(it2);
			auto found = cadabra::find_in_list(tr, sib1, [&](Ex::iterator it1) {
					auto lhs1 = tr.begin(it1);
					if(tr.equal_subtree(lhs1, lhs2)) {
						auto sum1=lhs1;
						++sum1;
						auto sum2=lhs2;
						++sum2;
						if(*sum1->name!="\\sum") 
							sum1=tr.wrap(sum1, str_node("\\sum"));
						tr.append_child(sum1, sum2);
						return iterator(sum1);
						}

					return tr.end();
					});
			if(found==tr.end()) {
				tr.append_child(iterator(sib1), it2);
				}
			});
	}

void evaluate::handle_prod(iterator it)
	{
	std::cerr << "evaluate::prod" << std::endl;
	
	// All factors are either \component nodes or nodes which still need replacing.

	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		sibling_iterator nxt=sib;
		++nxt;

		index_map_t empty;
		handle_factor(sib, empty);

		sib=nxt;
		}
	
	// Now everything is a \component node; the thing is effectively a
	// large sparse tensor product. We need to do the sums over the dummy
	// indices.
	
	index_map_t ind_free, ind_dummy;
	classify_indices(it, ind_free, ind_dummy);

	auto di = ind_dummy.begin();
	// Since no factor can be a sum anymore, dummy indices always occur in pairs,
	// there is no need to account for anything more tricky. Every pair leads
	// to a sum.
	while(di!=ind_dummy.end()) {
		std::cerr << *(di->first.begin()->name) << std::endl;
		// Figure out which values this index can take

		// three cases:
		//    two factors, single index in common. Merge is simple.
		//    two factors, more than one index in common. After merging this turns into:
		//    single factor, self-contraction
		++di;
		}
	}
