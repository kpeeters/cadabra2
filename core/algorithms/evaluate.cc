
#include "Functional.hh"
#include "Cleanup.hh"
#include "algorithms/evaluate.hh"
#include "algorithms/substitute.hh"
#include "properties/PartialDerivative.hh"
#include "properties/Coordinate.hh"
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
	
	cadabra::do_subtree(tr, it, [&](Ex::iterator walk) {
			if(*(walk->name)=="\\sum")       handle_sum(walk);
			else if(*(walk->name)=="\\prod") handle_prod(walk);
			
			const PartialDerivative *pd = kernel.properties.get<PartialDerivative>(walk);
			if(pd) handle_derivative(walk);
			}
		);

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

void evaluate::handle_factor(sibling_iterator& sib, const index_map_t& full_ind_free)
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
						repl.append_child(il, r.second.begin())->fl.parent_rel=str_node::p_none; 
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
				// FIXME: this wastes time, as we could now simply exit do_list,
				// but there is no mechanism for that.
				}
			});


	sib = tr.move_ontop(iterator(sib), repl.begin());
	}

void evaluate::merge_components(iterator it1, iterator it2)
	{
	sibling_iterator sib1=tr.end(it1);
	--sib1;
	sibling_iterator sib2=tr.end(it2);
	--sib2;

	cadabra::do_list(tr, sib2, [&](Ex::iterator it2) {
			assert(*it2->name=="\\equals");

			// We cannot directly compare the lhs of this equals node with the lhs
			// of the equals node of the other components node, because the index
			// order on the two components nodes may be different.

			auto lhs2 = tr.begin(it2);
			auto found = cadabra::find_in_list(tr, sib1, [&](Ex::iterator it1) {

					// First determine the permutation needed to put the indices on
					// it2 in the order of it1.

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

void evaluate::handle_derivative(iterator it)
	{
	// In order to figure out which components to keep, we need to do two things:
	// expand into components the argument of the derivative, and then
	// figure out the dependence of that argument on the various coordinates.
	// There may be other orders (for e.g. situations where we want to keep entire
	// components unevaluated), but that's for later when we have practical use cases.
	
	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(sib->is_index()==false) {
			index_map_t empty;
			handle_factor(sib, empty);
			break;
			}
		++sib;
		}
	assert(sib!=tr.end(it));
	
	// Walk all the index value sets of the \components node inside the
	// argument.  For each, determine the dependencies, and generate
	// one element for each dependence.

	sibling_iterator ivalues = tr.end(sib);
	--ivalues;

	cadabra::do_list(tr, ivalues, [&](Ex::iterator iv) {
			sibling_iterator rhs = tr.begin(iv);
			++rhs;
			auto deps=dependencies(rhs);

			// FIXME: all indices on \partial can take any of the values of the 
			// dependencies. Need all permutations. 
			assert(number_of_direct_indices(it)==1);

			for(auto& obj: deps) {
				Ex eqcopy(iv);
				auto lhs=eqcopy.begin(eqcopy.begin());
				assert(*lhs->name=="\\comma");
				eqcopy.append_child(iterator(lhs), obj.begin());
				++lhs;
				lhs=eqcopy.wrap(lhs, str_node("\\partial"));
				auto pch=tr.begin(it);
				auto arg=tr.begin(lhs);
				// FIXME: as above: need all permutations.
				while(pch!=tr.end(it)) {
					if(pch->is_index()) 
						eqcopy.insert_subtree(arg, obj.begin())->fl.parent_rel=str_node::p_sub;
					++pch;
					}
				tr.move_before(tr.begin(ivalues), eqcopy.begin());
				}
			tr.erase(iv);
			});

	// Now move the partial indices to the components node, and then unwrap the
	// partial node.
	auto pch=tr.begin(it);
	while(pch!=tr.end(it)) {
		tr.move_before(ivalues, pch);
		++pch;
		}
	it=tr.flatten_and_erase(it);

	tr.print_recursive_treeform(std::cerr, tr.begin());
	}

std::set<Ex, tree_exact_less_obj> evaluate::dependencies(iterator it)
	{
	tree_exact_less_obj comp(&kernel.properties);
	std::set<Ex, tree_exact_less_obj> ret(comp);

	cadabra::do_subtree(tr, it, [&](Ex::iterator nd) {
			// FIXME: this does not yet take into account implicit dependence through
			// the Depends property.
			const Coordinate *cd = kernel.properties.get<Coordinate>(nd);
			if(cd) {
				Ex cpy(nd);
				cpy.begin()->fl.bracket=str_node::b_none;
				cpy.begin()->fl.parent_rel=str_node::p_none;
				ret.insert(cpy);
				}
			});

	return ret;
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
		auto di2=di;
		++di2;
		int num1 = tr.index(di->second);
		int num2 = tr.index(di2->second);
		std::cerr << " is index " << num1 << " in first and index " << num2 << " in second node " << std::endl;

		// three cases:
		//    two factors, single index in common. Merge is simple.
		//    two factors, more than one index in common. After merging this turns into:
		//    single factor, self-contraction

		auto cit1 = tr.parent(di->second);
		auto cit2 = tr.parent(di2->second);

		// Are the components objects cit1, cit2 on which these indices sit the same one?
		if(cit1 != cit2) {
			std::cerr << "different tensors" << std::endl;

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
							std::cerr << "comparing value " << *ivalue1->name << " with " << *ivalue2->name << std::endl;
							std::cerr << "                " << &(*ivalue1) << " vs " << &(*ivalue2) << std::endl;
							if(tr.equal_subtree(ivalue1,ivalue2)) {
								// Create new merged index value set.
								Ex ivs("\\equals");
								auto ivs_lhs = tr.append_child(ivs.begin(), str_node("\\comma"));
								auto ivs_rhs = tr.append_child(ivs.begin(), str_node("\\prod"));
								auto ci = tr.begin(lhs1);
								int n=0;
								while(ci!=tr.end(lhs1)) {
									if(n!=num1)
										ivs.append_child(ivs_lhs, iterator(ci));
									++ci; ++n;
									}
								ci = ivs.begin(lhs2);
								n=0;
								while(ci!=ivs.end(lhs2)) {
									if(n!=num2) 
										ivs.append_child(ivs_lhs, iterator(ci));
									++ci; ++n;
									}
								auto rhs1=lhs1;
								++rhs1;
								ivs.append_child(ivs_rhs, iterator(rhs1));
								auto rhs2=lhs2;
								++rhs2;
								ivs.append_child(ivs_rhs, iterator(rhs2));								
								cleanup_dispatch(kernel, ivs, ivs_rhs);
								// Insert this new index value set before sib1, so that it will not get used
								// inside the outer loop.
								tr.move_before(it1, ivs.begin());
								}
							});
					// This index value set can now be erased as all
					// possible combinations have been considered.
					tr.erase(it1);
					});
			// Remove the dummy indices from the index set of tensor 1.
			tr.erase(di->second);
			tr.erase(di2->second);
			// Tensor 2 can now be removed from the product as well, as all information is now
			// part of tensor 1.
			tr.erase(cit2);
			tr.print_recursive_treeform(std::cerr, tr.begin());
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
						std::cerr << "keeping " << std::endl;
						tr.erase(ivalue1);
						tr.erase(ivalue2);
						}
					else {
						std::cerr << "discarding " << std::endl;
						tr.erase(it1);
						}
					}
				);
			tr.erase(di->second);
			tr.erase(di2->second);
			tr.print_recursive_treeform(std::cerr, tr.begin());
			}			

		++di; ++di;
		}
	}
