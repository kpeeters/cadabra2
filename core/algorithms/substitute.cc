
#include <sstream>
#include "Cleanup.hh"
#include "Functional.hh"
#include "algorithms/substitute.hh"
#include "properties/Indices.hh"

#define DBG_MACRO_NO_WARNING 
#define DBG_MACRO_DISABLE
#include "dbg.h"

//#define DEBUG 1

using namespace cadabra;

substitute::Rules substitute::replacement_rules = substitute::Rules();
size_t substitute::cache_hits   = 0;
size_t substitute::cache_misses = 0;

substitute::substitute(const Kernel& k, Ex& tr, Ex& args_, bool partial)
	: Algorithm(k, tr), comparator(k.properties), args(args_), sort_product_(k, tr), partial(partial)
	{
	if(args.is_empty()) 
		throw ArgumentException("substitute: Replacement rule is an empty expression.");

	// Stopwatch sw;
	// sw.start();

	// Check if args are present in global_rules
	// bool skipchecks = k.replacement_rules->is_present(args);
	bool skipchecks = replacement_rules.is_present(args);
	if(skipchecks) ++cache_hits;
	else           ++cache_misses;

	// std::cerr << cache_hits << " vs " << cache_misses << std::endl;
	
	// skip if args have already been checked
	if (!skipchecks) {
		cadabra::do_list(args, args.begin(), [&](Ex::iterator arrow) {
			//args.print_recursive_treeform(std::cerr, arrow);
			if(*arrow->name!="\\arrow" && *arrow->name!="\\equals")
				throw ArgumentException("substitute: Argument is neither a replacement rule nor an equality.");

			sibling_iterator lhs=args.begin(arrow);
			sibling_iterator rhs=lhs;
			rhs.skip_children();
			++rhs;

			if(*lhs->name=="") { // replacing a sub or superscript
				lhs=tr.flatten_and_erase(lhs);
				}
			if(*rhs->name=="") { // replacing with a sub or superscript
				rhs=tr.flatten_and_erase(rhs);
				}

			try {
				if(*lhs->multiplier!=1) {
					throw ArgumentException("substitute: No numerical pre-factors allowed on lhs of replacement rule.");
					}
				// test validity of lhs and rhs
				iterator lhsit=lhs, stopit=lhs;
				stopit.skip_children();
				++stopit;
				while(lhsit!=stopit) {
					if(lhsit->is_object_wildcard()) {
						if(tr.number_of_children(lhsit)>0) {
							throw ArgumentException("substitute: Object wildcards cannot have child nodes.");
							}
						}
					++lhsit;
					}
				lhsit=rhs;
				stopit=rhs;
				stopit.skip_children();
				++stopit;
				while(lhsit!=stopit) {
					if(lhsit->is_object_wildcard()) {
						if(tr.number_of_children(lhsit)>0) {
							throw ArgumentException("substitute: Object wildcards cannot have child nodes.");
							}
						}
					++lhsit;
					}

				// check whether there are dummies.
				index_map_t ind_free, ind_dummy;
				classify_indices(lhs, ind_free, ind_dummy);
				lhs_contains_dummies[arrow]= ind_dummy.size()>0;
				ind_free.clear();
				ind_dummy.clear();
				if(rhs!=tr.end()) {
					classify_indices(rhs, ind_free, ind_dummy);
					rhs_contains_dummies[arrow]=ind_dummy.size()>0;
					}
				}
			catch(std::exception& er) {
				throw ArgumentException(std::string("substitute: Index error in replacement rule. ")+er.what());
				}
			return true;
			});
		replacement_rules.store(args, lhs_contains_dummies, rhs_contains_dummies);
		}
	else {
		replacement_rules.retrieve(args, lhs_contains_dummies, rhs_contains_dummies);
		}

	// sw.stop();
	// std::cerr << "preparation took " << sw << std::endl;
	}

bool substitute::can_apply(iterator st)
	{
	// std::cerr << "attempting to match at " << st << std::endl;

	Ex::iterator found = cadabra::find_in_list(args, args.begin(), [&](Ex::iterator arrow) {
		comparator.clear();
		iterator lhs=tr.begin(arrow);
		if(*lhs->name=="\\conditional") {
			lhs=tr.begin(lhs);
			conditions=lhs;
			conditions.skip_children();
			++conditions;
			}
		else conditions=tr.end();

		if(lhs->name!=st->name && !lhs->is_object_wildcard() && !lhs->is_name_wildcard() && lhs->name->size()>0)
			return args.end();

		Ex_comparator::match_t ret;
		comparator.lhs_contains_dummies=lhs_contains_dummies[arrow];
		// std::cerr << "lhs_contains_dummies " << comparator.lhs_contains_dummies << std::endl;

		//	HERE: we need to have one entry point for matching, which dispatches depending
		// on whether we have a normal node, a product, a sum or a sibling range with
		// sibling wildcards. We also need a simple notation (and an exception at top
		// level for plus and prod).
		//
		//	> ex:=A+B+C+D;
		// A + B + C + D
		// > substitute(_, $B+C -> Q$)

		if(*lhs->name=="\\prod")     ret=comparator.match_subproduct(tr, lhs, tr.begin(lhs), st, conditions);
		else if(*lhs->name=="\\sum") ret=comparator.match_subsum(tr, lhs, tr.begin(lhs), st, conditions);
		else {
#ifdef DEBUG
			std::cerr << "substitute::can_apply: testing " << *lhs << " against " << *st << std::endl;
#endif
			ret=comparator.match_subtree(tr, lhs, st, conditions);
			}

		if(ret == Ex_comparator::match_t::subtree_match ||
			ret == Ex_comparator::match_t::match_index_less ||
			ret == Ex_comparator::match_t::match_index_greater) {
			use_rule=arrow;

			// If we are not matching a partial sum or partial product, need to check that all
			// terms or factors are accounted for.
			if(!partial) {
				if(*lhs->name=="\\prod") {
					if(*st->name!="\\prod")
						return args.end();
					
#ifdef DEBUG
					std::cerr << "substitute::can_apply: partial=false, so " 
								 << comparator.factor_locations.size()
								 << " has to equal "
								 << tr.number_of_children(st) << std::endl;
#endif
					if(comparator.factor_locations.size()!=tr.number_of_children(st))
						return args.end();
					}
				else {
					// If lhs is not a product, then we need to check that the node in the
					// expression we act on does not sit inside a product (and hence has
					// more factors).
					if(tr.is_head(st) || (*tr.parent(st)->name)!="\\prod")
						return arrow;
					else
						return args.end();
					}
				}

			return arrow;
			}

		return args.end();
		});
	//	if(found!=args.end())
	//		std::cerr << "rule working: " << Ex(found) << std::endl;
	//	else
	//		std::cerr << "rule not working, going to return " << (found!=args.end()) << std::endl;

	return found!=args.end();
	}

Algorithm::result_t substitute::apply(iterator& st)
	{
#ifdef DEBUG
	std::cerr << "substitute::apply at " << Ex(st) << std::endl;
#endif

//	dbg(comparator.replacement_map);
//	for(auto& rule: comparator.replacement_map)
//		std::cerr << "* " << rule.first << " -> " << rule.second << std::endl;

	sibling_iterator arrow=use_rule;
	iterator lhs=tr.begin(arrow);
	iterator rhs=lhs;
	rhs.skip_children();
	++rhs;
	if(*lhs->name=="\\conditional")
		lhs=tr.begin(lhs);

	// We construct a new tree 'repl' which is a copy of the rhs of the
	// replacement rule, and then replace nodes and subtrees in there
	// based on how the pattern matching went.
	Ex repl(rhs);
	index_map_t ind_free, ind_dummy, ind_forced;

	if(rhs_contains_dummies[use_rule]) {
		classify_indices(repl.begin(), ind_free, ind_dummy);
		//std::cerr << "rhs contains dummies " << ind_dummy.size() << std::endl;
		}
	else {
		//std::cerr << "rhs does not contain dummies" << std::endl;
		}

	// Replace all patterns on the rhs of the rule with the objects they matched.
	// Keep track of all indices which _have_ to stay what they are, in ind_forced.
	// Keep track of insertion points of subtrees.
	iterator it=repl.begin();
	Ex_comparator::replacement_map_t::iterator loc;
	Ex_comparator::subtree_replacement_map_t::iterator sloc;
	std::vector<iterator> subtree_insertion_points;
	while(it!=repl.end()) {
		bool is_stripped=false;

		// For some reason 'a?' is not found!?! Well, that's presumably because _{a?} does not
		// match ^{a?}. (though this does match when we write 'i' instead of a?.

		loc=comparator.replacement_map.find(Ex(it));
		if(loc==comparator.replacement_map.end() && it->is_name_wildcard() && tr.number_of_children(it)!=0) {
			Ex tmp(it);
			tmp.erase_children(tmp.begin());
			loc=comparator.replacement_map.find(tmp);
			is_stripped=true;
			}

		//std::cerr << "consider element of repl " << Ex(it) << std::endl;

		if(loc!=comparator.replacement_map.end()) { // name wildcards
#ifdef DEBUG
			std::cerr << "wildcard replaced: " << loc->first << " -> " << loc->second << std::endl;
#endif

			// When a replacement is made here, and the index is actually
			// a dummy in the replacement, we screw up the ind_dummy
			// map. Then, at the next step, when conflicting dummies are
			// relabelled, things go wrong.  Solution: in this case, the
			// index under consideration should be taken out of ind_dummy.
			// This is easy, because we can just throw out all indices
			// with the original name.

			ind_dummy.erase(Ex(it));

			str_node::bracket_t remember_br=it->fl.bracket;
			if(is_stripped || (it->is_name_wildcard() && !it->is_index()) ) {
				// a?_{i j k} type patterns should only replace the head
				// TODO: should we replace brackets here too?
				it->name=(*loc).second.begin()->name;
				multiply(it->multiplier, *(*loc).second.begin()->multiplier);
				it->fl=(*loc).second.begin()->fl;
				// std::cerr << "replaced: \n" << it << std::endl;
				}
			else {
				// Careful with the multiplier: the object has been matched to the pattern
				// without taking into account the top-level multiplier. So keep the multiplier
				// of the thing we are replacing.
				multiplier_t mt=*it->multiplier;
				it=tr.replace_index(it, (*loc).second.begin()); //, true);
				multiply(it->multiplier, mt);
				}
			it->fl.bracket=remember_br;
			if(rhs_contains_dummies[use_rule])
				ind_forced.insert(index_map_t::value_type(Ex(it), it));
			++it;

			}
		else if( (sloc=comparator.subtree_replacement_map.find(it->name))
		         !=comparator.subtree_replacement_map.end()) { // object wildcards
			//std::cerr << "srule : " << Ex(it) << std::endl;
			multiplier_t tmpmult=*it->multiplier; // remember target multiplier
			iterator tmp= tr.insert_subtree(it, (*sloc).second);
#ifdef DEBUG
			std::cerr << "subtree replaced: " << repl << std::endl;
#endif
			tmp->fl.bracket=it->fl.bracket;
			tmp->fl.parent_rel=it->fl.parent_rel; // ok?
			it=tr.erase(it);
			multiply(tmp->multiplier, tmpmult);
#ifdef DEBUG
			std::cerr << "subtree replaced 2: " << repl << std::endl;
#endif
			subtree_insertion_points.push_back(tmp);
			index_map_t ind_subtree_free, ind_subtree_dummy;
			// FIXME: as in the name wildcard case above, we only need these
			// next three lines if there are wildcards in the rhs.
			classify_indices(tmp, ind_subtree_free, ind_subtree_dummy);
			ind_forced.insert(ind_subtree_free.begin(), ind_subtree_free.end());
			ind_forced.insert(ind_subtree_dummy.begin(), ind_subtree_dummy.end());
			}
		else ++it;
		}

	// If the replacement contains dummies, avoid clashes introduced when
	// free indices in the replacement (induced from the original expression)
	// take values already used for the dummies.
	//
	// Note: the dummies which clash with other factors in a product are
	// not replaced here, but rather in the next step.
	// std::cerr << ind_dummy.size() << std::endl;
	if(ind_dummy.size()>0) {
#ifdef DEBUG
		std::cerr << "avoid dummy clashes" << std::endl;
#endif
		index_map_t must_be_empty;
		determine_intersection(ind_forced, ind_dummy, must_be_empty);
		index_map_t::iterator indit=must_be_empty.begin();
		index_map_t added_dummies;
		// std::cerr << must_be_empty.size() << " dummies have to be relabelled" << std::endl;
		while(indit!=must_be_empty.end()) {
			Ex the_key=indit->first;
			const Indices *dums=kernel.properties.get<Indices>(indit->second, true);
			if(dums==0) {
				std::ostringstream str;
				str << "Need to know an index set for " << Ex(*indit->second) << ".";
				throw ConsistencyException(str.str());
				}
			Ex relabel=get_dummy(dums, &ind_dummy, &ind_forced, &added_dummies);
			added_dummies.insert(index_map_t::value_type(relabel,(*indit).second));
			do {
				// std::cerr << "replace index " << *(indit->second->name) << " with " << *(relabel.begin()->name) << std::endl;
				tr.replace_index(indit->second,relabel.begin(), true);
				++indit;
				//				txtout << *(indit->first.begin()->name) << " vs " << *(the_key.begin()->name) << std::endl;
				}
			while(indit!=must_be_empty.end() && tree_exact_equal(&kernel.properties, indit->first,the_key,-1));
			}
		}

	// After all replacements have been done, we need to cleanup the
	// replacement tree.

#ifdef DEBUG
	std::cerr << repl << std::endl;
#endif
	
	cleanup_dispatch_deep(kernel, repl);

#ifdef DEBUG
	std::cerr << "after cleanup:\n" << repl << std::endl;
#endif

	repl.begin()->fl.bracket=st->fl.bracket;
	bool rename_replacement_dummies_called=false;

	// Now we do the actual replacement, putting the "repl" in the tree.
	// If the to-be-replaced object sits in a product, we have to relabel all
	// dummy indices in the replacement which clash with indices in other factors
	// in the product.
	if(*lhs->name=="\\prod") {
		for(unsigned int i=1; i<comparator.factor_locations.size(); ++i)
			tr.erase(comparator.factor_locations[i]);

		// no need to keep repl
		iterator newtr=tr.move_ontop(iterator(comparator.factor_locations[0]),repl.begin());
		multiply(st->multiplier, *newtr->multiplier);
		one(newtr->multiplier);
		if(ind_dummy.size()>0) {
			rename_replacement_dummies(newtr); // do NOW, otherwise the replacement cannot be isolated anymore
			rename_replacement_dummies_called=true;
			}
		if(*rhs->name=="\\prod" && *newtr->name=="\\prod") {
			tr.flatten(newtr);
			tr.erase(newtr);
			}
		if(tr.number_of_children(st)==1) {
			multiply(tr.begin(st)->multiplier, *st->multiplier);
			tr.flatten(st);
			st=tr.erase(st);
			}
		}
	else if(*lhs->name=="\\sum") {
		for(unsigned int i=1; i<comparator.factor_locations.size(); ++i)
			tr.erase(comparator.factor_locations[i]);

		multiply(repl.begin()->multiplier, 1/comparator.term_ratio);

		// no need to keep repl
		iterator newtr=tr.move_ontop(iterator(comparator.factor_locations[0]),repl.begin());
		//		multiply(st->multiplier, *newtr->multiplier);
		//		one(newtr->multiplier);
		if(ind_dummy.size()>0) {
			rename_replacement_dummies(newtr); // do NOW, otherwise the replacement cannot be isolated anymore
			rename_replacement_dummies_called=true;
			}

		}
	else {
#ifdef DEBUG
		std::cerr << "move " << repl << " on top of " << st << std::endl;
#endif
		multiply(repl.begin()->multiplier, *st->multiplier);
		auto keep_parent_rel=st->fl.parent_rel;
		st=tr.move_ontop(st, repl.begin()); // no need to keep the original repl tree
		st->fl.parent_rel=keep_parent_rel;
		}

	if(ind_dummy.size()>0 && !rename_replacement_dummies_called)
		rename_replacement_dummies(st);

	// The replacement is done now.  What is left is to take into
	// account any signs caused by moving factors through each other
	int totsign=1;
	for(unsigned int i=0; i<comparator.factor_moving_signs.size(); ++i) {
		totsign*=comparator.factor_moving_signs[i];
		dbg(i);
		dbg(comparator.factor_moving_signs[i]);
		}
	multiply(st->multiplier, totsign);

	//	// Get rid of numerical '1' factors inside products (this will not clean up
	//	// '1's from a 'q -> 1' type replacement, since in this case 'st' points to the 'q'
	//   // node and we are not allowed to touch the tree above the entry point; these
	//	// things are taken care of by the algorithm class itself).
	//	// FIXME: still needed?
	//	cleanup_dispatch(kernel, tr, st);

#ifdef DEBUG
	std::cerr << tr << std::endl;
#endif

	dbg(tr.begin());
	dbg(subtree_insertion_points.size());

	// Cleanup nests on all insertion points and on the top node.
//	for(unsigned int i=0; i<subtree_insertion_points.size(); ++i) {
//		iterator ip=subtree_insertion_points[i];
//		//std::cerr << *ip->name << std::endl;
//		cleanup_dispatch(kernel, tr, ip);
//		}
//
	cleanup_dispatch(kernel, tr, st);

	dbg("complete");

	return result_t::l_applied;
	}


size_t substitute::cache_size()
	{
	return replacement_rules.size();
	}

void substitute::Rules::store(Ex& rules,
								std::map<iterator, bool>& lhs_contains_dummies,
								std::map<iterator, bool>& rhs_contains_dummies)
	{
	try {
		// if number of stored rules has grown large, clean them up.
		if (properties.size() >= cleanup_threshold) {
			cleanup();
			}
		// If that didn't fix it, double the cleanup_threshold up to max_size
		if (cleanup_threshold != max_size && properties.size() >= cleanup_threshold) {
			if (cleanup_threshold * 2 < max_size) {
				cleanup_threshold *= 2;
				} 
			else {
				cleanup_threshold = max_size;
				}
			}

		std::weak_ptr<Ex> rules_ptr = rules.shared_from_this();
		// properties[rules_ptr] = { lhs_contains_dummies, rhs_contains_dummies };'
		properties.insert(rules_ptr, { lhs_contains_dummies, rhs_contains_dummies });
		// Mark this expression as cached; any change will remove that state
		// and ensure that we do not use the cached expression later.
		rules.update_state(result_t::l_cached);
		}
	catch(const std::bad_weak_ptr& error) {
		return;
		}
	}

void substitute::Rules::retrieve(Ex& rules,
								std::map<iterator, bool>& lhs_contains_dummies,
								std::map<iterator, bool>& rhs_contains_dummies) const
	{
	// Rules::present is assumed to have been called to check that the rules are valid
	std::weak_ptr<Ex> rules_ptr = rules.shared_from_this();
	lhs_contains_dummies = properties.at(rules_ptr).first;
	rhs_contains_dummies = properties.at(rules_ptr).second;
	}


bool substitute::Rules::is_present(Ex& rules) const
	{
	// Look to see if the rules are present in the map
	
	try {
		std::weak_ptr<Ex> rules_ptr = rules.shared_from_this();
		auto rule_it = properties.find(rules_ptr);
		bool rule_found = (rule_it != properties.end());
		if (!rule_found) return false;
		
		// rules should have l_cached set
		bool rule_unchanged = (rules.state() == result_t::l_cached);
		
		// If rule has been changed, erase it.
		if (!rule_unchanged) {
			properties.erase(rule_it);
			return false;
			} 
		else {
			return true;
			}
		}
	catch(const std::bad_weak_ptr& error) {
		return false;
		}
	}

size_t substitute::Rules::size() const
	{
	return properties.size();
	}

void substitute::Rules::cleanup()
	{
	// Erase rules that are pointing to garbage-collected Ex expressions
	// or rules that have possibly been changed.
	for (auto it = properties.begin(); it != properties.end(); ) {
		if (it->first.expired()) {
			it = properties.erase(it);
			} 
		else if (it->first.lock()->state() != result_t::l_cached) {
			it = properties.erase(it);
			}
		else {
			++it;
			}
		}
	}
