
#include "Functional.hh"
#include "algorithms/evaluate.hh"
#include "algorithms/substitute.hh"
#include <functional>

evaluate::evaluate(Kernel& k, Ex& tr, const Ex& c)
	: Algorithm(k, tr), components(c)
	{
	// Preparse the arguments.
//	collect_index_values(ind_values);	
//	prepare_replacement_rules(components);
	}

bool evaluate::can_apply(iterator) 
	{
	return true;
	}

Algorithm::result_t evaluate::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	// Descend down the tree.
	Ex::post_order_iterator walk=it;
	walk.descend_all();

	do {
		if(*(walk->name)=="\\sum")       handle_sum(walk);
		else if(*(walk->name)=="\\prod") handle_prod(walk);

		++walk;
		} while(walk!=it);

	return res;
	}


void evaluate::prepare_replacement_rules()
	{
	// We need to be able to match A_{t t} to A_{m n} knowing that m,n
	// take values including t. In order to do this, we look at the
	// component replacement rule, and then do an inverse lookup from the 
	// component values that we see there to the possible index name.
	// Typically (when indices take values in numbers and we have more than
	// one index set) this gives multiple options from the index value to
	// the index that can take this value.

	// FIXME: has this now been superceded by the logic in Compare.cc which
	// takes into account the index_values map there and can match component
	// A_{t r} in the rule to abstract tensor A_{m n} in the expression?

	std::map<Ex, const Indices *> index_candidates;
	
	cadabra::do_list(components, components.begin(), [&](Ex::iterator c) {
			index_map_t ind_free, ind_dummy;
			auto pat = components.begin(c);
			classify_indices(pat, ind_free, ind_dummy);
			std::cerr << "free indices in " << *pat->name << ": ";
			for(auto in: ind_free)
				std::cerr << *in.second->name << ", ";
			std::cerr << std::endl;
			});
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
		if(*sib->name=="\\components") continue;

		// Internal contractions.
		index_map_t ind_free, ind_dummy;
		classify_indices(sib, ind_free, ind_dummy);
		if(ind_dummy.size()>0) {
			std::cerr << "Internal contractions, not yet handled" << std::endl;
			continue;
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
					while(fi!=full_ind_free.end()) {
						for(auto& r: subs.comparator.index_value_map) {
							if(fi->first == r.second) {
								repl.append_child(il, r.first.begin()); 
								break;
								}
							}
						auto fiold(fi);
						while(fi!=full_ind_free.end() && fiold->first==fi->first)
							++fi;
						}
					subs.apply(oit);
					repl.append_child(el, obj.begin());
					// FIXME: this wastes time, as we could now simply exit do_list,
					// but there is no mechanism for that.
					}
				});

		sibling_iterator nxt=sib;
		++nxt;

		tr.move_ontop(iterator(sib), repl.begin());
		sib=nxt;
		}


	// Now all terms are \component nodes. We need to merge these together into a single
	// node.
	auto sib1=tr.begin(it);
	auto sib2=sib1;
	++sib2;
	while(sib2!=tr.end(it)) {
		merge_components(sib1, sib2);
		// remove sib2
		++sib2;
		}

	
	// Then determine the component values of all terms. Instead of
	// looping over all possible index value sets, we look straight at
	// the substitution rules, and check that these are required for
	// some index values (this is where symmetry arguments should come
	// in as well).
	//
	// In the example, we see A_{m n} in the expression, and we see
	// that the rules for A_{t t} and A_{r r} match this tensor.
	
	
	}

void evaluate::merge_components(iterator it1, iterator it2)
	{
	sibling_iterator sib1=tr.end(it1);
	--sib1;
	sibling_iterator sib2=tr.end(it2);
	--sib2;

	cadabra::do_list(tr, sib1, [&](Ex::iterator it) {
			std::cerr << *it->name << std::endl;
			});
	}

void evaluate::handle_prod(iterator it)
	{
	index_map_t ind_free, ind_dummy;
	classify_indices(it, ind_free, ind_dummy);
	
	}
