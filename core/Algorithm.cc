/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2015  Kasper Peeters <kasper.peeters@phi-sci.com>

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#include <stddef.h>
#include "Algorithm.hh"
#include "Storage.hh"
#include "Props.hh"
#include "Cleanup.hh"
#include <typeinfo>
#include <boost/version.hpp>
#if BOOST_VERSION > 105500
  #include <boost/core/demangle.hpp>
#endif

#include "properties/Accent.hh"
#include "properties/Derivative.hh"
#include "properties/Indices.hh"
#include "properties/Coordinate.hh"
#include "properties/Symbol.hh"
#include "properties/DependsBase.hh"

#include <sstream>

// #define DEBUG

using namespace cadabra;

Algorithm::Algorithm(const Kernel& k, Ex& tr_)
   : IndexClassifier(k),
     interrupted(false),
	  number_of_calls(0), number_of_modifications(0),
	  suppress_normal_output(false),
	  discard_command_node(false),
	  tr(tr_),
    pm(0),
    traverse_ldots(false)
	{
	}

Algorithm::~Algorithm()
	{
	}

void Algorithm::set_progress_monitor(ProgressMonitor *pm_)
	{
	pm=pm_;
	}

Algorithm::result_t Algorithm::apply_pre_order(bool repeat)
	{
#if BOOST_VERSION > 105500
	if(pm) 
		pm->group(boost::core::demangle(typeid(*this).name()).c_str());
#else
	if(pm) 
		pm->group(typeid(*this).name());
#endif
	
	result_t ret=result_t::l_no_action;
	Ex::iterator start=tr.begin();
	while(start!=tr.end()) {
		if(traverse_ldots || !tr.is_hidden(start)) {
			if(start->is_index()==false && apply_once(start)==result_t::l_applied) {
				ret=result_t::l_applied;
				// Need to cleanup on the entire tree above us.
				
				start.skip_children();
				}
			}
		++start;
		}

	cleanup_dispatch_deep(kernel, tr);
	
	if(pm) pm->group();
	return ret;
	}

Algorithm::result_t Algorithm::apply_generic(bool deep, bool repeat, unsigned int depth)
	{
	auto it = tr.begin();
	return apply_generic(it, deep, repeat, depth);
	}

Algorithm::result_t Algorithm::apply_generic(Ex::iterator& it, bool deep, bool repeat, unsigned int depth)
	{
#if BOOST_VERSION > 105500
	if(pm) 
		pm->group(boost::core::demangle(typeid(*this).name()).c_str());
#else
	if(pm) 
		pm->group(typeid(*this).name());
#endif

	result_t ret=result_t::l_no_action;

	Ex::fixed_depth_iterator start=tr.begin_fixed(it, depth, false);
	// std::cerr << "apply_generic at " << it.node << " " << *it->name << " " << *start->name << std::endl;

	while(tr.is_valid(start)) {
//		std::cerr << "evaluate main loop at " << *start->name << std::endl;
//		std::cerr << "main loop for " << typeid(*this).name() << ":\n" << Ex(start) << std::endl;

		result_t thisret=result_t::l_no_action;
		Ex::iterator enter(start);
		Ex::fixed_depth_iterator next(start);
		++next;
//		if(tr.is_valid(next))
//			std::cerr << "next = " << *next->name << std::endl;
		do {
//			std::cout << "apply at " << *enter->name << std::endl;
			bool work_is_topnode=(enter==it);
			if(deep && depth==0) 
				thisret = apply_deep(enter);
			else
				thisret = apply_once(enter);

			if(work_is_topnode)
				it=enter;
			
			// FIXME: handle l_error or remove
			if(thisret==result_t::l_applied)
				ret=result_t::l_applied;
			} while(depth==0 && repeat && thisret==result_t::l_applied);

		if(depth==0) {
			// std::cerr << "break " << std::endl;
			break;
			}
		else {
			// std::cerr << "no break " << std::endl;
			}
		start=next;
		} 

	// std::cerr << "pre-exit node " << it.node << std::endl;

	// If we are acting at fixed depth, we will not have gone up in the
	// tree, so missed one cleanup action. Do it now.
	if(depth>0) {
		Ex::fixed_depth_iterator start=tr.begin_fixed(it, depth-1, false);
		while(tr.is_valid(start)) {
			Ex::iterator work=start;
			++start;
			bool cpy=false;
			if(work==it) cpy=true;
			cleanup_dispatch(kernel, tr, work);
			if(cpy) it=work;
			}
		}

	// std::cerr << "exit node " << it.node << std::endl;
	
//	if(tr.is_valid(it)) {
//		std::cerr << "exit " << *it->name << std::endl;
//		std::cerr << "exit apply_generic\n" << Ex(it) << std::endl;
//		}

	if(pm) pm->group();
	return ret;
	}

Algorithm::result_t Algorithm::apply_once(Ex::iterator& it)
	{
	// std::cerr << "=== apply_once ===" << std::endl;
	if(traverse_ldots || !tr.is_hidden(it)) {
		if(can_apply(it)) {
			result_t res=apply(it);
			// std::cerr << "apply algorithm at " << *it->name << std::endl;
			if(res==result_t::l_applied) {
				cleanup_dispatch(kernel, tr, it);
				return res;
				}
			}
		}
	
	return result_t::l_no_action;
	}

Algorithm::result_t Algorithm::apply_deep(Ex::iterator& it) 
	{
	// This recursive algorithm walks the tree depth-first
	// (parent-after-child). The algorithm is applied on each node if
	// can_apply returns true. When the iterator goes up one level
	// (i.e. from a child to a parent), and any changes have been made
	// so far at the child level level, cleanup and simplification
	// routines will be called. The only nodes that can be removed from
	// the tree are nodes at a lower level than the simplification
	// node.

	// std::cout << "=== apply_deep ===" << std::endl;
//	tr.print_recursive_treeform(std::cout, it);

	post_order_iterator current=it;
	current.descend_all();
	post_order_iterator last=it;
	int deepest_action = -1;
//	std::cout << "apply_deep: it = " << *it->name << std::endl;
	bool stop_after_this_one=false;
	result_t some_changes_somewhere=result_t::l_no_action;

	for(;;) {
//		std::cout << "reached " << *current->name << std::endl;
#ifdef DEBUG
		std::cout << "apply_deep " << typeid(*this).name() << ": current = " << *current->name << std::endl;
#endif

		if(current.node==last.node) {
//			std::cout << "stop after this one" << std::endl;
			stop_after_this_one=true;
			}

		if(deepest_action > tr.depth(current)) {
#ifdef DEBUG
			std::cerr << "simplify; we are at " << *(current->name) << std::endl;
#endif
			iterator work=current;
			bool work_is_topnode=(work==it);
			cleanup_dispatch(kernel, tr, work);
			current=work;
			if(work_is_topnode)
				it=work;
#ifdef DEBUG
			std::cerr << "current now " << *(current->name) << std::endl;
			tr.print_recursive_treeform(std::cerr, current); 
#endif
			deepest_action = tr.depth(current); // needs to propagate upwards
			}
		
		if((traverse_ldots || !tr.is_hidden(current)) && can_apply(current)) {
#ifdef DEBUG
			std::cout << "acting at " << *current->name << std::endl;
#endif
			iterator work=current;
			post_order_iterator next(current);
			++next;
			bool work_is_topnode=(work==it);
			result_t res = apply(work);
			if(res==Algorithm::result_t::l_applied) {
				some_changes_somewhere=result_t::l_applied;
				rename_replacement_dummies(work, true);
				deepest_action=tr.depth(work);
				// If we got a zero at 'work', we need to propagate this up the tree and
				// then restart our post-order traversal such that everything that has
				// been removed from the tree by this zero will no longer be considered.
				if(*work->multiplier==0) {
#ifdef DEBUG
					std::cerr << "propagate zero up the tree" << std::endl;
					tr.print_recursive_treeform(std::cerr, it);
#endif
					post_order_iterator moved_next=work;
					propagate_zeroes(moved_next, it);
#ifdef DEBUG
					tr.print_recursive_treeform(std::cerr, it);
					std::cerr << Ex(it) << std::endl;
#endif
					next=moved_next;
					}

				// The 'work' iterator can now point to a new node. If we were acting at the
				// top node, we need to propagate the change in 'work' to 'it' so the caller
				// knows where the new top node is.
				if(work_is_topnode)
					it=work;
				}
         // The algorithm may have replaced the 'work' node, so instead of walking from
			// there, we continue at the node which was next in line before we called 'apply'.
			current=next; 
			} 
		else {
			++current;
			}

		if(stop_after_this_one)
			break;

		}

#ifdef DEBUG
	std::cerr << "recursive end **" << std::endl;
#endif

	return some_changes_somewhere;
	}

void Algorithm::propagate_zeroes(post_order_iterator& it, const iterator& topnode)
	{
	assert(*it->multiplier==0);
	if(it==topnode) return;
	iterator walk=tr.parent(it);
#ifdef DEBUG
	std::cerr << "propagate_zeroes at " << *walk->name << std::endl;
#endif
	if(!tr.is_valid(walk)) 
		return;

	const Derivative *der=kernel.properties.get<Derivative>(walk);
	if(*walk->name=="\\prod" || der) {
		if(der && it->is_index()) return;
		walk->multiplier=rat_set.insert(0).first;
		it=walk;
		propagate_zeroes(it, topnode);
		// Removing happens in the next step.
		}
	else if(*walk->name=="\\pow") {
		if(tr.index(it)==0) { // the argument
			walk->multiplier=rat_set.insert(0).first;
			it=walk;
			propagate_zeroes(it, topnode);
			}
		else { // the exponent
			rset_t::iterator rem=walk->multiplier;
			tr.erase(it);
			tr.flatten(walk);
			it=tr.erase(walk);
			node_one(it);
			it->multiplier=rem;
			}
		}
	else if(*walk->name=="\\sum") {
		if(tr.number_of_children(walk)>2) {
			if(tr.is_valid(tr.next_sibling(it))) {
				it=tr.erase(it);
				it.descend_all();
				}
			else {
				iterator ret=tr.parent(it); 
				tr.erase(it);
				it=ret;
				}
			}
		else {
			// If the sum is the top node, we cannot flatten it because
			// we are not allowed to invalidate the topnode iterator
			if(walk==topnode) {
#ifdef DEBUG
				std::cerr << "\\sum at top, cannot flatten" << std::endl;
#endif
//				it=tr.next_sibling(it); // Added but wrong?
				return;
				}

			tr.erase(it);
			iterator singlearg=tr.begin(walk);
			if(singlearg!=tr.end(walk)) {
				singlearg->fl.bracket=walk->fl.bracket; // to remove brackets of the sum
				if(*tr.parent(walk)->name=="\\prod") {
					multiply(tr.parent(walk)->multiplier, *singlearg->multiplier);
					cadabra::one(singlearg->multiplier);
					}
				}
			tr.flatten(walk);
			it=tr.erase(walk);
			if(*it->name=="\\prod" && *tr.parent(it)->name=="\\prod") {
				tr.flatten(it);
				it=tr.erase(it);
				}
			}
		}
	else { 
	   iterator nn=tr.insert_after(it, str_node("1"));
		nn->fl.parent_rel=it->fl.parent_rel;
		nn->fl.bracket=it->fl.bracket;
		it=tr.erase(it);
		zero(it->multiplier);
		}

	return;
	}

void Algorithm::pushup_multiplier(iterator it) 
	{
	if(!tr.is_valid(it)) return;
	if(*it->multiplier!=1) {
		if(*it->name=="\\sum") {
//			txtout << "SUM" << std::endl;
			sibling_iterator sib=tr.begin(it);
			while(sib!=tr.end(it)) {
				multiply(sib->multiplier, *it->multiplier);
//				txtout << "going up" << std::endl;
				pushup_multiplier(tr.parent(it));
//				txtout << "back and back up" << std::endl;
				pushup_multiplier(sib);
//				txtout << "back" << std::endl;
				++sib;
				}
			::one(it->multiplier);
			}
		else {
//			txtout << "PUSHUP: " << *it->name << std::endl;
			if(tr.is_valid(tr.parent(it))) {
//				txtout << "test propinherit" << std::endl;
//				iterator tmp=tr.parent(it);
				// tmp not always valid?!? This one crashes hard with a loop!?!
//				txtout << " of " << *tmp->name << std::endl;
				const PropertyInherit *pin=kernel.properties.get<PropertyInherit>(tr.parent(it));
				if(pin || *(tr.parent(it)->name)=="\\prod") {
					multiply(tr.parent(it)->multiplier, *it->multiplier);
					::one(it->multiplier); // moved up, was at end of block, correct?
//					txtout << "going up" << std::endl;
					pushup_multiplier(tr.parent(it));
//					txtout << "back" << std::endl;
					}
//				else txtout << "not relevant" << std::endl;
				}
			}
		}
	}

void Algorithm::node_zero(iterator it)
	{
	::zero(it->multiplier);
	tr.erase_children(it);
	it->name=name_set.insert("1").first;
	}

void Algorithm::node_one(iterator it)
	{
	::one(it->multiplier);
	tr.erase_children(it);
	it->name=name_set.insert("1").first;
	}

void Algorithm::node_integer(iterator it, int num)
	{
	::one(it->multiplier);
	tr.erase_children(it);
	it->name=name_set.insert("1").first;
	::multiply(it->multiplier, num);
	}

int Algorithm::index_parity(iterator it) const
	{
	sibling_iterator frst=tr.begin(tr.parent(it));
	sibling_iterator fnd(it);
	int sgn=1;
	while(frst!=fnd) {
		sgn=-sgn;
		++frst;
		}
	return sgn;
	}


unsigned int Algorithm::number_of_indices(iterator it) 
	{
	unsigned int res=0;
	index_iterator indit=begin_index(it);
	while(indit!=end_index(it)) {
		++res;
		++indit;
		}
	return res;
	}


index_iterator Algorithm::begin_index(iterator it) const
	{
	return index_iterator::begin(kernel.properties, it);
	}

index_iterator Algorithm::end_index(iterator it) const
	{
	return index_iterator::end(kernel.properties, it);
	}




bool Algorithm::check_index_consistency(iterator it) const 
	{
	index_map_t ind_free, ind_dummy;
	classify_indices(it,ind_free,ind_dummy);
	return true;
	}

bool Algorithm::check_degree_consistency(iterator it) const
	{
	return true; // FIXME: this needs to be implemented.
	}

bool Algorithm::check_consistency(iterator it) const
	{
	Stopwatch w1;
	w1.start();
//	debugout << "checking consistency ... " << std::flush;
	assert(tr.is_valid(tr.parent(it))==false);
//	iterator entry=it;
	iterator end=it;
	end.skip_children();
	++end;
	while(it!=end) {
		if(interrupted)
			throw InterruptionException("check_consistency");

		if(*it->name=="\\sum") {
			if(*it->multiplier!=1)
				throw ConsistencyException("Found \\sum node with non-unit multiplier.");
			else if(Ex::number_of_children(it)<2)
				throw ConsistencyException("Found a \\sum node with 0 or 1 child nodes.");
			else { 
				sibling_iterator sumch=it.begin();
				str_node::bracket_t firstbracket=sumch->fl.bracket;
				while(*sumch->name=="\\sum" || *sumch->name=="\\prod") {
					++sumch;
					if(sumch==it.end()) break;
					else                  firstbracket=sumch->fl.bracket;
					}
				sumch=it.begin();
				while(sumch!=it.end()) {
					if(*sumch->name!="\\sum" && *sumch->name!="\\prod") {
						if(sumch->fl.bracket!=firstbracket)
							throw ConsistencyException("Found a \\sum node with different brackets on its children.");
						}
//					else if(*sumch->name=="\\sum") {
//						sibling_iterator sumchch=sumch.begin();
//						while(sumchch!=sumch.end()) { 
//							if(sumchch->fl.bracket==str_node::b_none) {
//								tr.print_recursive_treeform(debugout, entry);
//								throw ConsistencyException("Found a sum node with \\sum child without bracketed children.");
//								}
//							++sumchch;
//							}
//						}
					++sumch;
					}
				}
			}
		else if(*it->name=="\\prod") {
			 if(Ex::number_of_children(it)<=1) 
				  throw ConsistencyException("Found \\prod node with only 0 or 1 children.");
			sibling_iterator ch=it.begin();
			str_node::bracket_t firstbracket=ch->fl.bracket;
			while(*ch->name=="\\sum" || *ch->name=="\\prod") {
				++ch;
				if(ch==it.end())   break;
				else               firstbracket=ch->fl.bracket;
				}
			ch=it.begin();
			while(ch!=it.end()) {
				if(*ch->name!="\\prod" && *ch->name!="\\sum") {
					if(ch->fl.bracket!=firstbracket)
						throw ConsistencyException("Found \\prod node with different brackets on its children.");
					}
				if(*ch->multiplier!=1) {
					throw ConsistencyException("Found \\prod child with non-unit multiplier.");
//					debugout << "node name " << *ch->name << ", multiplier " << *ch->multiplier << std::endl;
//					inconsistent=true;
//					break;
					}
				++ch;
				}
			}
		else if(*it->name=="\\sequence") {
			if(Ex::number_of_children(it)!=2)
				throw ConsistencyException("Found \\sequence node with incorrect (non-2) number of children.");
			}
		++it;
		}

	w1.stop();
//	debugout << "checking done..." << w1 << std::endl;
	return true;
	}

void Algorithm::report_progress(const std::string& str, int todo, int done, int count) 
	{
	bool display=false;
	if(count==2) display=true;
	else {
		if(report_progress_stopwatch.stopped()) {
			display=true;
			report_progress_stopwatch.start();
			}
		else {
			if(report_progress_stopwatch.seconds()>0 || report_progress_stopwatch.useconds()>300000L) {
				display=true;
				report_progress_stopwatch.reset();
				}
			}
		}

//	if(display) { // prevents updates at a rate of more than one per second
//		if(eo->output_format==Ex_output::out_xcadabra) {
//			txtout << "<progress>" << std::endl
//					 << str << std::endl
//					 << todo << std::endl
//					 << done << std::endl
//					 << count << std::endl
//					 << "</progress>" << std::endl;
//			}
//		else {
//			if(count==2)
//				txtout << str << " (" << done << " of " << todo << " completed)" << std::endl;
//			}
//		}
	}

bool Algorithm::rename_replacement_dummies(iterator two, bool still_inside_algo) 
	{
#ifdef DEBUG
	std::cerr << "renaming in " << two << std::endl;
#endif
//	std::cout << "full story " << *two->name << std::endl;
//	print_classify_indices(two);
//	std::cout << "replacement" << std::endl;
//	print_classify_indices(std::cout, two);

	index_map_t ind_free, ind_dummy;
	index_map_t ind_free_full, ind_dummy_full;

	if(still_inside_algo) {
		if(tr.is_head(two)==false)
			classify_indices_up(tr.parent(two), ind_free_full, ind_dummy_full);
		}
	else {
		classify_indices_up(two, ind_free_full, ind_dummy_full); // the indices in everything except the replacement
		}
	classify_indices(two, ind_free, ind_dummy); // the indices in the replacement subtree
#ifdef DEBUG
	std::cerr << "dummies of " << *two->name << std::endl;
	for(auto& ii: ind_dummy) 
	 	std::cerr << ii.first << std::endl;
	std::cerr << "free indices above us" << std::endl;
	for(auto& ii: ind_free_full) 
	 	std::cerr << ii.first << std::endl;
#endif

	index_map_t must_be_empty;
	index_map_t newly_generated;

	// Catch double index pairs
	determine_intersection(ind_dummy_full, ind_dummy, must_be_empty);
	index_map_t::iterator it=must_be_empty.begin();
	while(it!=must_be_empty.end()) {
		// std::cerr << "double index pair" << std::endl;
		Ex the_key=(*it).first;
		const Indices *dums=kernel.properties.get<Indices>(it->second, true);
		if(!dums)
			throw ConsistencyException("Failed to find dummy property for $"+*it->second->name+"$ while renaming dummies.");
//			txtout << "failed to find dummy property for " << *it->second->name << std::endl;
		assert(dums);
		Ex relabel
            =get_dummy(dums, &ind_dummy_full, &ind_dummy, &ind_free_full, &ind_free, &newly_generated);
		newly_generated.insert(index_map_t::value_type(Ex(relabel),(*it).second));
//		txtout << " renamed to " << *relabel << std::endl;
		do {
			tr.replace_index((*it).second, relabel.begin(), true);
//			(*it).second->name=relabel;
			++it;
			} while(it!=must_be_empty.end() && tree_exact_equal(&kernel.properties, (*it).first,the_key, 1, true, -2, true));
		}
								 
	// Catch triple indices (two cases: dummy pair in replacement, free index elsewhere and 
	// dummy elsewhere, free index in replacement)
	must_be_empty.clear();
//	newly_generated.clear(); // DO NOT ERASE, IDIOT!

	determine_intersection(ind_free_full, ind_dummy, must_be_empty);
	//for(auto& ii: must_be_empty) {
	//	std::cerr << ii.first << std::endl;
	//	}
	it=must_be_empty.begin();
	while(it!=must_be_empty.end()) {
		//std::cerr << "triple index pair " << it->first << std::endl;
		Ex the_key=(*it).first;
		const Indices *dums=kernel.properties.get<Indices>(it->second, true);
		if(!dums)
			 throw ConsistencyException("Failed to find dummy property for $"+*it->second->name+"$ while renaming dummies.");
		assert(dums);
		Ex relabel
            =get_dummy(dums, &ind_dummy_full, &ind_dummy, &ind_free_full, &ind_free, &newly_generated);
		relabel.begin()->fl.parent_rel=it->second->fl.parent_rel;
		newly_generated.insert(index_map_t::value_type(relabel,(*it).second));
		do {
			tr.replace_index((*it).second, relabel.begin(), true);
			++it;
			} while(it!=must_be_empty.end() && tree_exact_equal(&kernel.properties, (*it).first,the_key, 1, true, -2, true));
		}

	must_be_empty.clear();
   //	newly_generated.clear();
	determine_intersection(ind_free, ind_dummy_full, must_be_empty);
	it=must_be_empty.begin();
	while(it!=must_be_empty.end()) {
		// std::cerr << "triple index pair 2" << std::endl;
		Ex the_key=(*it).first;
		const Indices *dums=kernel.properties.get<Indices>(it->second, true);
		if(!dums)
			 throw ConsistencyException("Failed to find dummy property for $"+*it->second->name+"$ while renaming dummies.");
		assert(dums);
		Ex relabel
            =get_dummy(dums, &ind_dummy_full, &ind_dummy, &ind_free_full, &ind_free, &newly_generated);
		relabel.begin()->fl.parent_rel=it->second->fl.parent_rel;
		newly_generated.insert(index_map_t::value_type(relabel,(*it).second));
		do {
			tr.replace_index((*it).second, relabel.begin(), true);
			++it;
			} while(it!=must_be_empty.end() && tree_exact_equal(&kernel.properties, (*it).first,the_key, 1, true, -2, true));
		}

	return true;
	}



bool Algorithm::contains(sibling_iterator from, sibling_iterator to, sibling_iterator arg)
	{
	while(from!=to) {
		if(from->name==arg->name) return true;
		++from;
		}
	return false;
	}

Algorithm::range_vector_t::iterator Algorithm::find_arg_superset(range_vector_t& ran, 
																		 sibling_iterator it)
	{
	sibling_iterator nxt=it;
	++nxt;
	return find_arg_superset(ran, it, nxt);
	}

//void Algorithm::find_argument_lists(range_vector_t& ran, bool only_comma_lists) const
//	{
//	sibling_iterator argit=args_begin();
//	while(argit!=args_end()) {
//		if(*argit->name=="\\comma") {
//			ran.push_back(range_t(tr.begin(argit), tr.end(argit)));
//			}
//		else if(!only_comma_lists) {
//			sibling_iterator argnxt=argit; ++argnxt;
//			ran.push_back(range_t(argit, argnxt));
//			}
//		++argit;
//		}	
//	}

template<class Iter>
Algorithm::range_vector_t::iterator Algorithm::find_arg_superset(range_vector_t& ran, Iter st, Iter nd)
	{
	range_vector_t::iterator ranit=ran.begin();
	while(ranit!=ran.end()) {
		sibling_iterator findthese=st;
		bool contained=true;
		while(findthese!=nd) {
			if(contains((*ranit).first, (*ranit).second, findthese)) {
				++findthese;
				}
			else { 
				contained=false;
				break;
				}
			}
		if(contained) return ranit;
		++ranit;
		}
	return ran.end();
	}

bool Algorithm::is_termlike(iterator it)
	{
	if(!is_factorlike(it))
		if(*it->name!="\\sum")
			if(it->fl.parent_rel==str_node::p_none)
				return true;

	return false;
	}

bool Algorithm::is_factorlike(iterator it)
	{
	if(tr.is_valid(tr.parent(it))) {
		if(*tr.parent(it)->name=="\\prod")
			return true;
		}
	return false;
	}

// This only returns true if the indicated node is a single non-reserved node (non-prod, non-sum, ...)
// at the top level of an expression (real top, top of equation lhs/rhs, top of integral argument, ...).

bool Algorithm::is_single_term(iterator it)
	{
	if(*it->name!="\\prod" && *it->name!="\\sum" && *it->name!="\\asymimplicit"
	   && *it->name!="\\comma" && *it->name!="\\equals" && *it->name!="\\arrow") {

		if(tr.is_head(it) || *tr.parent(it)->name=="\\equals" || *tr.parent(it)->name=="\\int") return true;
		else if(*tr.parent(it)->name=="\\sum")
			return true;
		else if(*tr.parent(it)->name!="\\prod" && it->fl.parent_rel==str_node::parent_rel_t::p_none
				  && kernel.properties.get<Accent>(tr.parent(it))==0 ) {
#ifdef DEBUG			
			std::cerr << "Found single term in " << tr.parent(it) << std::endl;
#endif
			return true;
			}
		}
	return false;
	}

bool Algorithm::is_nonprod_factor_in_prod(iterator it)
	{
	if(*it->name!="\\prod" && *it->name!="\\sum" && *it->name!="\\asymimplicit" && *it->name!="\\comma" 
		&& *it->name!="\\equals") {
		if(tr.is_valid(tr.parent(it))) {
			if(*tr.parent(it)->name=="\\prod")
				return true;
			}
//		else return true;
		}
	return false;
	}

bool Algorithm::prod_wrap_single_term(iterator& it)
	{
	if(is_single_term(it)) {
		force_node_wrap(it, "\\prod");
		return true;
		}
	else return false;
	}

bool Algorithm::sum_wrap_single_term(iterator& it)
	{
	if(is_single_term(it)) {
		force_node_wrap(it, "\\sum");
		return true;
		}
	else return false;
	}

void Algorithm::force_node_wrap(iterator& it, std::string nm)
	{
	iterator prodnode=tr.insert(it, str_node(nm));
	sibling_iterator fr=it, to=it;
	++to;
	tr.reparent(prodnode, fr, to);
	prodnode->fl.bracket=it->fl.bracket;
	it->fl.bracket=str_node::b_none;
	prodnode->multiplier=it->multiplier;
	one(it->multiplier);
	it=prodnode;
	}

bool Algorithm::prod_unwrap_single_term(iterator& it)
	{
	if((*it->name)=="\\prod") {
		if(tr.number_of_children(it)==1) {
			multiply(tr.begin(it)->multiplier, *it->multiplier);
			tr.begin(it)->fl.bracket=it->fl.bracket;
			tr.begin(it)->multiplier=it->multiplier;
			tr.flatten(it);
			it=tr.erase(it);
			return true;
			}
		}
	return false;
	}

bool Algorithm::sum_unwrap_single_term(iterator& it)
	{
	if((*it->name)=="\\sum") {
		if(tr.number_of_children(it)==1) {
			multiply(tr.begin(it)->multiplier, *it->multiplier);
			tr.begin(it)->fl.bracket=it->fl.bracket;
			tr.begin(it)->multiplier=it->multiplier;
			tr.flatten(it);
			it=tr.erase(it);
			return true;
			}
		}
	return false;
	}

bool Algorithm::separated_by_derivative(iterator i1, iterator i2, iterator check_dependence) const
	{
	iterator lca = tr.lowest_common_ancestor(i1, i2);

	// Walk up the tree from the first node until the LCA, flag any derivatives
	// with which we do not commute.

	struct {
	  bool operator()(const Kernel& kernel, Ex& tr, iterator walk, iterator lca, iterator check_dependence) {
	      const Properties& pr=kernel.properties;
		   do {
				walk=Ex::parent(walk);
				if(walk == lca) break;
				const Derivative *der=pr.get<Derivative>(walk);
				if(der) {
					if(tr.is_valid(check_dependence) ) {
						const DependsBase *dep = pr.get_composite<DependsBase>(check_dependence);
						if(dep) {
							Ex deps=dep->dependencies(kernel, check_dependence);
							sibling_iterator depobjs=deps.begin(deps.begin());
							while(depobjs!=deps.end(deps.begin())) {
								if(walk->name == depobjs->name) {
									return true;
									}
								else {
									// compare all indices
									sibling_iterator indit=tr.begin(walk);
									while(indit!=tr.end(walk)) {
										if(indit->is_index()) {
											if(subtree_exact_equal(&pr, indit, depobjs))
												return true;
											}
										++indit;
										}
									}
								++depobjs;
								} 
							return false; // Dependence found but not relevant here.
							}
						else return false; // No dependence property found at all.
						}
					else return true; // Should not check for dependence.
					}
				} while(walk != lca);
			return false;
		   }
	} one_run;
	
	if(one_run(kernel, tr, i1, lca, check_dependence)) return true;
	if(one_run(kernel, tr, i2, lca, check_dependence)) return true;

	return false;
	}


// bool Algorithm::cleanup_anomalous_products(Ex& tr, Ex::iterator& it)
// 	{
// 	if(*(it->name)=="\\prod") {
// 		 if(tr.number_of_children(it)==0) {
// 			  it->name=name_set.insert("1").first;
// 			  return true;
// 			  }
// 		 else if(tr.number_of_children(it)==1) {
// 			  tr.begin(it)->fl.bracket=it->fl.bracket;
// 			  tr.begin(it)->multiplier=it->multiplier;
// 			  tr.flatten(it);
// 			  Ex::iterator tmp=tr.erase(it);
// //			  txtout << "HERRE?" << std::endl;
// 			  pushup_multiplier(tmp);
// 			  it=tmp;
// 			  return true;
// 			  }
// 		 }
// 	return false;
// 	}
// 

unsigned int Algorithm::locate_single_object(Ex::iterator obj_to_find, 
															Ex::iterator st, Ex::iterator nd,
															std::vector<unsigned int>& store)
	{
	unsigned int count=0;
	unsigned int index=0;
	while(st!=nd) {
		Ex::iterator it1=st; it1.skip_children(); ++it1;
		if(tr.equal(st, it1, obj_to_find, Algorithm::compare_)) {
			++count;
			store.push_back(index);
			}
		++st;
		++index;
		}
	return count;
	}

bool Algorithm::locate_object_set(const Ex& objs, 
											 Ex::iterator st, Ex::iterator nd,
											 std::vector<unsigned int>& store)
	{
	// Locate the objects in which to symmetrise. We use an integer
	// index (offset wrt. 'st') rather than an iterator because the
	// latter only apply to a single tree, not to its copies.

	// We accept only a tree with a \comma node at the top.
	Ex::iterator top=objs.begin();
	if(*top->name!="\\comma") 
		top = objs.begin(objs.begin());

	assert(*top->name=="\\comma");

	Ex::sibling_iterator fst=objs.begin(top);
	Ex::sibling_iterator fnd=objs.end(top);
	while(fst!=fnd) {
		Ex::iterator aim=fst;
		if((*aim->name)=="\\comma") {
			// Objects can themselves be lists of other objects (for instance
         // when we want to symmetrise in index pairs).
			if(locate_object_set(aim, st, nd, store)==false)
				return false;
			}
		else {
			if((*aim->name).size()==0 && tr.number_of_children(aim)==1)
				aim=tr.begin(aim);
			if(locate_single_object(aim, st, nd, store)!=1)
				return false;
			}
		++fst;
		}
	return true;
	}


namespace cadabra {

// static functions
	
unsigned int Algorithm::number_of_indices(const Properties& pr, iterator it) 
	{
	unsigned int res=0;
	index_iterator indit=index_iterator::begin(pr, it);
	while(indit!=index_iterator::end(pr, it)) {
		++res;
		++indit;
		}
	return res;
	}

unsigned int Algorithm::number_of_direct_indices(iterator it) 
	{
	unsigned int res=0;
	sibling_iterator sib=Ex::begin(it);
	while(sib!=Ex::end(it)) {
		if(sib->fl.parent_rel==str_node::p_sub || sib->fl.parent_rel==str_node::p_super)
			++res;
		++sib;
		}
	return res;
	}

bool Algorithm::less_without_numbers(nset_t::iterator it1, nset_t::iterator it2) 
	{
	std::string::const_iterator ch1=(*it1).begin();
	std::string::const_iterator ch2=(*it2).begin();

	while(ch1!=(*it1).end() && ch2!=(*it2).end()) {
		if(isdigit(*ch1)) return true;   // bla1  < blaq
		if(isdigit(*ch2)) return false;  // blaa !< bla1 
		if(*ch1>=*ch2)    return false;
		++ch1;
		++ch2;
		}
	if(ch1==(*it1).end()) {
		if(ch2==(*it2).end())
			return false;
		else 
			return true;
		}
	return false;
	}

bool Algorithm::equal_without_numbers(nset_t::iterator it1, nset_t::iterator it2) 
	{
	std::string::const_iterator ch1=(*it1).begin();
	std::string::const_iterator ch2=(*it2).begin();

	while(ch1!=(*it1).end() && ch2!=(*it2).end()) {
		if(isdigit(*ch1)) {
			if(isdigit(*ch2)) 
				return true;
			else
				return false;
			}
		if(*ch1!=*ch2) return false;
		++ch1;
		++ch2;
		}
	if(ch1==(*it1).end()) {
		if(ch2==(*it2).end())
			return true;
		else 
			return false;
		}
	return false;	
	}

bool Algorithm::compare_(const str_node& one, const str_node& two)
	{
	// If the obj->name is empty, this means that we look for a tree with
	// anything as root, but the required index structure in obj.  This
	// requires a slightly different 'equal_to' (one that always matches
	// an empty node with a non-empty node).

	if(/* one.fl.bracket!=two.fl.bracket || */ one.fl.parent_rel!=two.fl.parent_rel)
		return false;

	if((*two.name).size()==0)
		return true;
	else if(one.name==two.name)
		return true;
	return false;
	}

}
