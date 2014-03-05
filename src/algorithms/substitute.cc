
#include "Cleanup.hh"
#include "algorithms/substitute.hh"
#include "algorithms/prodcollectnum.hh"
#include "properties/Indices.hh"

substitute::substitute(Kernel& k, exptree& tr, exptree& args_)
	: Algorithm(k, tr), args(args_), comparator(k.properties), sort_product_(k, tr)
	{
//	if(number_of_args()==0) {
//		txtout << "substitute: need (list of) replacement rules." << std::endl;
//		throw constructor_error();
//		}

	sibling_iterator subslist=args.begin().begin();
	for(unsigned int i=0; i<args.arg_size(subslist); ++i) {
		iterator arrow=tr.arg(subslist, i);
		iterator lhs, rhs=tr.end();
		if(*arrow->name!="\\arrow" && *arrow->name!="\\equals") {
			lhs=arrow;
			throw ArgumentException("Argument is neither a replacement rule nor an equality");
			}
		else {
			lhs=args.begin(arrow);
			rhs=lhs;
			rhs.skip_children();
			++rhs;
			}
		try {
			if(*lhs->multiplier!=1) {
				throw ArgumentException("No numerical pre-factors allowed on lhs of replacement rule.");
				}
			// test validity of lhs and rhs
			iterator lhsit=lhs, stopit=lhs;
			stopit.skip_children();
			++stopit;
			while(lhsit!=stopit) {
				if(lhsit->is_object_wildcard()) {
					if(tr.number_of_children(lhsit)>0) {
						throw ArgumentException("Object wildcards cannot have child nodes.");
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
						throw ArgumentException("Object wildcards cannot have child nodes.");
						}
					}
				++lhsit;
				}

			// check whether there are dummies.
			index_map_t ind_free, ind_dummy;
			classify_indices(lhs, ind_free, ind_dummy);
			if(ind_dummy.size()>0)
				lhs_contains_dummies.push_back(true);
			else
				lhs_contains_dummies.push_back(false);
			ind_free.clear(); ind_dummy.clear();
			if(rhs!=tr.end()) {
				classify_indices(rhs, ind_free, ind_dummy);
				if(ind_dummy.size()>0)
					rhs_contains_dummies.push_back(true);
				else
					rhs_contains_dummies.push_back(false);
				}
			}
		catch(std::exception& er) {
			throw ArgumentException("Index error in replacement rule.");
// " << i+1 << "." << std::endl;
//			txtout << er.what() << std::endl;
			}
		}
	}

bool substitute::can_apply(iterator st)
	{
	sibling_iterator subslist=args.begin().begin();
	for(unsigned int i=0; i<tr.arg_size(subslist); ++i) {
		use_rule=i;

		comparator.clear();

		iterator arrow=tr.arg(subslist, i);
		iterator lhs=tr.begin(arrow);
		if(*lhs->name=="\\conditional") {
			lhs=tr.begin(lhs); 
			conditions=lhs;
			conditions.skip_children();
			++conditions;
			}
		else conditions=tr.end();
		
//		std::cerr << *lhs->name << " - " << *st->name << std::endl;

		if(lhs->name!=st->name && !lhs->is_object_wildcard() && !lhs->is_name_wildcard() && lhs->name->size()>0) 
			continue;

		exptree_comparator::match_t ret;
		comparator.lhs_contains_dummies=lhs_contains_dummies[i];
		if(*lhs->name=="\\prod") ret=comparator.match_subproduct(lhs, tr.begin(lhs), st);
		else                     ret=comparator.equal_subtree(lhs, st);

		if(ret == exptree_comparator::subtree_match) {
			if(conditions==tr.end()) return true;
			std::string error;
			if(comparator.satisfies_conditions(conditions, error))
				return true;
			else throw ArgumentException(error);
			}
		}
 	return false;
	}

Algorithm::result_t substitute::apply(iterator& st)
	{
//	prod_wrap_single_term(st);

   sibling_iterator arrow=tr.arg(args.begin().begin(), use_rule);
   iterator lhs=tr.begin(arrow);
   iterator rhs=lhs;
   rhs.skip_children();
   ++rhs;
   if(*lhs->name=="\\conditional")
      lhs=tr.begin(lhs);

	// We construct a new tree 'repl' which is a copy of the rhs of the
	// replacement rule, and then replace nodes and subtrees in there
	// based on how the pattern matching went.
   exptree repl(rhs);

	repl.wrap(repl.begin(), str_node("\\expression"));
	// First activate the inert '@(...)' commands present on the rhs.
	// FIXME: this is a hack, it should be much easier to activate inert commands
	// inside algorithm modules.
	bool replacer_found=false;
	iterator rit=repl.begin();
	while(rit!=repl.end()) {
		if(*rit->name=="@@") {
			replacer_found=true;
			// V2: fixme
//			eqn replacer(tr, tr.end());
//			iterator num=repl.begin(rit);
//			replacer.apply(num);
			iterator newrit=rit;
			newrit.skip_children();
			++newrit;
			repl.flatten(rit);
			repl.erase(rit);
			rit=newrit;
			}
		else ++rit;
		}
   index_map_t ind_free, ind_dummy, ind_forced;

	if(rhs_contains_dummies[use_rule])
		classify_indices(repl.begin(), ind_free, ind_dummy);
	
	// Replace all patterns on the rhs of the rule with the objects they matched.  
	// Keep track of all indices which _have_ to stay what they are, in ind_forced.
	// Keep track of insertion points of subtrees.
	iterator it=repl.begin();
	exptree_comparator::replacement_map_t::iterator loc;
	exptree_comparator::subtree_replacement_map_t::iterator sloc;
	std::vector<iterator> subtree_insertion_points;
	while(it!=repl.end()) { 
		bool is_stripped=false;
//		tr.print_recursive_treeform(std::cerr, repl.begin());

//		For some reason 'a?' is not found!?! Well, that's presumably because _{a?} does not
//      match ^{a?}. (though this does match when we write 'i' instead of a?. 

		loc=comparator.replacement_map.find(exptree(it));
		if(loc==comparator.replacement_map.end() && it->is_name_wildcard() && tr.number_of_children(it)!=0) {
			 exptree tmp(it);
			 tmp.erase_children(tmp.begin());
			 loc=comparator.replacement_map.find(tmp);
			 is_stripped=true;
			 }

		if(loc!=comparator.replacement_map.end()) { // name wildcards
//			if((*loc).first.begin()->fl.parent_rel==str_node::p_sub)
//				std::cerr << "_";
//			std::cerr << "rule : " << *((*loc).first.begin()->name) << " -> " 
//						 << *((*loc).second.begin()->name) << std::endl;
//			std::cerr << it->fl.parent_rel << " ";
//			std::cerr << "going to replace " << *it->name << " with " << *((*loc).second.begin()->name) << std::endl;

			// When a replacement is made here, and the index is actually
			// a dummy in the replacement, we screw up the ind_dummy
			// map. Then, at the next step, when conflicting dummies are
			// relabelled, things go wrong.  Solution: in this case, the
			// index under consideration should be taken out of ind_dummy.
			// This is easy, because we can just throw out all indices
			// with the original name.

			ind_dummy.erase(exptree(it));

			str_node::bracket_t remember_br=it->fl.bracket;
			if(is_stripped || (it->is_name_wildcard() && !it->is_index()) ) { 
            // a?_{i j k} type patterns should only replace the head
				// TODO: should we replace brackets here too?
				it->name=(*loc).second.begin()->name;
				it->multiplier=(*loc).second.begin()->multiplier;
				it->fl=(*loc).second.begin()->fl;
				}
			else {
				// Careful with the multiplier: the object has been matched to the pattern
				// without taking into account the top-level multiplier. So keep the multiplier
				// of the thing we are replacing.
				multiplier_t mt=*it->multiplier;
				it=tr.replace_index(it, (*loc).second.begin());
				multiply(it->multiplier, mt);
				}
			it->fl.bracket=remember_br;
			if(rhs_contains_dummies[use_rule])
				ind_forced.insert(index_map_t::value_type(exptree(it), it));
			++it;

			}
		else if( (sloc=comparator.subtree_replacement_map.find(it->name)) 
					!=comparator.subtree_replacement_map.end()) { // object wildcards
//			txtout << "srule : " << *it->name << std::endl;
			multiplier_t tmpmult=*it->multiplier; // remember target multiplier
			iterator tmp= tr.insert_subtree(it, (*sloc).second);
			tmp->fl.bracket=it->fl.bracket;
			tmp->fl.parent_rel=it->fl.parent_rel; // ok?
			it=tr.erase(it);
			multiply(tmp->multiplier, tmpmult);
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
//	tr.print_recursive_treeform(std::cerr, repl.begin());

	// If the replacement contains dummies, avoid clashes introduced when
	// free indices in the replacement (induced from the original expression)
   // take values already used for the dummies.
	// 
	// Note: the dummies which clash with other factors in a product are
	// not replaced here, but rather in the next step.
	if(ind_dummy.size()>0) {
		index_map_t must_be_empty;
		determine_intersection(ind_forced, ind_dummy, must_be_empty);
		index_map_t::iterator indit=must_be_empty.begin();
		index_map_t added_dummies;
//		txtout << must_be_empty.size() << " dummies have to be relabelled" << std::endl;
		while(indit!=must_be_empty.end()) {
			exptree the_key=indit->first;
			const Indices *dums=kernel.properties.get<Indices>(indit->second, true);
			if(dums==0)
				throw ConsistencyException("Need to know an index set for "); // V2: fix + *indit->second->name +".");
			exptree relabel=get_dummy(dums, &ind_dummy, &ind_forced, &added_dummies);
			added_dummies.insert(index_map_t::value_type(relabel,(*indit).second));
			do {
//				txtout << "replace index " << *(indit->second->name) << " with " << *(relabel.begin()->name) << std::endl;
				tr.replace_index(indit->second,relabel.begin());
				++indit;
//				txtout << *(indit->first.begin()->name) << " vs " << *(the_key.begin()->name) << std::endl;
				} while(indit!=must_be_empty.end() && tree_exact_equal(&kernel.properties, indit->first,the_key,-1));
			}
		}

	// Remove the wrapping "\expression" node, not needed anymore.
	repl.flatten(repl.begin());
	repl.erase(repl.begin());

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
		if(*rhs->name=="\\prod") {
			tr.flatten(newtr);
			tr.erase(newtr);
			}
		if(tr.number_of_children(st)==1) {
			multiply(tr.begin(st)->multiplier, *st->multiplier);
			tr.flatten(st);
			st=tr.erase(st);
			}
		}
	else {
		multiply(repl.begin()->multiplier, *st->multiplier);
		st=tr.move_ontop(st, repl.begin()); // no need to keep the original repl tree
		}

	if(ind_dummy.size()>0 && !rename_replacement_dummies_called) 
		rename_replacement_dummies(st);

	expression_modified=true;

	// The replacement is done now.  What is left is to take into
	// account any signs caused by moving factors through each other.
	int totsign=1;
	for(unsigned int i=0; i<comparator.factor_moving_signs.size(); ++i)
		totsign*=comparator.factor_moving_signs[i];
	multiply(st->multiplier, totsign);

	// Get rid of numerical '1' factors inside products (this will not clean up
	// '1's from a 'q -> 1' type replacement, since in this case 'st' points to the 'q'
   // node and we are not allowed to touch the tree above the entry point; these
	// things are taken care of by the algorithm class itself).
	if(*st->name=="\\prod") {
//		 debugout << "calling prodcollectnum" << std::endl;
//		 exptree::print_recursive_treeform(debugout, st);
		prodcollectnum pc(kernel, tr);
		pc.apply(st);
//		 exptree::print_recursive_treeform(debugout, st);
		}

//	tr.print_recursive_treeform(txtout, tr.begin());
//	txtout << "-----" << std::endl;

	// Cleanup nests on all insertion points and on the top node.
	for(unsigned int i=0; i<subtree_insertion_points.size(); ++i) {
		iterator ip=subtree_insertion_points[i];
		if(*ip->name=="\\sum") { // FIXME: is also in algorithm.cc, and should be factored out
			if(*ip->multiplier!=1) {
				sibling_iterator sib=tr.begin(ip);
				while(sib!=tr.end(ip)) {
					multiply(sib->multiplier, *ip->multiplier);
					++sib;
					}
				::one(ip->multiplier);
				}
			}
		cleanup_nests(tr, ip);
		}

//	tr.print_recursive_treeform(txtout, st);
	
//	prod_unwrap_single_term(st);

	cleanup_nests(tr, st);

//	tr.print_recursive_treeform(txtout, tr.begin());
//	prodcollectnum pc(tr, tr.end());
//	pc.apply(st);
//	if(replacer_found) {
//		txtout << "replacement took " << tmr << std::endl;
//		start_reporting_outside=true;
//		}
//	debugout << "leaving with st=" << *st->name << std::endl;
	tr.print_recursive_treeform(std::cout, tr.begin());
//	txtout << "======" << std::endl;

	return l_applied;
	}

