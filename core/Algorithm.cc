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

#include "properties/Derivative.hh"
#include "properties/Indices.hh"
#include "properties/Coordinate.hh"
#include "properties/Symbol.hh"
#include "properties/DependsBase.hh"

#include <typeinfo>
#include <sstream>


Algorithm::Algorithm(const Kernel& k, Ex& tr_)
  : interrupted(false),
	  number_of_calls(0), number_of_modifications(0),
	  suppress_normal_output(false),
	  discard_command_node(false),
	  kernel(k),
	  tr(tr_)
	{
	}

Algorithm::~Algorithm()
	{
	}

Algorithm::result_t Algorithm::apply_generic(Ex::iterator& it, bool deep, bool repeat, unsigned int depth)
	{
	result_t ret=result_t::l_no_action;

	Ex::fixed_depth_iterator start=tr.begin_fixed(it, depth);
	while(tr.is_valid(start)) {
		result_t thisret=result_t::l_no_action;
		Ex::iterator enter(start);
		Ex::fixed_depth_iterator next(start);
		++next;
		do {
//			std::cout << "apply at " << *enter->name << std::endl;
			if(deep && depth==0) 
				thisret = apply_deep(enter);
			else
				thisret = apply_once(enter);
			
			// FIXME: handle l_error or remove
			if(thisret==result_t::l_applied)
				ret=result_t::l_applied;
			} while(depth==0 && repeat && thisret==result_t::l_applied);

		if(depth==0)  
			break;
		start=next;
		} 

	// If we are acting at fixed depth, we will not have gone up in the
	// tree, so missed one cleanup action. Do it now.
	if(depth>0) {
		Ex::fixed_depth_iterator start=tr.begin_fixed(it, depth-1);
		while(tr.is_valid(start)) {
			Ex::iterator work=start;
			++start;
			cleanup_dispatch(kernel, tr, work);
			}
		}

	return ret;
	}

Algorithm::result_t Algorithm::apply_once(Ex::iterator& it)
	{
	if(can_apply(it)) {
		result_t res=apply(it);
		// std::cerr << "apply algorithm at " << *it->name << std::endl;
		if(res==result_t::l_applied) {
			cleanup_dispatch(kernel, tr, it);
			return res;
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

//	std::cout << "=== apply_deep ===" << std::endl;
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
//		std::cout << "apply_deep: current = " << *current->name << std::endl;

		if(current.node==last.node) {
//			std::cout << "stop after this one" << std::endl;
			stop_after_this_one=true;
			}

		if(deepest_action > tr.depth(current)) {
//			std::cout << "simplify; we are at " << *(current->name) << std::endl;
			iterator work=current;
			cleanup_dispatch(kernel, tr, work);
			current=work;
//			std::cout << "current now " << *(current->name) << std::endl;
			deepest_action = tr.depth(current); // needs to propagate upwards
			}
		
		if(can_apply(current)) {
			// std::cout << "acting at " << *current->name << std::endl;
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
					post_order_iterator moved_next=work;
					propagate_zeroes(moved_next, it);
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

//	std::cout << "recursive end **" << std::endl;

	return some_changes_somewhere;
	}

void Algorithm::propagate_zeroes(post_order_iterator& it, const iterator& topnode)
	{
	assert(*it->multiplier==0);
	if(it==topnode) return;
	iterator walk=tr.parent(it);
//	debugout << *walk->name << std::endl;
	if(!tr.is_valid(walk)) 
		return;

	const Derivative *der=kernel.properties.get<Derivative>(walk);
	if(*walk->name=="\\prod" || der) {
		if(der && it->is_index()) return;
		walk->multiplier=rat_set.insert(0).first;
		it=walk;
		propagate_zeroes(it, topnode);
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
			if(walk==topnode) return; 

			tr.erase(it);
			iterator singlearg=tr.begin(walk);
			if(singlearg!=tr.end(walk)) {
				singlearg->fl.bracket=walk->fl.bracket; // to remove brackets of the sum
				if(*tr.parent(walk)->name=="\\prod") {
					multiply(tr.parent(walk)->multiplier, *singlearg->multiplier);
					::one(singlearg->multiplier);
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

bool Algorithm::check_consistency(iterator it) const
	{
	stopwatch w1;
	w1.start();
//	debugout << "checking consistency ... " << std::flush;
	assert(*it->name=="\\expression");
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
//	std::cout << "full story " << *two->name << std::endl;
//	print_classify_indices(two);
//	std::cout << "replacement" << std::endl;
//	print_classify_indices(std::cout, two);

	index_map_t ind_free, ind_dummy;
	index_map_t ind_free_full, ind_dummy_full;

	if(still_inside_algo) {
		classify_indices_up(tr.parent(two), ind_free_full, ind_dummy_full);
//		print_classify_indices(std::cout, tr.parent(two));
		}
	else {
//		txtout << "classify indices up" << *(tr.parent(two)->name) << std::endl;
		classify_indices_up(two, ind_free_full, ind_dummy_full); // the indices in everything except the replacement
//		print_classify_indices(std::cout, two);
		}
	classify_indices(two, ind_free, ind_dummy); // the indices in the replacement subtree

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

int Algorithm::max_numbered_name_one(const std::string& nm, const index_map_t * one) const
	{
	assert(one);

	int themax=0;
	index_map_t::const_iterator it=one->begin();
	while(it!=one->end()) {
		size_t pos=(*it->first.begin()->name).find_first_of("0123456789");
		if(pos!=std::string::npos) {
//			txtout << (*it->first).substr(0,pos) << std::endl;
			if((*it->first.begin()->name).substr(0,pos) == nm) {
				int thenum=atoi((*it->first.begin()->name).substr(pos).c_str());
//				txtout << "num = " << thenum << std::endl;
				themax=std::max(themax, thenum);
				}
			}
		++it;
		}
	return themax;
	}

int Algorithm::max_numbered_name(const std::string& nm, 
											const index_map_t * one, 
											const index_map_t * two,
											const index_map_t * three,
											const index_map_t * four,
											const index_map_t * five) const
	{
	int themax=0;
	if(one) {
		themax=std::max(themax, max_numbered_name_one(nm, one));
		if(two) {
			themax=std::max(themax, max_numbered_name_one(nm, two));
			if(three) {
				themax=std::max(themax, max_numbered_name_one(nm, three));
				if(four) {
					themax=std::max(themax, max_numbered_name_one(nm, four));
					if(five) {
						themax=std::max(themax, max_numbered_name_one(nm, five));
						}
					}
				}
			}
		}
	return themax;
	}

bool Algorithm::index_in_set(Ex ex, const index_map_t *im) const
	{
	if(im==0) return false;

	if(im->count(ex)>0) return true;

	if(ex.begin()->fl.parent_rel==str_node::p_super) {
		ex.begin()->fl.parent_rel=str_node::p_sub;
		int c=im->count(ex);
		if(c>0) return true;
		}
	if(ex.begin()->fl.parent_rel==str_node::p_sub) {
		ex.begin()->fl.parent_rel=str_node::p_super;
		int c=im->count(ex);
		if(c>0) return true;
		}
	return false;
	}

Ex Algorithm::get_dummy(const list_property *dums,
												  const index_map_t * one, 
												  const index_map_t * two,
												  const index_map_t * three,
												  const index_map_t * four,
												  const index_map_t * five) const
	{
	std::pair<Properties::pattern_map_t::const_iterator, Properties::pattern_map_t::const_iterator>
		pr=kernel.properties.pats.equal_range(dums);

	// std::cerr << "finding index not in: " << std::endl;
	// if(one)
	// 	for(auto& i: *one)
	// 		std::cerr << i.first << std::endl;
	// if(two)
	// 	for(auto& i: *two)
	// 		std::cerr << i.first << std::endl;
	// if(three)
	// 	for(auto& i: *three)
	// 		std::cerr << i.first << std::endl;
	// if(four)
	// 	for(auto& i: *four)
	// 		std::cerr << i.first << std::endl;
	// if(five)
	// 	for(auto& i: *five)
	// 		std::cerr << i.first << std::endl;
	
	while(pr.first!=pr.second) {
		// std::cerr << "trying " << pr.first->second->obj << std::endl;
		if(pr.first->second->obj.begin()->is_autodeclare_wildcard()) {
			std::string base=*pr.first->second->obj.begin()->name_only();
			int used=max_numbered_name(base, one, two, three, four, five);
			std::ostringstream str;
			str << base << used+1;
//			txtout << "going to use " << str.str() << std::endl;
			nset_t::iterator newnm=name_set.insert(str.str()).first;
			Ex ret;
			ret.set_head(str_node(newnm));
			return ret;
			}
		else {
			const Ex& inm=(*pr.first).second->obj;
			// BUG: even if only _{a} is in the used map, we should not
			// accept ^{a}. But since ...
			if(index_in_set(inm, one)==false   &&
				index_in_set(inm, two)==false   &&
				index_in_set(inm, three)==false &&
				index_in_set(inm, four)==false  &&
				index_in_set(inm, five)==false) {
				// std::cerr << "ok to use " << inm << std::endl;
				return inm;
				}
			}
		++pr.first;
		}

	const Indices *dd=dynamic_cast<const Indices *>(dums);
	assert(dd);
	throw ConsistencyException("Ran out of dummy indices for type \""+dd->set_name+"\".");
	}

Ex Algorithm::get_dummy(const list_property *dums, iterator it) const
	{
	index_map_t one, two, three, four, five;
	classify_indices_up(it, one, two);
	classify_indices(it, three, four);
	
	return get_dummy(dums, &one, &two, &three, &four, 0);
	}

// Find a dummy index of the type given in "nm", making sure that this index
// name does not class with the object in it1 nor it2.

Ex Algorithm::get_dummy(const list_property *dums, iterator it1, iterator it2) const
	{
	index_map_t one, two, three, four, five;
	classify_indices_up(it1, one, two);
	classify_indices_up(it2, one, two);
	classify_indices(it1, three, four);
	classify_indices(it2, three, four);
	
	return get_dummy(dums, &one, &two, &three, &four, 0);
	}

void Algorithm::print_classify_indices(std::ostream& str, iterator st) const
	{
	str << "for node " << Ex(st) << std::endl;
	index_map_t ind_free, ind_dummy;
	classify_indices(st, ind_free, ind_dummy);
	
	index_map_t::iterator it=ind_free.begin();
	index_map_t::iterator prev=ind_free.end();
	str << "free indices: " << std::endl;
	while(it!=ind_free.end()) {
		if(prev==ind_free.end() || tree_exact_equal(&kernel.properties, (*it).first,(*prev).first,1,true,-2,true)==false)
			str << *(*it).first.begin()->name << " (" << ind_free.count((*it).first) << ") ";
		prev=it;
		++it;
		}
	str << std::endl;
	it=ind_dummy.begin();
	prev=ind_dummy.end();
	str << "dummy indices: ";
	while(it!=ind_dummy.end()) {
		if(prev==ind_dummy.end() || tree_exact_equal(&kernel.properties, (*it).first,(*prev).first,1,true,-2,true)==false)
			str << *(*it).first.begin()->name << " (" << ind_dummy.count((*it).first) << ") ";
		prev=it;
		++it;
		}
	str << "---" << std::endl;
	}

// For each iterator in the original map, find the sequential position of the index.
// That is, the index 'd' has position '3' in A_{a b} C_{c} D_{d}.
// WARNING: expensive operation.
//
void Algorithm::fill_index_position_map(iterator prodnode, const index_map_t& im, index_position_map_t& ipm) const
	{
	ipm.clear();
	index_map_t::const_iterator imit=im.begin();
	while(imit!=im.end()) {
		int current_pos=0;
		bool found=false;
		index_iterator indexit=begin_index(prodnode);
		while(indexit!=end_index(prodnode)) {
			if(imit->second==(iterator)(indexit)) {
				ipm.insert(index_position_map_t::value_type(imit->second, current_pos));
				found=true;
				break;
				}
			++current_pos;
			++indexit;
			}
		if(!found) 
			throw ConsistencyException("Internal error in fill_index_position_map; cannot find index "
											+ *(imit->first.begin()->name)+".");
		++imit;
		}
	}

void Algorithm::fill_map(index_map_t& mp, sibling_iterator st, sibling_iterator nd) const
	{
	while(st!=nd) {
		mp.insert(index_map_t::value_type(Ex(st), iterator(st)));
		++st;
		}
	}

// Determine those indices in 'two' which have a name which is identical to
// an index name occurring in 'one'. Store these indices of 'two' in target.
// If 'move_out' is true, instead move both the indices in 'one' and 'two' 
// (i.e. move instead of copy, and also store the 'one' index).
//
// One exception: numerical, coordinate and symbol indices are always kept in 'one'.
//
void Algorithm::determine_intersection(index_map_t& one, index_map_t& two, index_map_t& target, bool move_out)  const
	{
	index_map_t::iterator it1=one.begin();
	while(it1!=one.end()) {
		const Coordinate *cdn=kernel.properties.get<Coordinate>(it1->second, true);
		const Symbol     *smb=Symbol::get(kernel.properties, it1->second, true);
		if(it1->second->is_integer()==false && !cdn && !smb) {
			bool move_this_one=false;
			index_map_t::iterator it2=two.begin();
			while(it2!=two.end()) {
				if(tree_exact_equal(&kernel.properties, (*it1).first,(*it2).first,1,true,-2,true)) {
					target.insert((*it2));
					if(move_out) {
						index_map_t::iterator nxt=it2;
						++nxt;
						two.erase(it2);
						it2=nxt;
						move_this_one=true;
						}
					else ++it2;
					}
				else ++it2;
				}
			Ex the_key=(*it1).first;
			if(move_this_one && move_out) {
				index_map_t::iterator nxt=it1;
				++nxt;
				target.insert(*it1);
				one.erase(it1);
				it1=nxt;
				}
			else ++it1;
			// skip all indices in two with the same name
			while(it1!=one.end() && tree_exact_equal(&kernel.properties, (*it1).first,the_key,1,true,-2,true)) {
				if(move_this_one && move_out) {
					index_map_t::iterator nxt=it1;
					++nxt;
					target.insert(*it1);
					one.erase(it1);
					it1=nxt;
					}
				else ++it1;
				}
			}
		else ++it1;
		}
	}

Algorithm::index_map_t::iterator Algorithm::find_modulo_parent_rel(iterator it, index_map_t& imap)  const
	{
	auto fnd=imap.find(it);
	if(fnd==imap.end()) {
		it->flip_parent_rel();
		fnd=imap.find(it);
		it->flip_parent_rel();
		return fnd;
		}
	return fnd;
	}

// Directly add an index to the free/dummy sets, as appropriate (only add if this really is an 
// index!)

void Algorithm::classify_add_index(iterator it, index_map_t& ind_free, index_map_t& ind_dummy) const
	{
	if((it->fl.parent_rel==str_node::p_sub || it->fl.parent_rel==str_node::p_super) &&
		it->fl.bracket==str_node::b_none /* && it->is_integer()==false */) {
		const Coordinate *cdn=kernel.properties.get<Coordinate>(it, true);
		const Symbol     *smb=Symbol::get(kernel.properties, it, true);
		 if(it->is_integer() || cdn || smb)
			  ind_free.insert(index_map_t::value_type(Ex(it), it));
		 else {
			 index_map_t::iterator fnd=find_modulo_parent_rel(it, ind_free);
			 if(fnd!=ind_free.end()) {
				 // std::cerr << "found in free indices" << std::endl;
				 if(ind_dummy.count(it)>0) {
					 throw ConsistencyException("Triple index occurred.");
					 }
				 ind_dummy.insert(*fnd);
				 ind_dummy.insert(index_map_t::value_type(Ex(it), it));
				 ind_free.erase(fnd);
				 }
			 else {
				 // std::cerr << "not yet found; after insertion" << std::endl;
				 ind_free.insert(index_map_t::value_type(Ex(it), it));
				 // for(auto& n: ind_free)
				 //	 std::cerr << n.first << std::endl;
				 }
			 }
		 }
	}

// This classifies indices bottom-up, that is, given a node, it goes up the tree to find
// all free and dummy indices in the product in which this node would end up if a full
// distribute would be done on the entire expression. 
void Algorithm::classify_indices_up(iterator it, index_map_t& ind_free, index_map_t& ind_dummy)  const
	{
	loopie:
	iterator par=Ex::parent(it);
	if(tr.is_valid(par)==false || par==tr.end() || *par->name=="\\expression" || *par->name=="\\history") { // reached the top
		return;
		}
	const IndexInherit *inh=kernel.properties.get<IndexInherit>(par);

	if(*par->name=="\\sum" || *par->name=="\\equals") {
		// sums or equal signs are no problem since the other terms do not end up in our
		// factor; therefore, just go up.
		it=par;
		goto loopie;
		}
	else if(*par->name=="\\fermibilinear" || inh) {
		// For each _other_ child in this product, do a top-down classify for all non-sub/super
      // children; add the indices thus found to the maps since they will end up in our factor.
		sibling_iterator sit=par.begin();
		while(sit!=par.end()) {
			if(sit!=sibling_iterator(it)) {
				if(sit->is_index()==false) {
					index_map_t factor_free, factor_dummy;
					classify_indices(sit, factor_free, factor_dummy);
					
					// Test for absence of triple or quadruple indices
					index_map_t must_be_empty;
					determine_intersection(factor_free, ind_dummy, must_be_empty);
					if(must_be_empty.size()>0)
						throw ConsistencyException("Triple index occurred.");
					
					// Test for absence of double index pairs
					must_be_empty.clear();
					determine_intersection(factor_dummy, ind_dummy, must_be_empty);
					if(must_be_empty.size()>0)
						throw ConsistencyException("Double index pair occurred.");
					
					ind_dummy.insert(factor_dummy.begin(), factor_dummy.end());
					index_map_t new_dummy;
					determine_intersection(factor_free, ind_free, new_dummy, true);
					ind_free.insert(factor_free.begin(), factor_free.end());
					ind_dummy.insert(new_dummy.begin(), new_dummy.end());
					}
				else {
//					ind_free.insert(free_so_far.begin(), free_so_far.end());
//					free_so_far.clear();
					classify_add_index(sit, ind_free, ind_dummy);
					}
				}
			++sit;
			}
		it=par;
		goto loopie;
		}
	else if(*par->name=="\\expression") { // reached the top
		index_sw.stop();
		return;
		}
	else if((*par->name).size()>0 && (*par->name)[0]=='@') { // command nodes swallow everything
		index_sw.stop();
		return;
		}
	else if(*par->name=="\\tie") { // tie lists do not care about indices
		ind_free.clear();
		ind_dummy.clear();
		it=par;
		goto loopie;
		}
	else if(*par->name=="\\arrow") { // rules can have different indices on lhs and rhs
//		ind_free.clear();
//		ind_dummy.clear();
		it=par;
		goto loopie;
		}
//	else if(*par->name=="\\indexbracket") { // it's really just a bracket, so go up
//		sibling_iterator sit=tr.begin(par);
//		++sit;
//		while(sit!=tr.end(par)) {
//			++sit;
//			}
//		it=par;
//		goto loopie;
//		} 
	else if(*par->name=="\\comma") { // comma lists can contain anything NO: [a_{mu}, b_{nu}]
		// reaching a comma node is like reaching the top of an expression.
		return;
		}

// FIXME: do something with these warnings!!
//	txtout << "Index classification for this expression failed because of " 
//			 << *par->name << " node, disabling index checking." << std::endl;
//	assert(1==0);
	ind_free.clear();
	ind_dummy.clear();
	}

void Algorithm::dumpmap(std::ostream& str, const index_map_t& mp) const
	{
	index_map_t::const_iterator dpr=mp.begin();
	while(dpr!=mp.end()) {
		str << *(dpr->first.begin()->name) << " ";
		++dpr;
		}
	str << std::endl;
	}

// This classifies indices top-down, that is, finds the free indices and all dummy 
// index pairs used in the full subtree below a given node.
void Algorithm::classify_indices(iterator it, index_map_t& ind_free, index_map_t& ind_dummy) const
	{
	index_sw.start();
//	debugout << "   " << *it->name << std::endl;
	const IndexInherit *inh=kernel.properties.get<IndexInherit>(it);
	if(*it->name=="\\sum" || *it->name=="\\equals") {
		index_map_t first_free;
		sibling_iterator sit=it.begin();
		bool is_first_term=true;
		while(sit!=it.end()) {
			if(*sit->multiplier!=0) { // zeroes are always ok
				index_map_t term_free, term_dummy;
				// print_classify_indices(std::cerr, sit);
				classify_indices(sit, term_free, term_dummy);
				if(!is_first_term) {
					index_map_t::iterator fri=first_free.begin();
					while(fri!=first_free.end()) {
						const Coordinate *cdn=kernel.properties.get_composite<Coordinate>(fri->second, true);
						const Symbol     *smb=kernel.properties.get_composite<Symbol>(fri->second, true); //Symbol::get(kernel.properties, fri->second, true);
                  // integer, coordinate or symbol indices always ok
						if(fri->second->is_integer()==false && !cdn && !smb) { 
							if(term_free.count((*fri).first)==0) {
								// std::cerr << (*fri).first << std::endl;
								// std::cerr << "did not find Symbol for " << fri->second << std::endl;
								if(*it->name=="\\sum") 
									throw ConsistencyException("Free indices in different terms in a sum do not match.");
								else
									throw ConsistencyException("Free indices on lhs and rhs do not match.");
								}
							}
						++fri;
						}
					fri=term_free.begin();
					while(fri!=term_free.end()) {
						const Coordinate *cdn=kernel.properties.get_composite<Coordinate>(fri->second, true);
						const Symbol     *smb=kernel.properties.get_composite<Symbol>(fri->second, true); //Symbol::get(kernel.properties, fri->second, true);
                  // integer, coordinate or symbol indices always ok
						if(fri->second->is_integer()==false && !cdn && !smb) { 
							if(first_free.count((*fri).first)==0) {
								if(*it->name=="\\sum") {
									std::cerr << (*fri).first << " not in first term" << std::endl;
									 std::cerr << Ex(it) << std::endl;
									throw ConsistencyException("Free indices in different terms in a sum do not match.");
									}
								else
									throw ConsistencyException("Free indices on lhs and rhs do not match.");
								}
							}
						++fri;
						}
					}
				else {
					first_free=term_free;
					is_first_term=false;
					}
				
				ind_dummy.insert(term_dummy.begin(), term_dummy.end());
				ind_free.insert(term_free.begin(), term_free.end());
				}
			++sit;
			}
		}
	else if(inh) {
		index_map_t free_so_far;
		sibling_iterator sit=it.begin();
		while(sit!=it.end()) {
			if(sit->is_index()==false) {
				index_map_t factor_free, factor_dummy;
				classify_indices(sit, factor_free, factor_dummy);
				
				// Test for absence of triple or quadruple indices
				index_map_t must_be_empty;
				determine_intersection(factor_free, ind_dummy, must_be_empty);
				if(must_be_empty.size()>0) 
					 throw ConsistencyException("Triple index " 
													 + *(must_be_empty.begin()->second->name) 
													 + " inside a single factor found.");
				
				// Test for absence of double index pairs
				must_be_empty.clear();
				determine_intersection(factor_dummy, ind_dummy, must_be_empty);
				if(must_be_empty.size()>0)
					throw ConsistencyException("Double index pair " 
													+ *(must_be_empty.begin()->second->name) 
													+ " inside a single factor found.");
				
				ind_dummy.insert(factor_dummy.begin(), factor_dummy.end());
				index_map_t new_dummy;
				determine_intersection(factor_free, free_so_far, new_dummy, true);
				free_so_far.insert(factor_free.begin(), factor_free.end());
				ind_dummy.insert(new_dummy.begin(), new_dummy.end());
				}
			else {
//				ind_free.insert(free_so_far.begin(), free_so_far.end());
//				free_so_far.clear();
				//std::cerr << "adding index " << Ex(sit) << std::endl;
				classify_add_index(sit, free_so_far, ind_dummy);
				}
			++sit;
//			const Derivative *der=kernel.properties.get<Derivative>(it);
//			if(*it->name=="\\indexbracket" || der) { // the other children are indices themselves
//				ind_free.insert(free_so_far.begin(), free_so_far.end());
//				free_so_far.clear();
//				while(sit!=it.end()) {
//					classify_add_index(sit, ind_free, ind_dummy);
//					++sit;
//					}
//				break;
//				}
			}
		ind_free.insert(free_so_far.begin(), free_so_far.end());
		}
	else if(*it->name=="\\expression") {
		classify_indices(it.begin(), ind_free, ind_dummy);
		}
	else if(*it->name=="\\tie") {
		ind_free.clear();
		ind_dummy.clear();
		}
	else if((*it->name).size()>0 && (*it->name)[0]=='@') {
		// This is an active node that has not been replaced yet; since
		// we do not know anything about what this will become, do not return
		// any index information (clashes will be resolved when the active
		// node gets replaced).
		}
	else {
		sibling_iterator sit=it.begin();
		index_map_t item_free;
		index_map_t item_dummy;
		while(sit!=it.end()) {
//			std::cout << *sit->name << std::endl;
			if((sit->fl.parent_rel==str_node::p_sub || sit->fl.parent_rel==str_node::p_super) && sit->fl.bracket==str_node::b_none) {
				if(*sit->name!="??") {
					const Coordinate *cdn=kernel.properties.get<Coordinate>(sit, true);
//					const Symbol     *smb=Symbol::get(kernel.properties, sit, true);
					const Symbol     *smb=kernel.properties.get<Symbol>(sit, true); //Symbol::get(kernel.properties, sit, true);
					// integer, coordinate or symbol indices always ok
					if(sit->is_integer() || cdn || smb) {
						// Note: even integers need to be stored as indices, because we expect e.g. canonicalise
						// to re-order even numerical indices. They should just never be flagged as dummies.
						item_free.insert(index_map_t::value_type(Ex(sit), iterator(sit)));
						}
					else {
						index_map_t::iterator fnd=find_modulo_parent_rel(sit, item_free);
//						index_map_t::iterator fnd=item_free.find(Ex(sit));
						if(fnd!=item_free.end()) {
//							std::cerr << *sit->name << " already in free set" << std::endl;
//							if(item_dummy.find(Ex(sit))!=item_dummy.end())
							if(find_modulo_parent_rel(sit, item_dummy)!=item_dummy.end())
								throw ConsistencyException("Triple index " + *sit->name + " inside a single factor found.");
							item_dummy.insert(*fnd);
							item_free.erase(fnd);
							item_dummy.insert(index_map_t::value_type(Ex(sit), iterator(sit)));
//							std::cout << item_dummy.size() << " " << item_free.size() << std::endl;
							}
						else {
//							std::cerr << *sit->name << " is new" << std::endl;
							item_free.insert(index_map_t::value_type(Ex(sit), iterator(sit)));
							}
						}
					}
				}
//			else {
//				item_free.insert(index_map_t::value_type(sit->name, iterator(sit)));
//				}
			++sit;
			}
		ind_free.insert(item_free.begin(), item_free.end());
		ind_dummy.insert(item_dummy.begin(), item_dummy.end());
		}
//	std::cout << "ind_free: " << ind_free.size() << std::endl;
//	std::cout << "ind_dummy: " << ind_dummy.size() << std::endl;

	index_sw.stop();
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
	if(tr.is_valid(tr.parent(it))) {
		if(*tr.parent(it)->name=="\\sum" || 
			(*tr.parent(it)->name=="\\expression" && *it->name!="\\sum") ||
			tr.parent(it)->is_command() ) 
			return true;
		}
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

bool Algorithm::is_single_term(iterator it)
	{
	if(*it->name!="\\prod" && *it->name!="\\sum" && *it->name!="\\asymimplicit" && *it->name!="\\comma" 
		&& *it->name!="\\equals" && *it->name!="\\arrow" && *it->name!="\\expression" ) {
		if(tr.is_valid(tr.parent(it))) {
			if(*tr.parent(it)->name=="\\sum" || *tr.parent(it)->name=="\\expression" || tr.parent(it)->is_command())
				return true;
//			if(*tr.parent(it)->name!="\\prod" && 
//				it->fl.parent_rel==str_node::p_none) // object is an argument of a wrapping object, not an index
//				return true;
			}
		else return true;
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
		force_prod_wrap(it);
		return true;
		}
	else return false;
	}

void Algorithm::force_prod_wrap(iterator& it)
	{
	iterator prodnode=tr.insert(it, str_node("\\prod"));
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

	// We accept either a tree with a \comma node at the top, or
	// a tree which is \expression{\comma{...}}.
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
