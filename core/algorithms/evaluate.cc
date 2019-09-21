
#include "Functional.hh"
#include "Cleanup.hh"
#include "Permutations.hh"
#include "MultiIndex.hh"
//#include "SympyCdb.hh"
#include "algorithms/evaluate.hh"
#include "algorithms/simplify.hh"
#include "algorithms/substitute.hh"
#include "properties/EpsilonTensor.hh"
#include "properties/PartialDerivative.hh"
#include "properties/Coordinate.hh"
#include "properties/Depends.hh"
#include "properties/Accent.hh"
#include <functional>

// #define DEBUG 1

using namespace cadabra;

evaluate::evaluate(const Kernel& k, Ex& tr, const Ex& c, bool rhs, bool simplify)
	: Algorithm(k, tr), components(c), only_rhs(rhs), call_sympy(simplify)
	{
	}

bool evaluate::can_apply(iterator it)
	{
	return tr.is_head(it); // only act at top level, we descend ourselves
	}

bool evaluate::is_scalar_function(iterator it) const
	{
	if(*it->name=="\\pow" || *it->name=="\\exp" || *it->name=="\\sin" || *it->name=="\\cos" ) return true;
	return false;
	}

Algorithm::result_t evaluate::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	// The first pass is top-down.


	// The second pass is bottom-up. The general logic of the routines
	// this calls is that, instead of looping over all possible index
	// value sets, we look straight at the substitution rules, and
	// check that these are required for some index values (this is
	// where symmetry arguments should come in as well).
	//
	// The logic in Compare.cc helps us by matching component A_{t r}
	// in the rule to an abstract tensor A_{m n} in the expression, storing
	// the index name -> index value map.

	it = cadabra::do_subtree(tr, it, [&](Ex::iterator walk) -> Ex::iterator {
#ifdef DEBUG
		std::cerr << "evaluate at " << walk << std::endl;
#endif

		if(*(walk->name)=="\\components") walk = handle_components(walk);
		// FIXME: currently \pow is the only function for which we go straight up without
		// evaluating. For this reason, its children do not get wrapped in a \components node
		// in handle_factor. This needs to be extended to other function as well.
		else if(is_scalar_function(walk))
			{
			unwrap_scalar_in_components_node(walk);   // this is a scalar
			return walk;
			}
		else if(is_component(walk)) return walk;
		else if(*(walk->name)=="\\sum")   walk = handle_sum(walk);
		else if(*(walk->name)=="\\prod" || *(walk->name)=="\\wedge" || *(walk->name)=="\\frac")
			walk = handle_prod(walk);
		else
			{
			const PartialDerivative *pd = kernel.properties.get<PartialDerivative>(walk);
			if(pd) walk = handle_derivative(walk);
			else {
				const EpsilonTensor *eps = kernel.properties.get<EpsilonTensor>(walk);
				if(eps) {
					walk = handle_epsilon(walk);
					}
				else if(*walk->name!="\\equals" && walk->is_index()==false) {
					if(! (only_rhs && tr.is_head(walk)==false && ( *(tr.parent(walk)->name)=="\\equals" || *(tr.parent(walk)->name)=="\\arrow" ) && tr.index(walk)==0) ) {
						index_map_t empty;
						sibling_iterator tmp(walk);
#ifdef DEBUG
						std::cerr << "handling factor" << std::endl;
						std::cerr << *walk->name << std::endl;
#endif
						walk = handle_factor(tmp, empty);
						// std::cerr << "handling factor done" << std::endl;
						}
					}
				}
			}
		return walk;
		}
	                        );

	// Final cleanup, e.g. to reduce scalar expressions to proper
	// scalars instead of 'components' nodes.

	cleanup_dispatch_deep(kernel, tr);

	return res;
	}

bool evaluate::is_component(iterator it) const
	{
	// FIXME: The fact that this is called in the main loop above
	// prevents any evaluation of tensorial expressions which appear
	// inside components. Such things could in principle appear if, in
	// an existing components node, a scalar was replaced with an
	// object built from a tensor.

	do {
		if(*it->name=="\\components") {
			return true;
			}
		it=tr.parent(it);
		}
	while(tr.is_valid(it));
	return false;
	}

Ex::iterator evaluate::handle_components(iterator it)
	{
	// This just cleans up component nodes. At the moment this means
	// taking care of handling dummy pairs.

	index_map_t ind_free, ind_dummy;
	classify_indices(it, ind_free, ind_dummy);
	if(ind_dummy.size()==0) return it;

	// Wrap in a product, use handle_prod to sort out summation.
	it = tr.wrap(it, str_node("\\prod"));
	it = handle_prod(it);
	return it;
	}

Ex::iterator evaluate::handle_sum(iterator it)
	{
	// std::cerr << "handle sum" << Ex(it) << std::endl;

	// pre-scan: remove zero nodes from evaluate having processed
	// nodes at lower level, and ensure child nodes are \component
	// nodes.
	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		sibling_iterator nxt=sib;
		++nxt;

		if(*sib->multiplier==0) { // zero terms can be removed
			tr.erase(sib);
			}
		else if(is_component(sib)==false) {
			index_map_t empty;
			handle_factor(sib, empty);
			}

		sib=nxt;
		}
	if(tr.number_of_children(it)==0) {
		node_zero(it);
		return it;
		}

	index_map_t full_ind_free, full_ind_dummy;

	// First find the values that all indices will need to take. We do not loop over
	// them, but we need them in order to figure out which patterns in the rule can
	// match to patterns in the expression.

	classify_indices(it, full_ind_free, full_ind_dummy);
	for(auto i: full_ind_free) {
		// std::cerr << "finding prop for " << Ex(i.second) << std::endl;
		const Indices *prop = kernel.properties.get<Indices>(i.second);
		if(prop==0) {
			const Coordinate *crd = kernel.properties.get<Coordinate>(i.second, true);
			if(crd==0)
				throw ArgumentException("evaluate: Index "+*(i.second->name)
				                        +" does not have an Indices property.");
			}

		if(prop!=0 && prop->values.size()==0)
			throw ArgumentException("evaluate: Do not know values of index "+*(i.second->name)+".");
		}

	// Iterate over all terms in the sum. These should be of two types: \component nodes,
	// which we do not need to touch anymore, and nodes which have still not been
	// evaluated. We send them all to handle_factor, which will return immediately on the
	// first node type, and convert the second type to the first.

	sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		sibling_iterator nxt=sib;
		++nxt;
		if(sib->is_zero())
			sib=tr.erase(sib);
		else {
			handle_factor(sib, full_ind_free);
			sib=nxt;
			}
		}

	// Now all terms in the sum (which has its top node at 'it') are
	// \component nodes. We need to merge these together into a single
	// node.

	auto sib1=tr.begin(it);
	//	merge_component_children(sib1);
	auto sib2=sib1;
	++sib2;
	while(sib2!=tr.end(it)) {
#ifdef DEBUG
		std::cerr << "merging components " << Ex(sib1) << " and " << Ex(sib2) << std::endl;
#endif
		merge_components(sib1, sib2);
		sib2=tr.erase(sib2);
		}
	cleanup_components(sib1);

	it=tr.flatten_and_erase(it);

	return it;
	}

Ex::iterator evaluate::handle_factor(sibling_iterator sib, const index_map_t& full_ind_free)
	{
#ifdef DEBUG
	std::cerr << "handle_factor " << Ex(sib) << std::endl;
#endif
	if(*sib->name=="\\components") return sib;

	// If this factor is an accent at the top level, descent further.
	const Accent *acc = kernel.properties.get<Accent>(sib);
	if(acc) {
		auto deeper=tr.begin(sib);
		handle_factor(deeper, full_ind_free);
		// Put the accent on each of the components.
		sibling_iterator cl = tr.end(deeper);
		--cl;
		cadabra::do_list(tr, cl, [&](Ex::iterator c) {
			auto towrap = tr.child(c, 1);
			tr.wrap(towrap, *sib);
			return true;
			});
		//tr.print_recursive_treeform(std::cerr, sib);
		// Move the component node up, outside the accent.
		sib=tr.flatten(sib);
		sib=tr.erase(sib);
		//tr.print_recursive_treeform(std::cerr, sib);
		return sib;
		}

	// We need to know for all indices whether they are free or dummy,
	// in particular to handle internal contractions correctly.
	index_map_t ind_free, ind_dummy;
	classify_indices(sib, ind_free, ind_dummy);

	// Pure scalar nodes need to be wrapped in a \component node to make life
	// easier for the rest of the algorithm.
	if(ind_free.size()==0 && ind_dummy.size()==0) {
		if(!tr.is_head(sib) && *tr.parent(sib)->name!="\\pow") {
			sib=wrap_scalar_in_components_node(sib);
#ifdef DEBUG
			std::cerr << "wrapped scalar" << std::endl;
#endif
			}
		return sib;
		}
	// If the indices are all Coordinates, this is a scalar, not a tensor.
	// It then needs simple wrapping just like a 'proper' scalar handed above.
	if(ind_dummy.size()==0 && ind_free.size()!=0) {
		bool all_coordinates=true;
		for(auto& ind: ind_free) {
			const Coordinate *crd = kernel.properties.get<Coordinate>(ind.second, true);
			if(!crd) {
				all_coordinates=false;
				break;
				}
			}
		if(all_coordinates) {
			if(!tr.is_head(sib) && *tr.parent(sib)->name!="\\pow") {
				sib=wrap_scalar_in_components_node(sib);
#ifdef DEBUG
				std::cerr << "wrapped scalar with component derivatives" << std::endl;
#endif
				}
			return sib;
			}
		}

	// Attempt to apply each component substitution rule on this term.
	Ex repl("\\components");
	for(auto& ind: ind_free)
		repl.append_child(repl.begin(), ind.second);
	// If there are no free indices, add an empty first child anyway,
	// otherwise we need special cases in various other places.
	auto vl = repl.append_child(repl.begin(), str_node("\\comma"));
	bool has_acted=false;
	cadabra::do_list(components, components.begin(), [&](Ex::iterator c) {
		Ex rule(c);
		Ex obj(sib);
		//			std::cerr << "attempting rule " << rule << " on " << obj << std::endl;
		// rule is a single rule, we walk the list.
		substitute subs(kernel, obj, rule);
		subs.comparator.set_value_matches_index(true);
		iterator oit=obj.begin();
		if(subs.can_apply(oit)) {
			has_acted=true;
			// std::cerr << "can apply" << std::endl;
			auto el = repl.append_child(vl, str_node("\\equals"));
			auto il = repl.append_child(el, str_node("\\comma"));
			auto fi = full_ind_free.begin();
			// FIXME: need to do something sensible with indices on the lhs
			// of rules which are not coordinates. You can have A_{m n} as expression,
			// A_{0 0} -> r, A_{i j} -> \delta_{i j} as rule, but at the moment the
			// second rule does not do the right thing.

			// Store all free indices (not the dummies!) in the component node.
			// If we have been passed an empty list of free indices (because the parent
			// node is not a sum node), simply add all free index values in turn.
			if(fi==full_ind_free.end()) {
				//					for(auto& r: subs.comparator.index_value_map) {
				//						repl.append_child(il, r.second.begin())->fl.parent_rel=str_node::p_none;
				//						}

				fi=ind_free.begin();
				while(fi!=ind_free.end()) {
					for(auto& r: subs.comparator.index_value_map) {
						if(fi->first == r.first) {
							// std::cerr << "adding " << r.second.begin() << std::endl;
							repl.append_child(il, r.second.begin())->fl.parent_rel=str_node::p_none;
							break;
							}
						}
					auto fiold(fi);
					while(fi!=ind_free.end() && fiold->first==fi->first)
						++fi;
					}
				}
			else {
				while(fi!=full_ind_free.end()) {
					for(auto& r: subs.comparator.index_value_map) {
						if(fi->first == r.first) {
							repl.append_child(il, r.second.begin())->fl.parent_rel=str_node::p_none;
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

			return true; // Cannot yet abort the do_list loop.
			}
		else {
			// TRACE: There is no rule which matches this factor. This means that
			// we want to keep all components?
			}
		return true;
		});
	if(!has_acted) {
		// There was not a single rule which matched for this tensor. That's means
		// that the user wants to keep the entire tensor (all components).
#ifdef DEBUG
		std::cerr << "No single rule matched " << Ex(sib) << std::endl;
#endif
		sib=dense_factor(sib, ind_free, ind_dummy);
		}
	else {
		merge_component_children(repl.begin());

#ifdef DEBUG
		std::cerr << "result now " << repl << std::endl;
#endif
		sib = tr.move_ontop(iterator(sib), repl.begin());
		}

	return sib;
	}

Ex::iterator evaluate::dense_factor(iterator it, const index_map_t& ind_free, const index_map_t& ind_dummy)
	{
	if(ind_dummy.size()!=0)
		throw RuntimeException("Cannot yet evaluate this expression.");

	// For each index we need to iterate over all possible values, and generate a
	// components node for it. This should be done 'on the fly' eventually, the way
	// python treats 'map', but that will require wrapping all access to
	// '\components' in a separate class.

	index_position_map_t ind_pos_free;
	fill_index_position_map(it, ind_free, ind_pos_free);

	Ex comp("\\components");

	auto fi = ind_free.begin();
	//std::cerr << "dense factor with indices: ";
	MultiIndex<Ex> mi;
	while(fi!=ind_free.end()) {
		comp.append_child(comp.begin(), fi->first.begin());
		// Look up which values this index takes.
		auto *id = kernel.properties.get<Indices>(fi->second);
		std::vector<Ex> values;
		if(!id || id->values.size()==0) {
			// No index property known, or not known which values the index
			// takes, so keep this index unexpanded.
			auto val=Ex(fi->second);
			val.begin()->fl.parent_rel=str_node::parent_rel_t::p_none;
			values.push_back(val);
			}
		else {
			for(const auto& ex: id->values)
				values.push_back(ex);
			}

		mi.values.push_back(values);
		++fi;
		}

	auto comma=comp.append_child(comp.begin(), str_node("\\comma"));

	// For each set of index values...
	for(mi.start(); !mi.end(); ++mi) {
		auto ivs  = comp.append_child(comma, str_node("\\equals"));
		auto ivsc = comp.append_child(ivs, str_node("\\comma"));
		// ... add the values of the indices.
		for(std::size_t i=0; i<mi.values.size(); ++i) {
			comp.append_child(ivsc, mi[i].begin());
			}
		// ... then set the value of the tensor component.
		auto repfac = comp.append_child(ivs, it);
		fi = ind_free.begin();
		size_t i=0;
		while(fi!=ind_free.end()) {
			auto il = begin_index(repfac);
			auto num = ind_pos_free[fi->second];
			il += num;
			auto ii = iterator(il);
			auto parent_rel = il->fl.parent_rel;
			comp.replace(ii, mi[i].begin())->fl.parent_rel=parent_rel;
			++fi;
			++i;
			}
		}

#ifdef DEBUG
	std::cerr << Ex(it) << std::endl;
#endif

	it=tr.move_ontop(it, comp.begin());

	return it;
	}

void evaluate::merge_component_children(iterator it)
	{
	// Scan the entries of a single \components node for those
	// which carry the same index value for the free indices.
	// Such things can arise from e.g. A_{m} A_{m n} and the
	// rule { A_{r}=3, A_{t}=5, A_{r t}=1, A_{t t}=2 }, which
	// leads to two entries for the free index n=t.

	//	if(*it->name!="\\components")
	//		std::cerr << "*** " << *it->name << std::endl;
	assert(*it->name=="\\components");

	auto comma=tr.end(it);
	--comma;

	//	if(*comma->name!="\\comma")
	//		std::cerr << "*** " << *comma->name << std::endl;
	assert(*comma->name=="\\comma");

	auto cv1=tr.begin(comma);  // equals node
	while(cv1!=tr.end(comma)) {
		auto iv1=tr.begin(cv1); // index values \comma
		auto cv2=cv1;
		++cv2;
		while(cv2!=tr.end(comma)) {
			auto iv2=tr.begin(cv2); // index values \comma
			if(tr.equal_subtree(iv1, iv2)) {
				// std::cerr << "merging " << Ex(iv1) << std::endl;
				Ex::sibling_iterator tv1=iv1; // tensor component value
				++tv1;
				Ex::sibling_iterator tv2=iv2;
				++tv2;
				// std::cerr << "need to merge" << std::endl;
				if(*tv1->name!="\\sum")
					tv1=tr.wrap(tv1, str_node("\\sum"));
				tr.append_child(tv1, tv2);
				cv2=tr.erase(cv2);
				}
			else ++cv2;
			}
		++cv1;
		}
	}

void evaluate::merge_components(iterator it1, iterator it2)
	{
	// Merge two component nodes which come from two terms in a sum (so that
	// we can be assured that the free indices match; they just may not be
	// in the same order).

#ifdef DEBUG
	std::cerr << "merge_components on " << Ex(it1) << " and " << Ex(it2) << std::endl;
#endif

	assert(*it1->name=="\\components");
	assert(*it2->name=="\\components");
	sibling_iterator sib1=tr.end(it1);
	--sib1;
	sibling_iterator sib2=tr.end(it2);
	--sib2;
	assert(*sib1->name=="\\comma");
	assert(*sib2->name=="\\comma");

	// We cannot directly compare the lhs of the equals nodes of it1
	// with the lhs of the equals node of it2, because the index order
	// on the two components nodes may be different. We first have to
	// ensure that the orders are the same (but only, of course) if we
	// have anything to permutate in the first place.

	if(*tr.begin(it1)->name!="\\comma") {
		// Look at all indices on the two components nodes. Find
		// the permutation that takes the indices on it2 and brings
		// them in the order as they are on it1.
		Perm perm;
		perm.find(tr.begin(it2), sib2, tr.begin(it1), sib1);

		// For each \equals node in the it2 comma node, permute
		// the values so they agree with the index order on it1.
		cadabra::do_list(tr, sib2, [&](Ex::iterator nd) {
			// nd is an \equals node.
			assert(*nd->name=="\\equals");
			auto comma = tr.begin(nd);
			assert(*comma->name=="\\comma");
			perm.apply(tr.begin(comma), tr.end(comma));
			return true;
			});

#ifdef DEBUG
		std::cerr << "permutations done" << std::endl;
#endif
		}

	// Now all index orders match and we can simply compare index value sets.

	cadabra::do_list(tr, sib2, [&](Ex::iterator it2) {
		assert(*it2->name=="\\equals");

		auto lhs2 = tr.begin(it2);
		auto found = cadabra::find_in_list(tr, sib1, [&](Ex::iterator it1) {

			auto lhs1 = tr.begin(it1);
			//std::cerr << "comparing " << *lhs1->name << " with " << *lhs2->name << std::endl;
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
		return true;
		});


	if(call_sympy)
		simplify_components(it1);
	}

void evaluate::cleanup_components(iterator it)
	{
	sibling_iterator sib=tr.end(it);
	--sib;

	cadabra::do_list(tr, sib, [&](Ex::iterator nd) {
		auto iv=tr.begin(nd);
		++iv;
		iterator p=iv;
		cleanup_dispatch(kernel, tr, p);
		return true;
		});
	}

Ex::iterator evaluate::handle_derivative(iterator it)
	{
#ifdef DEBUG
	std::cerr << "handle_derivative " << Ex(it) << std::endl;
#endif

	// In order to figure out which components to keep, we need to do two things:
	// expand into components the argument of the derivative, and then
	// figure out the dependence of that argument on the various coordinates.
	// There may be other orders (for e.g. situations where we want to keep entire
	// components unevaluated), but that's for later when we have practical use cases.

	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(sib->is_index()==false) {
			if(is_component(sib)==false) {
				index_map_t empty;
				// This really shouldn't be necessary; the way in which the
				// top level 'apply' works, it should have rewritten the argument
				// of the derivative into a \components node already.
				sib=handle_factor(sib, empty);
				}
			break;
			}
		++sib;
		}
	assert(sib!=tr.end(it));

	// std::cerr << "after handle\n" << Ex(it) << std::endl;

	index_map_t ind_free, ind_dummy;
	classify_indices(it, ind_free, ind_dummy);

	// Flag an error if a partial derivative has an upper index which
	// is not position=free: this would require converting the index
	// with a metric, and that should be done by the user using
	// rewrite_indices.

	auto fu = tr.begin(it);
	while(fu!=tr.end(it)) {
		if(fu->is_index() && fu->fl.parent_rel==str_node::p_super) {
			const Indices *ind = kernel.properties.get<Indices>(fu);
			if(ind && ind->position_type!=Indices::free)
				throw RuntimeException("All indices on derivatives need to be lowered first.");
			}
		++fu;
		}

	// Figure out the positions of the index values in the components
	// node inside the derivative which correspond to values of dummy
	// indices (these necessarily have the other dummy on the
	// derivative itself).
	std::vector<std::pair<size_t, size_t>> dummy_positions;

	decltype(ind_dummy.begin()) dumit[2];
	dumit[0] = ind_dummy.begin();
	while(dumit[0]!=ind_dummy.end()) {
		dumit[1]=dumit[0];
		++dumit[1];
		assert(dumit[1]!=ind_dummy.end());

		bool     on_component[2];
		iterator parents[2];
		for(int i=0; i<2; ++i) {
			parents[i]=tr.parent(dumit[i]->second);
			on_component[i]=*parents[i]->name=="\\components";
			}

		if(on_component[0]==false && on_component[1]==true)
			dummy_positions.push_back(std::make_pair(tr.index(dumit[0]->second), tr.index(dumit[1]->second)));
		else if(on_component[1]==false && on_component[0]==true)
			dummy_positions.push_back(std::make_pair(tr.index(dumit[1]->second), tr.index(dumit[0]->second)));

		++dumit[0];
		++dumit[0];
		}

	// Walk all the index value sets of the \components node inside the
	// \partial node.  For each, determine the dependencies, and
	// generate one element for each dependence.

	sibling_iterator ivalues = tr.end(sib);
	--ivalues;

	size_t ni=number_of_direct_indices(it);

	cadabra::do_list(tr, ivalues, [&](Ex::iterator iv) {
#ifdef DEBUG
		std::cerr << "====" << std::endl;
		std::cerr << Ex(iv) << std::endl;
#endif
		// For each internal dummy set, keep track of the
		// position in the permutation array where we generate
		// its value.
		std::map<Ex, int, tree_exact_less_for_indexmap_obj> d2p;

		sibling_iterator rhs = tr.begin(iv);
		++rhs;
		auto deps=dependencies(rhs);

		// If the argument does not depend on anything, all derivatives
		// would produce zero. Remove this \equals node from the tree.
		if(deps.size()==0) {
			tr.erase(iv);
			return true;
			}

		// All indices on \partial can take any of the values of the
		// dependencies, EXCEPT when the index is a dummy index OR
		// when the index on the partial is a Coordinate.
		//
		// In the 1st exceptional case, we firstly need to ensure
		// that both indices in the dummy pair take the same value
		// (this is done with d2p). Secondly, we need to ensure that
		// if the second index sits on the argument, we only use the
		// value of that index as given in the 'iv' list.
		//
		// In the 2nd exceptional case, we just need to determine if
		// the particular derivative does not annihilate the argument.

		// Need all combinations of values, with repetition (multiple
		// pick) allowed.

		combin::combinations<Ex> cb;
		for(auto& obj: deps) {
#ifdef DEBUG
			std::cerr << "dep " << obj << std::endl;
#endif
			cb.original.push_back(obj);
			}
		cb.multiple_pick=true;
		cb.block_length=1;
		for(size_t n=0; n<ni; ++n) {
			// If this child is a coordinate, take it out of the combinatorics.
			if(kernel.properties.get<Coordinate>(tr.child(it, n), true)!=0)
				continue;

			Ex iname(tr.child(it,n)); // FIXME: does not handle Accented objects
			if(ind_dummy.find(iname)!=ind_dummy.end()) {
				// If this dummy has one leg on the argument of the derivative,
				// take it out of the combinatorics, because its value will
				// be fixed.
				bool out=false;
				for(auto& d: dummy_positions)
					if(d.first==n) {
						out=true;
						break;
						}
				if(out) continue;

				if(d2p.find(iname)!=d2p.end())
					continue;
				else {
					d2p[iname]=cb.sublengths.size();
					}
				}
			cb.sublengths.push_back(1);
			}
		if(cb.sublengths.size()>0) // only if not all indices are fixed
			cb.permute();

#ifdef DEBUG
		std::cerr << cb.size() << " permutations of indices" << std::endl;
#endif

		// Note: indices on partial may be dummies, in which case the
		// values cannot be arbitrary. This is a self-contraction,
		// but cannot be caught by handle_factor because derivatives
		// do not get handled by patterns directly, they get
		// constructed by looking at dependencies.

		// For each index value set we constructed for the indices on the
		// derivative, create an entry in the \components node.

		for(unsigned int i=0; i<cb.size() || cb.size()==0; ++i) {
#ifdef DEBUG
			std::cerr << "Index combination " << i << std::endl;
#endif
			Ex eqcopy(iv);
			auto lhs=eqcopy.begin(eqcopy.begin());
			assert(*lhs->name=="\\comma");

			if(cb.size()>0) {
#ifdef DEBUG
				std::cerr << "Copying values of derivative indices" << std::endl;
#endif
				// Setup the index values; simply copy from the cb array, but only
				// if the indices are not internal dummy.
				for(size_t j=0; j<cb[i].size(); ++j) {
					auto fd = ind_dummy.find(Ex(tr.child(it, j)));
					if(fd==ind_dummy.end()) {
						eqcopy.append_child(iterator(lhs), cb[i][j].begin() );
						}
					}
				}
			auto rhs=lhs;
			++rhs;
			multiplier_t mult=*rhs->multiplier;
			one(rhs->multiplier);

			// Wrap a '\\partial' node around the component value, and add the
			// same index values as above to this node.
			rhs=eqcopy.wrap(rhs, str_node("\\partial"));
			multiply(rhs->multiplier, mult);
			multiply(rhs->multiplier, *it->multiplier);
			//				auto pch=tr.begin(it);
			//				iterator arg=tr.begin(rhs);
			for(size_t j=0, cb_j=0; j<ni; ++j) {
#ifdef DEBUG
				std::cerr << j << " : ";
#endif
				bool done=false;
				for(auto& d: dummy_positions) {
					if(d.first==j) {
						// This index is forced to a value because it is a dummy of which the partner
						// is fixed by the argument on which the derivative acts.
#ifdef DEBUG
						std::cerr << "fixed" << std::endl;
#endif
						eqcopy.insert_subtree(rhs.begin(), tr.child(lhs,d.second))->fl.parent_rel=str_node::p_sub;
						done=true;
						break;
						}
					}
				// std::cerr << "testing index " << j << " of \n" << Ex(it) << std::endl;
				if(kernel.properties.get<Coordinate>(tr.child(it, j), true)!=0) {
					// std::cerr << "Coordinate, so need straight copy" << std::endl;
					eqcopy.insert_subtree(rhs.begin(), tr.child(it,j))->fl.parent_rel=str_node::p_sub;
					done=true;
					}
				if(!done) {
					size_t fromj=cb_j;
					Ex iname(tr.child(it,j));
					auto fi = d2p.find(iname);
					if(fi!=d2p.end()) {
						fromj = (*fi).second;
						if(fromj == cb_j)
							++cb_j;
						}
					else {
						++cb_j;
						}
					// std::cerr << "cb: " << i << ", " << fromj << std::endl;
					eqcopy.insert_subtree(rhs.begin(), cb[i][fromj].begin() )->fl.parent_rel=str_node::p_sub;
					}
				}
			// std::cerr << "----" << std::endl;

			// For all dummy pairs which have one index on the
			// \components node inside the derivative, we need to
			// remove the corresponding value from the components
			// node.
			std::vector<sibling_iterator> sibs_to_erase;
			for(auto di: dummy_positions) {
				sibs_to_erase.push_back(tr.child(lhs, di.second));
				}
			for(auto se: sibs_to_erase)
				tr.erase(se);

			// Now move this replacement expression into the tree.

			// std::cerr << "Replacement now " << std::endl;
			// std::cerr << eqcopy << std::endl;
			tr.move_before(tr.begin(ivalues), eqcopy.begin());

			if(cb.size()==0) break;
			}

		// Erase the original \equals entry (we generated a full replacement above).
		tr.erase(iv);
		return true;
		});

#ifdef DEBUG
	std::cerr << tr.number_of_children(ivalues) << " nonzero components in this derivative" << std::endl;
#endif
	if(tr.number_of_children(ivalues)==0) {
		// All components of the derivative evaluated to zero because
		// there were no dependencies. Replace this derivative node with
		// a zero and return;
		node_zero(it);
		return it;
		}

	one(it->multiplier);

#ifdef DEBUG
	std::cerr << "now " << Ex(it) << std::endl;
#endif


	// Now move the free (but not the internal dummy or Coordinate!)
	//	partial indices to the components node, and then unwrap the
	//	partial node.

	auto pch=tr.begin(it);
	for(size_t n=0; n<ni; ++n) {
		sibling_iterator nxt=pch;
		++nxt;
		if(ind_dummy.find(Ex(pch))!=ind_dummy.end()) {
			tr.erase(pch);
			}
		else if(kernel.properties.get<Coordinate>(pch, true)!=0) {
			tr.erase(pch);
			}
		else
			tr.move_before(ivalues, pch);
		pch=nxt;
		}


	// Remove indices from the components node which came from the
	// argument and which are dummy.
	it=tr.flatten_and_erase(it);
	auto se = tr.begin(it);
	while(se!=tr.end(it)) {
		if(ind_dummy.find(Ex(se))!=ind_dummy.end())
			se = tr.erase(se);
		else
			++se;
		}

#ifdef DEBUG
	std::cerr << "after index move " << Ex(it) << std::endl;
#endif

	merge_component_children(it);

#ifdef DEBUG
	std::cerr << "after merge " << Ex(it) << std::endl;
#endif

	if(call_sympy)
		simplify_components(it);
	// std::cerr << "then " << Ex(it) << std::endl;

	return it;
	}

Ex::iterator evaluate::handle_epsilon(iterator it)
	{
	Ex rep("\\components");
	// attach indices to components
	// figure out the index value ranges
	// generate permutations of 'r1 ... rn' and signs
	// fill components
	auto sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		rep.append_child(rep.begin(), (iterator)sib);
		++sib;
		}
	auto cvals = rep.append_child(rep.begin(), str_node("\\comma"));

	sib=tr.begin(it);
	const Indices *ind = kernel.properties.get<Indices>(sib);
	if(ind==0)
		throw ArgumentException("No Indices property known for indices in EpsilonTensor.");

	combin::combinations<Ex> cb;
	for(auto& val: ind->values)
		cb.original.push_back(val);
	cb.multiple_pick=false;
	cb.block_length=1;
	cb.set_unit_sublengths();
	cb.permute();

	for(unsigned int i=0; i<cb.size(); ++i) {
		auto equals = rep.append_child(cvals, str_node("\\equals"));
		auto vcomma = rep.append_child(equals, str_node("\\comma"));
		for(unsigned int j=0; j<cb.original.size(); ++j) {
			//			std::cerr << *(cb[i][j].begin()->multiplier) << " ";
			rep.append_child(vcomma, cb[i][j].begin());
			}
		auto one = rep.append_child(equals, str_node("1"));
		multiply(one->multiplier, cb.ordersign(i));
		//		std::cerr << std::endl;
		}

	it=tr.move_ontop(it, rep.begin());
	return it;
	}

void evaluate::simplify_components(iterator it)
	{
	assert(*it->name=="\\components");

	// Simplify the components of the now single \component node by
	// calling the scalar backend.  We feed it the components
	// individually.
	sibling_iterator lst = tr.end(it);
	--lst;

	cadabra::simplify simp(kernel, tr);
	simp.set_progress_monitor(pm);

	cadabra::do_list(tr, lst, [&](Ex::iterator eqs) {
		assert(*eqs->name=="\\equals");
		auto rhs1 = tr.begin(eqs);
		++rhs1;
		iterator nd=rhs1;
		if(pm) pm->group("scalar_backend");
		// std::cerr << "simplify at " << Ex(nd) << std::endl;
		simp.apply_generic(nd, false, false, 0);
		if(pm) pm->group();

		if(nd->is_zero()) {
			// std::cerr << "component zero " << nd.node << std::endl;
			tr.erase(eqs);
			}
		else {
			// std::cerr << "component non-zero " << nd.node << std::endl;
			}
		return true;
		});

	// Note: the 'erase' in the loop above may have left us with a
	// \components node with an empty list of component values. However,
	// since all logic expects to find a \component node, we do NOT yet
	// replace this with a scalar zero here.
	}

std::set<Ex, tree_exact_less_obj> evaluate::dependencies(iterator it)
	{
	tree_exact_less_obj comp(&kernel.properties);
	std::set<Ex, tree_exact_less_obj> ret(comp);

	// Is this node a coordinate itself? If so, add it.
	const Coordinate *cd = kernel.properties.get<Coordinate>(it, true);
	if(cd) {
		Ex cpy(it);
		cpy.begin()->fl.bracket=str_node::b_none;
		cpy.begin()->fl.parent_rel=str_node::p_none;
		one(cpy.begin()->multiplier);
		ret.insert(cpy);
		}

	// Determine explicit dependence on Coordinates, that is, collect
	// parent_rel=p_none arguments, and add them directly if they
	// carry a Coordinate property, or run the algorithm recursively
	// if not (to catch e.g. exp(F(r)) depending on 'r'.

	cadabra::do_subtree(tr, it, [&](Ex::iterator nd) -> Ex::iterator {
		if(nd==it) return nd; // skip node itself, leads to indefinite recursion
		if(nd->fl.parent_rel==str_node::p_none)
			{
			const Coordinate *cd = kernel.properties.get<Coordinate>(nd, true);
			if(cd) {
				Ex cpy(nd);
				cpy.begin()->fl.bracket=str_node::b_none;
				cpy.begin()->fl.parent_rel=str_node::p_none;
				one(cpy.begin()->multiplier);
				ret.insert(cpy);
				}
			else {
				auto arg_deps=dependencies(nd);
				if(arg_deps.size()>0)
					for(const auto& new_dep: arg_deps)
						ret.insert(new_dep);
				}
			}
		return nd;
		});

	// Determine implicit dependence via Depends.
#ifdef DEBUG
	std::cerr << "deps for " << Ex(it) << std::endl;
#endif

	const DependsBase *dep = kernel.properties.get<DependsBase>(it, true);
	if(dep) {
#ifdef DEBUG
		std::cerr << "implicit deps" << std::endl;
#endif
		Ex deps(dep->dependencies(kernel, it));
		cadabra::do_list(deps, deps.begin(), [&](Ex::iterator nd) {
			Ex cpy(nd);
			cpy.begin()->fl.bracket=str_node::b_none;
			cpy.begin()->fl.parent_rel=str_node::p_none;
			ret.insert(cpy);
			return true;
			});
#ifdef DEBUG
		for(auto& e: ret)
			std::cerr << e << std::endl;
#endif
		}

	return ret;
	}

Algorithm::iterator evaluate::wrap_scalar_in_components_node(iterator sib)
	{
	// FIXME: would be good if we could write this in a more readable form.
	auto eq=tr.wrap(sib, str_node("\\equals"));
	tr.prepend_child(eq, str_node("\\comma"));
	eq=tr.wrap(eq, str_node("\\comma"));
	sib=tr.wrap(eq, str_node("\\components"));
	return sib;
	}

void evaluate::unwrap_scalar_in_components_node(iterator it)
	{
	// To apply to a scalar function: remove all scalars wrapped in
	// components nodes and make them normal scalars again.
	auto sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(*sib->name=="\\components") {
			iterator tmp=sib;
			::cleanup_components(kernel, tr, tmp);
			}
		++sib;
		}
	}

Ex::iterator evaluate::handle_prod(iterator it)
	{
	// std::cerr << "handle_prod " << Ex(it) << std::endl;

	std::string prod_name=*it->name;
	
	// All factors are either \component nodes, pure scalar nodes, or nodes which still need replacing.
	// The handle_factor function takes care of the latter two.

	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		sibling_iterator nxt=sib;
		++nxt;

		if(*sib->multiplier==0) { // zero factors make the entire product zero.
			node_zero(it);
			return it;
			}

		if(is_component(sib)==false) {
			index_map_t empty;
			handle_factor(sib, empty);
			}

		sib=nxt;
		}

	// TRACE: If a factor has not had a rule match, it will be left
	// un-evaluated here. So you get
	//  X^{a} \component_{a}( 0=3, 2=-5 )
	// and then we fail lower down. What we could do is let
	// handle_factor write out such unevaluated expressions to
	// component ones. That's somewhat wasteful though.

#ifdef DEBUG
	std::cerr << "every factor a \\components:\n" << Ex(it) << std::endl;
#endif

	// Now every factor in the product is a \component node.  The thing
	// is effectively a large sparse tensor product. We need to do the
	// sums over the dummy indices, turning this into a single
	// \component node.

	index_map_t ind_free, ind_dummy;
	classify_indices(it, ind_free, ind_dummy);

	auto di = ind_dummy.begin();
	// Since no factor can be a sum anymore, dummy indices always occur in pairs,
	// there is no need to account for anything more tricky. Every pair leads
	// to a sum.
	while(di!=ind_dummy.end()) {
		auto di2=di;
		++di2;
		int num1 = tr.index(di->second);
		int num2 = tr.index(di2->second);
		// std::cerr << *(di->first.begin()->name)
		// << " is index " << num1 << " in first and index " << num2 << " in second node " << std::endl;

		// three cases:
		//    two factors, single index in common. Merge is simple.
		//    two factors, more than one index in common. After merging this turns into:
		//    single factor, self-contraction

		auto cit1 = tr.parent(di->second);
		auto cit2 = tr.parent(di2->second);

		// Are the components objects cit1, cit2 on which these indices sit the same one?
		if(cit1 != cit2) {
			// Walk through all components of the first tensor, and for each check whether
			// any of the components of the second tensor matches the value for this dummy
			// index.
			sibling_iterator sib1=tr.end(cit1);
			--sib1;
			sibling_iterator sib2=tr.end(cit2);
			--sib2;

			// Move all indices of the second tensor to be indices of the first.
			sibling_iterator mv=tr.begin(cit2);
			while(mv!=sib2) {
				sibling_iterator nxt=mv;
				++nxt;
				tr.move_before(sib1, mv);
				mv=nxt;
				}

			cadabra::do_list(tr, sib1, [&](Ex::iterator it1) {
				if(*it1->name!="\\equals")
					std::cerr << *it->name << std::endl;
				assert(*it1->name=="\\equals");
				auto lhs1 = tr.begin(it1);
				auto ivalue1 = tr.begin(lhs1);
				ivalue1 += num1;
				cadabra::do_list(tr, sib2, [&](Ex::iterator it2) {
					assert(*it2->name=="\\equals");
					auto lhs2 = tr.begin(it2);
					auto ivalue2 = tr.begin(lhs2);
					ivalue2 += num2;

					// Compare the two index values in the two tensors, only continue if
					// these are the same.
					// std::cerr << "comparing value " << *ivalue1->name << " with " << *ivalue2->name << std::endl;
					// std::cerr << "                " << &(*ivalue1) << " vs " << &(*ivalue2) << std::endl;
					if(tr.equal_subtree(ivalue1,ivalue2)) {
						// Create new merged index value set.
						Ex ivs("\\equals");
						auto ivs_lhs = tr.append_child(ivs.begin(), str_node("\\comma"));
						auto ivs_rhs = tr.append_child(ivs.begin(), str_node(prod_name));
						auto ci = tr.begin(lhs1);
						int n=0;
						while(ci!=tr.end(lhs1)) {
							if(n!=num1)
								ivs.append_child(ivs_lhs, iterator(ci));
							++ci;
							++n;
							}
						ci = ivs.begin(lhs2);
						n=0;
						while(ci!=ivs.end(lhs2)) {
							if(n!=num2)
								ivs.append_child(ivs_lhs, iterator(ci));
							++ci;
							++n;
							}
						auto rhs1=lhs1;
						++rhs1;
						ivs.append_child(ivs_rhs, iterator(rhs1));
						auto rhs2=lhs2;
						++rhs2;
						ivs.append_child(ivs_rhs, iterator(rhs2));
						//std::cerr << "ivs_rhs = " << Ex(ivs_rhs) << std::endl;
						cleanup_dispatch_deep(kernel, ivs);
						// Insert this new index value set before sib1, so that it will not get used
						// inside the outer loop.
						tr.move_before(it1, ivs.begin());
						}
					return true;
					});
				// This index value set can now be erased as all
				// possible combinations have been considered.
				tr.erase(it1);
				return true;
				});
			// Remove the dummy indices from the index set of tensor 1.
			tr.erase(di->second);
			tr.erase(di2->second);
			// Tensor 2 can now be removed from the product as well, as all information is now
			// part of tensor 1.
			tr.erase(cit2);
			}

		else {
			// Components objects cit1 and cit2 are actually the same. We just need to
			// do a single loop now, going over all index value sets and keeping those
			// for which the num1-th and num2-th value are identical.

			sibling_iterator sib1=tr.end(cit1);
			--sib1;

			cadabra::do_list(tr, sib1, [&](Ex::iterator it1) {
				assert(*it1->name=="\\equals");
				auto lhs = tr.begin(it1);
				auto ivalue1 = tr.begin(lhs);
				auto ivalue2 = ivalue1;
				ivalue1 += num1;
				ivalue2 += num2;
				if(tr.equal_subtree(ivalue1,ivalue2)) {
					tr.erase(ivalue1);
					tr.erase(ivalue2);
					}
				else {
					tr.erase(it1);
					}
				return true;
				}
			                );
			tr.erase(di->second);
			tr.erase(di2->second);
			}

		++di;
		++di;
		}

	// TRACE: are we still ok here? Looks ok: one component node
	// with no indices.
	// std::cerr << "Before doing outer product:\n" << Ex(it) << std::endl;

	// At this stage we have one or more components nodes in the product,
	// and we have collected all possible index value combinations.
	// We need to do an outer multiplication, merging all index names into
	// one, and computing tensor component values for all possible index values.

	int n=tr.number_of_children(it);
	// std::cerr << "outer product:\n" << Ex(it) << std::endl;
	if(n>1) {
		//std::cerr << "merging" << std::endl;
		auto first=tr.begin(it); // component node
		auto other=first;
		++other;
		auto fi=tr.end(first);
		--fi;
		// Add the free indices of 'other' to 'first'.
		while(other!=tr.end(it)) {
			auto oi=tr.begin(other);
			while(oi!=tr.end(other)) {
				if(oi->is_index()==false)
					break;
				tr.insert_subtree(fi, oi);
				++oi;
				}
			++other;
			}

		// Now do an outer combination of all possible indexvalue/componentvalue
		// in the various component nodes.
		auto comma1=tr.end(first);
		--comma1;
		other=first;
		++other;
		while(other!=tr.end(it)) {
			Ex newcomma("\\comma"); // List of index value combinations and associated component values
			auto comma2=tr.end(other);
			--comma2;
			assert(*comma1->name=="\\comma");
			assert(*comma2->name=="\\comma");
			auto eq1=tr.begin(comma1);    // The \equals node
			while(eq1!=tr.end(comma1)) {
				auto eq2=tr.begin(comma2);
				while(eq2!=tr.end(comma2)) {
					// Collect all index values.
					auto neq = newcomma.append_child(newcomma.begin(), str_node("\\equals"));
					auto ncm = newcomma.append_child(neq, str_node("\\comma")); // List of index values
					auto iv=tr.begin(tr.begin(eq1));
					while(iv!=tr.end(tr.begin(eq1))) {
						newcomma.append_child(ncm, iterator(iv));
						++iv;
						}
					iv=tr.begin(tr.begin(eq2));
					while(iv!=tr.end(tr.begin(eq2))) {
						newcomma.append_child(ncm, iterator(iv));
						++iv;
						}
					// Multiply component values.
					Ex prod(*it->name);
					iv=tr.end(eq1);
					--iv;
					prod.append_child(prod.begin(), iterator(iv));
					iv=tr.end(eq2);
					--iv;
					prod.append_child(prod.begin(), iterator(iv));
					cleanup_dispatch_deep(kernel, prod);
					newcomma.append_child(neq, prod.begin());
					++eq2;
					}
				++eq1;
				}
			// Now replace the original comma1 node with newcomma, and re-iterate if there
			// are further factors in the tensor product.
			comma1 = tr.move_ontop(iterator(comma1), newcomma.begin());
			other=tr.erase(other);
			}
		//		std::cerr << Ex(it) << std::endl;
		}

	// At this stage, there should be only one factor in the product, which
	// should be a \components node. We do a cleanup, after which it should be
	// at the 'it' node.

	assert(*it->name=="\\prod" || *it->name=="\\wedge" || *it->name=="\\frac");
	assert(tr.number_of_children(it)==1);
	assert(*tr.begin(it)->name=="\\components");
	tr.begin(it)->fl.bracket=it->fl.bracket;
	tr.begin(it)->fl.parent_rel=it->fl.parent_rel;
	tr.begin(it)->multiplier=it->multiplier;
	tr.flatten(it);
	it=tr.erase(it);
	push_down_multiplier(kernel, tr, it);

	//	iterator pr=tr.end();
	//	if(tr.is_head(it)==false) {
	//		pr=tr.parent(it);
	//		std::cerr << "Tracing just before merge:\n " << Ex(pr) << std::endl;
	//		}

	merge_component_children(it);

	//	if(pr!=tr.end())
	//		std::cerr << "after component merge:\n " << Ex(pr) << std::endl;

	//	cleanup_dispatch(kernel, tr, it);
	//	if(pr!=tr.end())
	//		std::cerr << "And afterwards:\n " << Ex(pr) << std::endl;

	if(*it->name!="\\components") {
		// The result is a scalar. Because we are expected to return
		// a \components node, we wrap this scalar again.
		// std::cerr << "wrapping scalar" << std::endl;
		it=wrap_scalar_in_components_node(it);
		// std::cerr << Ex(it) << std::endl;
		}

	//	else {
	//		// We may have duplicate index value entries; merge them.
	//		merge_component_children(it);
	//		}

	// Use sympy to simplify components.
	if(call_sympy)
		simplify_components(it);
	//std::cerr << "simplified:\n" << Ex(it) << std::endl;

	return it;
	}
