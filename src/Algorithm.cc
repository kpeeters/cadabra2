/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2014  Kasper Peeters <kasper.peeters@phi-sci.com>

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

#include "Algorithm.hh"
#include "Storage.hh"
#include "Props.hh"
#include "CoreProps.hh"

#include "properties/Derivative.hh"

#include <typeinfo>
#include <sstream>

stopwatch algorithm::index_sw;
stopwatch algorithm::get_dummy_sw;

active_node::active_node(exptree& tr_, iterator it_)
	: this_command(it_), tr(tr_), args_begin_(tr_.end()), args_end_(tr_.end())
	{
	}

exptree::sibling_iterator active_node::args_begin() const
	{
	if(args_begin_==tr.end() && this_command!=tr.end()) {
		args_begin_=tr.begin(this_command);
		args_end_  =tr.end(this_command);
		if(args_begin_!=args_end_) {
			if(args_begin_->fl.bracket==str_node::b_round ||
				args_begin_->fl.bracket==str_node::b_square)
				++args_begin_;
			}
		}
	return args_begin_;
	}

exptree::sibling_iterator active_node::args_end() const
	{
	args_begin();
	return args_end_;
	}

unsigned int active_node::number_of_args() const
	{
	sibling_iterator it=args_begin();
	unsigned int ret=0;
	while(it!=args_end()) {
		++ret;
		++it;
		}
	return ret;
	}

bool active_node::has_argument(const std::string& arg) const
	{
	sibling_iterator sit=args_begin();
	while(sit!=args_end()) {
		if(*sit->name==arg) return true;
		++sit;
		}
	return false;
	}

algorithm::constructor_error::constructor_error()
	{
	}


algorithm::algorithm(exptree& tr_, iterator it_)
	: active_node(tr_, it_),
	  expression_modified(false), 
	  equation_number(0), global_success(g_not_yet_started), 
	  number_of_calls(0), number_of_modifications(0),
	  suppress_normal_output(false),
	  discard_command_node(false)
	{
	}

algorithm::~algorithm()
	{
	}

bool algorithm::is_output_module() const
	{
	return false;
	}


// The entry point called by manipulator.cc
void algorithm::apply(unsigned int lue, bool multiple, bool until_nochange, bool make_copy, int act_at_level, 
							 bool called_by_manipulator)
	{
	if(called_by_manipulator) {
		report_progress_stopwatch.stop();
		report_progress_stopwatch.reset();
		}

	last_used_equation_number=lue;
	expression_modified=false;
	iterator actold=tr.end();
	iterator acton=tr.end();
	subtree=tr.end();

	// Figure out on which subtree the algorithm is supposed to act.

	if(tr.number_of_children(this_command)>=1) {
		iterator chld=tr.begin(this_command);
		if(chld->fl.bracket==str_node::b_round && *this_command->name!="@") { // all normal commands
			actold=tr.equation_by_number_or_name(chld, last_used_equation_number, equation_number);
			global_success=g_arguments_accepted;
			if(actold==tr.end()) {
				throw ConsistencyException("Expression ("
												+tr.equation_number_or_name(chld, last_used_equation_number)
												+") does not exist.");
				return;
				}
			else global_success=g_operand_determined;
			}
		else if(chld->fl.bracket==str_node::b_round && *this_command->name=="@") { // exception for '@(1)'
			equation_number=lue;
			global_success=g_operand_determined;
			acton=chld;
			}
		else if(chld->fl.bracket==str_node::b_square) {
			global_success=g_operand_determined;
			acton=chld;
//			if(chld->fl.bracket==str_node::b_square)
//				chld->fl.bracket=str_node::b_none; // otherwise the square brackets stay forever
			}
		}

	// Depending on the outcome, different actions should be taken to copy the
	// original tree and store the result.
	
	bool is_output_module=this->is_output_module(); //dynamic_cast<exptree_output *>(this);

	if(actold!=tr.end()) { // act on an existing expression
		previous_expression=tr.named_parent(tr.active_expression(actold), "\\expression");
		if(!is_output_module && make_copy)
			copy_expression(previous_expression);
		actold=tr.active_expression(actold);
		if(multiple) {
//			 debugout << "acting with " << *(this_command->name) << " multiple." << std::endl;
			subtree=actold;
			apply_recursive(subtree, true, act_at_level, called_by_manipulator, until_nochange);
			}
		else {
//			 debugout << "acting with " << *(this_command->name) << " single." << std::endl;
			subtree=tr.begin(actold);
			if(can_apply(subtree)) {
				++number_of_calls;
				report_progress((*this_command->name).substr(1,
																			(*this_command->name).size()-2), 
									 0,0,1);
				result_t res=apply(subtree);
				if(expression_modified) {
					++number_of_modifications;
					global_success=g_applied;
//					debugout << "**** " << std::endl;
//					tr.print_recursive_treeform(debugout, tr.begin());
//					debugout << "==== " << std::endl;
					if(getenv("CDB_PARANOID")) 					
						check_consistency(tr.named_parent(subtree,"\\expression"));
					}
				else if(is_output_module && res==l_applied)
					global_success=g_applied;
				if(res==l_error)
					global_success=g_apply_failed;
				}
			}
		if(!is_output_module) {
			if(global_success==g_apply_failed) {
				if(make_copy) {
					cancel_modification();
					}
				subtree=previous_expression;
				}
			}
		discard_command_node=true;
		}
	else if(acton!=tr.end()) { // act on argument
		subtree=acton;
		if(multiple) {
			 apply_recursive(subtree, true, act_at_level, called_by_manipulator, until_nochange);
			}
		else {
			if(can_apply(subtree)) {
				global_success=g_operand_determined;
				++number_of_calls;
				report_progress((*this_command->name).substr(1,
																			(*this_command->name).size()-2), 
									 0,0,1);
				apply(subtree);
				}
			}
		if(is_output_module)
			global_success=g_applied;
		if(/* expression_modified && */ subtree!=tr.end()) { // even if expr. not modified, keep original
			global_success=g_applied;
			check_consistency(tr.named_parent(subtree,"\\expression"));
			multiply(subtree->multiplier, *this_command->multiplier);
			subtree->fl.bracket=this_command->fl.bracket;
			subtree->fl.parent_rel=this_command->fl.parent_rel;
			
			// Rename dummy indices in the replacement to avoid clashes
			rename_replacement_dummies(subtree, true);
			// Then safely replace. First remove all children of the command
			// node which are not the replacement subtree.
			sibling_iterator sibrem=tr.begin(this_command);
			while(sibrem!=tr.end(this_command)) {
				if(sibrem!=subtree)
					sibrem=tr.erase(sibrem);
				else
					++sibrem;
				}
			// Now flatten the tree at the node.
			tr.flatten(this_command);
			tr.erase(this_command);
			// SOMETHING IS WRONG WITH ARGUMENT STUFF.
//			subtree=tr.replace(this_command,thiscp,subtree,citp);

			// There are various situations which need to be cleaned up, but can only be handled
			// here because algorithms themselves are not allowed to modify the tree above the
			// entry point. The cases are a) nested products, b) numerical factors in products,
			// c) one to the power something, d) numerical factors on sum nodes, e) nested sums.
			if(*subtree->name=="\\prod") {
				if(*tr.parent(subtree)->name=="\\prod") {
					multiply(tr.parent(subtree)->multiplier, *subtree->multiplier);
					tr.flatten(subtree);
					subtree=tr.erase(subtree);
					}
				}
			else {
				if(*tr.parent(subtree)->name=="\\prod") {
					if(*subtree->multiplier!=1) {
						multiply(tr.parent(subtree)->multiplier, *subtree->multiplier);
						subtree->multiplier=rat_set.insert(1).first;
						}
					}
//				else if(*tr.parent(subtree)->name=="\\pow") {
//					 // FIXME: NOT TRIGGERED?
//					 if(subtree->is_identity()) {
//						  node_one(tr.parent(subtree));
//						  subtree=tr.parent(subtree);
//						  }
//					 }
				else if(*subtree->name=="\\sum") {
					if(*subtree->multiplier!=1) {
						sibling_iterator sib=tr.begin(subtree);
						while(sib!=tr.end(subtree)) {
							multiply(sib->multiplier, *subtree->multiplier);
							++sib;
							}
						::one(subtree->multiplier);
						}
					if(*tr.parent(subtree)->name=="\\sum") {
						tr.flatten(subtree);
						subtree=tr.erase(subtree);
						}
					}
				}
			discard_command_node=false;
			}
		else {
			discard_command_node=true;
			subtree=tr.end();
			}
		}

	if(is_output_module) {
		suppress_normal_output=true;
		}
	}


// Returns whether the algorithm has applied at least once.
//
bool algorithm::apply_recursive(exptree::iterator& st, bool check_cons, int act_at_level, 
										  bool called_by_manipulator, bool until_nochange)
	{
	assert(tr.is_valid(st));
	assert(st!=tr.end());
	unsigned long count=0;
	bool atleastone=false;
	bool atleastoneglobal=false;
	post_order_iterator cit=st;
	post_order_iterator wit;
	exptree::fixed_depth_iterator fdi; // used when iterating at fixed depth
	int worked=0;
	int failed=0;

	long total_number_of_nodes=0;
	long processed_number_of_nodes=0;

	do { // loop which keeps iterating until the expression no longer changes
		post_order_iterator end;
		if(act_at_level!=-1) {
			fdi=tr.begin_fixed(st, act_at_level);
			wit=fdi;
			end=tr.end();
			}
		else {
			total_number_of_nodes=tr.size(cit);
			wit=cit;
			end=wit;
			wit.descend_all();
			++end;
			}
		atleastone=false;
		int num=1;
		int applied=0;

//		debugout << "entering loop... for " << typeid(*this).name()  << std::endl;
//		exptree::print_recursive_treeform(debugout, tr.begin());
		while(tr.is_valid(wit) && wit!=end) { // loop over the entire tree
			bool change_st=false; 
			iterator start=wit;
         // If we are at the top node and the algorithm changes the iterator 'start',
			// we have to propagate that change into 'st'.
			if(start==st) change_st=true; 
			if(can_apply(start)) {
//				debugout << "can apply, entry point:" << std::endl;
//				exptree::print_recursive_treeform(debugout, start);
				if(global_success<g_operand_determined)
					global_success=g_operand_determined;
				expression_modified=false;
				dont_iterate=false;

				post_order_iterator nextone=wit; 
				if(act_at_level==-1) {
					// Because nextone is a post_order_iterator, the increment that follows 
					// skips straight to the next sibling, not to the next child.
					processed_number_of_nodes+=tr.size(nextone);
					if(called_by_manipulator)
						report_progress((*this_command->name).substr(1,
																					(*this_command->name).size()-2), 
											 total_number_of_nodes, processed_number_of_nodes, 1);
					++nextone;
					}
				else { 
					++fdi;
					if(tr.is_valid(fdi)) nextone=fdi;
					else                 nextone=end;
					}
				count++;
				++number_of_calls;
				std::string www=*wit->name;
				result_t res=apply(start);
//				debugout << "after apply: " << *(start->multiplier) << std::endl;
//				exptree::print_recursive_treeform(debugout, start);
//				exptree::print_recursive_treeform(txtout, tr.begin());
				wit=start; // this copying back and forth is needed because wit has different type
				switch(res) {
					case l_no_action:
						++failed;
						wit=nextone;
						break;
					case l_applied:
						global_success=g_applied;
						++applied;
						if(change_st) 
							st=start;
						if(expression_modified) {
							++worked;
							++number_of_modifications;
							atleastone=true;
							atleastoneglobal=true;
							}
						// Handle zeroes and ones.
//						debugout << "handling zeroes and ones." << std::endl;
//						debugout << "start tree:" << std::endl;
//						debugout << *start->multiplier << std::endl;
//						exptree::print_recursive_treeform(debugout, start);
						if(wit!=tr.end() && *wit->multiplier==0) { 
							propagate_zeroes(wit,st);
							if(*wit->multiplier==0) { // a top-level zero 
								++wit;       
								}
							else if(act_at_level!=-1) wit=nextone; // do not follow post_order sequence
							}
						else if(wit!=tr.end() && wit->is_rational()) { // handle multipliers in products and identity powers
							bool tryprod=false;
							bool ispow=true;
							sibling_iterator tmpact=wit;
							if( *tr.parent(tmpact)->name=="\\pow" && wit->is_identity() ) {
								iterator par=tr.parent(tmpact);
								if( tmpact==tr.begin(par) ) { // 1**x = 1
									node_one(par);
									tmpact=par;
									tryprod=true;
									}
								else { // x**1 = x
									tr.erase(tmpact);
									tr.flatten(par);
									tr.erase(par);
									wit=nextone;
									}
								}
							else ispow=false;
							
							if( (!ispow || tryprod) && *tr.parent(tmpact)->name=="\\prod") {
								iterator tmp=tr.parent(tmpact);
								multiply(tmp->multiplier, *tmpact->multiplier);
								tr.erase(tmpact); // may leave us with 0 or 1 children
								cleanup_anomalous_products(tr,tmp);
								if(tryprod) wit=tmp;
								else        wit=nextone;
								}
							else {
								if(tryprod) wit=tmpact;
								else        wit=nextone;
								}
							}
						else {
							if(wit!=tr.end()) {
//								txtout << "THEN HERE" << std::endl;
								pushup_multiplier(wit); // Ensure a valid tree wrt. multipliers.
//								txtout << "THEN HERE DONE" << std::endl;
								}
							wit=nextone;
							}
						break;
					case l_error: 
						global_success=g_apply_failed;
						return atleastoneglobal;
						break;
					}
				}
			else {
				if(act_at_level==-1) ++wit;
				else {
					++fdi;
					if(tr.is_valid(fdi)) wit=fdi;
					else wit=end;
//					wit=tr.next_at_same_depth(wit);
					}
				}
			
			if(interrupted) 
				throw InterruptionException("apply_recursive");

			++num;
			}
		} while(until_nochange && atleastone); // enable this again at some point for repeatall type apply

	// Completely top-level zeroes did not get handled above.
	if(*st->name == "\\expression" && tr.begin(st)->is_zero()) {
		 tr.erase_children(tr.begin(st));
		 tr.begin(st)->name=name_set.insert("1").first;
		 }
//	tr.print_recursive_treeform(txtout, tr.begin());

	// Check consistency of the tree if requested.
	if(getenv("CDB_PARANOID")) 
		if(global_success==g_applied && check_cons) 
			check_consistency(tr.named_parent(cit,"\\expression"));
	//	txtout << "algorithm " << (this_command==tr.end()?"?":*this_command->name) << " worked " << worked 
	//			 << " failed " << failed << std::endl;

//	tr.debug_verify_consistency();
//	exptree::print_recursive_treeform(txtout, tr.begin());

	return atleastoneglobal;
	}

bool algorithm::prepare_for_modification(bool make_copy)
	{
	// Collect iterators pointing to all selected nodes and copy the
	// expression into a new \\expression node where the modifications
	// can be made.
	marks.clear();
//	tr.marked_nodes(marks);

	if(marks.size()==0) 
		return false;

//	previous_expression=tr.named_parent(marks[0], "\\expression");
	if(make_copy)
		copy_expression(previous_expression);
//	for(unsigned int i=0; i<marks.size(); ++i) 
//		tr.unselect(marks[i]);
	marks.clear();
//	tr.marked_nodes(marks);
	return true;
	}

void algorithm::copy_expression(exptree::iterator previous_expression) const
	{
//	txtout << "*** copying expression" << std::endl;
	assert(tr.is_valid(previous_expression));
	tr.append_child(tr.parent(previous_expression), previous_expression);
//	txtout << "*** copying expression done" << std::endl;
	}

void algorithm::cancel_modification()
	{
	if(tr.is_valid(previous_expression)) {
		iterator act=tr.active_expression(previous_expression);
		tr.erase(act);
		}
	}

void algorithm::propagate_zeroes(post_order_iterator& it, const iterator& topnode)
	{
	assert(*it->multiplier==0);
	if(it==topnode) return;
	iterator walk=tr.parent(it);
//	debugout << *walk->name << std::endl;
	if(!tr.is_valid(walk)) 
		return;

	const Derivative *der=properties::get<Derivative>(walk);
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

void algorithm::pushup_multiplier(iterator it) 
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
				const PropertyInherit *pin=properties::get<PropertyInherit>(tr.parent(it));
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

void algorithm::node_zero(iterator it)
	{
	::zero(it->multiplier);
	tr.erase_children(it);
	it->name=name_set.insert("1").first;
	}

void algorithm::node_one(iterator it)
	{
	::one(it->multiplier);
	tr.erase_children(it);
	it->name=name_set.insert("1").first;
	}

void algorithm::node_integer(iterator it, int num)
	{
	::one(it->multiplier);
	tr.erase_children(it);
	it->name=name_set.insert("1").first;
	::multiply(it->multiplier, num);
	}

int algorithm::index_parity(iterator it) const
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

bool algorithm::less_without_numbers(nset_t::iterator it1, nset_t::iterator it2) 
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

bool algorithm::equal_without_numbers(nset_t::iterator it1, nset_t::iterator it2) 
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

bool algorithm::check_index_consistency(iterator it) const 
	{
	index_map_t ind_free, ind_dummy;
	classify_indices(it,ind_free,ind_dummy);
	return true;
	}

bool algorithm::check_consistency(iterator it) const
	{
	stopwatch w1;
	w1.start();
//	debugout << "checking consistency ... " << std::flush;
	assert(*it->name=="\\expression");
	iterator entry=it;
	iterator end=it;
	end.skip_children();
	++end;
	while(it!=end) {
		if(interrupted)
			throw InterruptionException("check_consistency");

		if(*it->name=="\\sum") {
			if(*it->multiplier!=1)
				throw ConsistencyException("Found \\sum node with non-unit multiplier.");
			else if(exptree::number_of_children(it)<2)
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
			 if(exptree::number_of_children(it)<=1) 
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
			if(exptree::number_of_children(it)!=2)
				throw ConsistencyException("Found \\sequence node with incorrect (non-2) number of children.");
			}
		++it;
		}

	w1.stop();
//	debugout << "checking done..." << w1 << std::endl;
	return true;
	}

void algorithm::report_progress(const std::string& str, int todo, int done, int count) 
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
//		if(eo->output_format==exptree_output::out_xcadabra) {
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

bool algorithm::rename_replacement_dummies(iterator two, bool still_inside_algo) 
	{
//	txtout << "full story " << *two->name << std::endl;
//	print_classify_indices(tr.named_parent(one, "\\expression"));
//	txtout << "replacement" << std::endl;
//	print_classify_indices(two);

	index_map_t ind_free, ind_dummy;
	index_map_t ind_free_full, ind_dummy_full;

	if(still_inside_algo) {
		classify_indices_up(tr.parent(two), ind_free_full, ind_dummy_full);
//		print_classify_indices(tr.parent(two));
		}
	else {
//		txtout << "classify indices up" << *(tr.parent(two)->name) << std::endl;
		classify_indices_up(two, ind_free_full, ind_dummy_full); // the indices in everything except the replacement
		}
	classify_indices(two, ind_free, ind_dummy); // the indices in the replacement subtree

	index_map_t must_be_empty;
	index_map_t newly_generated;

	// Catch double index pairs
	determine_intersection(ind_dummy_full, ind_dummy, must_be_empty);
	index_map_t::iterator it=must_be_empty.begin();
	while(it!=must_be_empty.end()) {
//		txtout << "double index pair " << *((*it).first.begin()->name) 
//				 << " (index appears " << must_be_empty.count((*it).first) 
//				 << " times); renaming..." << std::endl;
		exptree the_key=(*it).first;
		const Indices *dums=properties::get<Indices>(it->second, true);
		if(!dums)
			throw ConsistencyException("Failed to find dummy property for $"+*it->second->name+"$ while renaming dummies.");
//			txtout << "failed to find dummy property for " << *it->second->name << std::endl;
		assert(dums);
		exptree relabel
            =get_dummy(dums, &ind_dummy_full, &ind_dummy, &ind_free_full, &ind_free, &newly_generated);
		newly_generated.insert(index_map_t::value_type(exptree(relabel),(*it).second));
//		txtout << " renamed to " << *relabel << std::endl;
		do {
			tr.replace_index((*it).second, relabel.begin());
//			(*it).second->name=relabel;
			++it;
			} while(it!=must_be_empty.end() && tree_exact_equal((*it).first,the_key, 1, true, -2, true));
		}
								 
	// Catch triple indices (two cases: dummy pair in replacement, free index elsewhere and 
	// dummy elsewhere, free index in replacement)
	must_be_empty.clear();
//	newly_generated.clear(); // DO NOT ERASE, IDIOT!

	determine_intersection(ind_free_full, ind_dummy, must_be_empty);
	it=must_be_empty.begin();
	while(it!=must_be_empty.end()) {
//		txtout << "triple index " << *((*it).first) 
//				 << " (index appears " << must_be_empty.count((*it).first) 
//				 << " times); renaming..." << std::endl;
		exptree the_key=(*it).first;
		const Indices *dums=properties::get<Indices>(it->second, true);
		if(!dums)
			 throw ConsistencyException("Failed to find dummy property for $"+*it->second->name+"$ while renaming dummies.");
		assert(dums);
		exptree relabel
            =get_dummy(dums, &ind_dummy_full, &ind_dummy, &ind_free_full, &ind_free, &newly_generated);
		newly_generated.insert(index_map_t::value_type(relabel,(*it).second));
		do {
			tr.replace_index((*it).second, relabel.begin());
//			(*it).second->name=relabel;
			++it;
			} while(it!=must_be_empty.end() && tree_exact_equal((*it).first,the_key, 1, true, -2, true));
		}

	must_be_empty.clear();
//	newly_generated.clear();
	determine_intersection(ind_free, ind_dummy_full, must_be_empty);
	it=must_be_empty.begin();
	while(it!=must_be_empty.end()) {
//		txtout << "triple index " << *((*it).first) 
//				 << " (index appears " << must_be_empty.count((*it).first) 
//				 << " times); renaming..." << std::endl;
		exptree the_key=(*it).first;
		const Indices *dums=properties::get<Indices>(it->second, true);
		if(!dums)
			 throw ConsistencyException("Failed to find dummy property for $"+*it->second->name+"$ while renaming dummies.");
		assert(dums);
		exptree relabel
            =get_dummy(dums, &ind_dummy_full, &ind_dummy, &ind_free_full, &ind_free, &newly_generated);
		newly_generated.insert(index_map_t::value_type(relabel,(*it).second));
		do {
			tr.replace_index((*it).second, relabel.begin());
			++it;
			} while(it!=must_be_empty.end() && tree_exact_equal((*it).first,the_key, 1, true, -2, true));
		}

	return true;
	}

int algorithm::max_numbered_name_one(const std::string& nm, const index_map_t * one) const
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

int algorithm::max_numbered_name(const std::string& nm, 
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

exptree algorithm::get_dummy(const list_property *dums,
												  const index_map_t * one, 
												  const index_map_t * two,
												  const index_map_t * three,
												  const index_map_t * four,
												  const index_map_t * five) const
	{
	std::pair<properties::pattern_map_t::iterator, properties::pattern_map_t::iterator>
		pr=properties::pats.equal_range(dums);
	
	while(pr.first!=pr.second) {
//		txtout << "trying " << std::endl;
//		tr.print_recursive_treeform(txtout, (*pr.first).second->obj.begin());
		if(pr.first->second->obj.begin()->is_autodeclare_wildcard()) {
			std::string base=*pr.first->second->obj.begin()->name_only();
			int used=max_numbered_name(base, one, two, three, four, five);
			std::ostringstream str;
			str << base << used+1;
//			txtout << "going to use " << str.str() << std::endl;
			nset_t::iterator newnm=name_set.insert(str.str()).first;
			exptree ret;
			ret.set_head(str_node(newnm));
			return ret;
			}
		else {
			const exptree& inm=(*pr.first).second->obj;
			if(!one || one->count(inm)==0)
				if(!two || two->count(inm)==0)
					if(!three || three->count(inm)==0) 
						if(!four || four->count(inm)==0) 
							if(!five || five->count(inm)==0) {
								return inm;
								}
			}
		++pr.first;
		}

	const Indices *dd=dynamic_cast<const Indices *>(dums);
	assert(dd);
	throw ConsistencyException("Ran out of dummy indices for type \""+dd->set_name+"\".");
	}

exptree algorithm::get_dummy(const list_property *dums, iterator it) const
	{
	index_map_t one, two, three, four, five;
	classify_indices_up(it, one, two);
	classify_indices(it, three, four);
	
	return get_dummy(dums, &one, &two, &three, &four, 0);
	}

// Find a dummy index of the type given in "nm", making sure that this index
// name does not class with the object in it1 nor it2.

exptree algorithm::get_dummy(const list_property *dums, iterator it1, iterator it2) const
	{
	index_map_t one, two, three, four, five;
	classify_indices_up(it1, one, two);
	classify_indices_up(it2, one, two);
	classify_indices(it1, three, four);
	classify_indices(it2, three, four);
	
	return get_dummy(dums, &one, &two, &three, &four, 0);
	}

// FIXME: make print to a given stream
void algorithm::print_classify_indices(iterator st) const
	{
	index_map_t ind_free, ind_dummy;
	classify_indices(st, ind_free, ind_dummy);
	
	index_map_t::iterator it=ind_free.begin();
	index_map_t::iterator prev=ind_free.end();
//	txtout << "free indices: " << std::endl;
	while(it!=ind_free.end()) {
		if(prev==ind_free.end() || tree_exact_equal((*it).first,(*prev).first,1,true,-2,true)==false)
//			txtout << *(*it).first.begin()->name << " (" << ind_free.count((*it).first) << ") ";
		prev=it;
		++it;
		}
//	txtout << std::endl;
	it=ind_dummy.begin();
	prev=ind_dummy.end();
//	txtout << "dummy indices: ";
	while(it!=ind_dummy.end()) {
		if(prev==ind_dummy.end() || tree_exact_equal((*it).first,(*prev).first,1,true,-2,true)==false)
//			txtout << *(*it).first.begin()->name << " (" << ind_dummy.count((*it).first) << ") ";
		prev=it;
		++it;
		}
//	txtout << std::endl;
	}

// For each iterator in the original map, find the sequential position of the index.
// That is, the index 'd' has position '3' in A_{a b} C_{c} D_{d}.
// WARNING: expensive operation.
//
void algorithm::fill_index_position_map(iterator prodnode, const index_map_t& im, index_position_map_t& ipm) const
	{
	ipm.clear();
	index_map_t::const_iterator imit=im.begin();
	while(imit!=im.end()) {
		int current_pos=0;
		bool found=false;
		exptree::index_iterator indexit=tr.begin_index(prodnode);
		while(indexit!=tr.end_index(prodnode)) {
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

void algorithm::fill_map(index_map_t& mp, sibling_iterator st, sibling_iterator nd) const
	{
	while(st!=nd) {
		mp.insert(index_map_t::value_type(exptree(st), iterator(st)));
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
void algorithm::determine_intersection(index_map_t& one, index_map_t& two, index_map_t& target, bool move_out)  const
	{
	index_map_t::iterator it1=one.begin();
	while(it1!=one.end()) {
		const Coordinate *cdn=properties::get<Coordinate>(it1->second, true);
		const Symbol     *smb=Symbol::get(it1->second, true);
		if(it1->second->is_integer()==false && !cdn && !smb) {
			bool move_this_one=false;
			index_map_t::iterator it2=two.begin();
			while(it2!=two.end()) {
				if(tree_exact_equal((*it1).first,(*it2).first,1,true,-2,true)) {
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
			exptree the_key=(*it1).first;
			if(move_this_one && move_out) {
				index_map_t::iterator nxt=it1;
				++nxt;
				target.insert(*it1);
				one.erase(it1);
				it1=nxt;
				}
			else ++it1;
			// skip all indices in two with the same name
			while(it1!=one.end() && tree_exact_equal((*it1).first,the_key,1,true,-2,true)) {
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

// Directly add an index to the free/dummy sets, as appropriate (only add if this really is an 
// index!)

void algorithm::classify_add_index(iterator it, index_map_t& ind_free, index_map_t& ind_dummy) const
	{
	if((it->fl.parent_rel==str_node::p_sub || it->fl.parent_rel==str_node::p_super) &&
		it->fl.bracket==str_node::b_none /* && it->is_integer()==false */) {
		const Coordinate *cdn=properties::get<Coordinate>(it, true);
		const Symbol     *smb=Symbol::get(it, true);
		 if(it->is_integer() || cdn || smb)
			  ind_free.insert(index_map_t::value_type(exptree(it), it));
		 else {
			  index_map_t::iterator fnd=ind_free.find(it);
			  if(fnd!=ind_free.end()) {
					if(ind_dummy.count(it)>0) {
						throw ConsistencyException("Triple index occurred.");
						 }
					ind_dummy.insert(*fnd);
					ind_dummy.insert(index_map_t::value_type(exptree(it), it));
					ind_free.erase(fnd);
					}
			  else {
					ind_free.insert(index_map_t::value_type(exptree(it), it));
					}
			  }
		 }
	}

// This classifies indices bottom-up, that is, given a node, it goes up the tree to find
// all free and dummy indices in the product in which this node would end up if a full
// distribute would be done on the entire expression. 
void algorithm::classify_indices_up(iterator it, index_map_t& ind_free, index_map_t& ind_dummy)  const
	{
	loopie:
	iterator par=exptree::parent(it);
	if(tr.is_valid(par)==false || par==tr.end() || *par->name=="\\expression" || *par->name=="\\history") { // reached the top
		return;
		}
	const IndexInherit *inh=properties::get<IndexInherit>(par);

//	txtout << "class: " << *par->name << std::endl;
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

void algorithm::dumpmap(std::ostream& str, const index_map_t& mp) const
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
void algorithm::classify_indices(iterator it, index_map_t& ind_free, index_map_t& ind_dummy) const
	{
	index_sw.start();
//	debugout << "   " << *it->name << std::endl;
	const IndexInherit *inh=properties::get<IndexInherit>(it);
	if(*it->name=="\\sum" || *it->name=="\\equals") {
		index_map_t first_free;
		sibling_iterator sit=it.begin();
		bool is_first_term=true;
		while(sit!=it.end()) {
			if(*sit->multiplier!=0) { // zeroes are always ok
				index_map_t term_free, term_dummy;
				classify_indices(sit, term_free, term_dummy);
				if(!is_first_term) {
					index_map_t::iterator fri=first_free.begin();
					while(fri!=first_free.end()) {
						const Coordinate *cdn=properties::get_composite<Coordinate>(fri->second, true);
						const Symbol     *smb=Symbol::get(fri->second, true);
                  // integer, coordinate or symbol indices always ok
						if(fri->second->is_integer()==false && !cdn && !smb) { 
							if(term_free.count((*fri).first)==0) {
//								debugout << "check 1" << std::endl;
//								debugout << "free indices elsewhere: ";
//								dumpmap(debugout, first_free);
//								debugout << "free indices here     : ";
//								dumpmap(debugout, term_free);
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
						const Coordinate *cdn=properties::get_composite<Coordinate>(fri->second, true);
						const Symbol     *smb=Symbol::get(fri->second, true);
                  // integer, coordinate or symbol indices always ok
						if(fri->second->is_integer()==false && !cdn && !smb) { 
							if(first_free.count((*fri).first)==0) {
//								debugout << "check 2" << std::endl;
//								debugout << "free indices elsewhere: ";
//								dumpmap(debugout, first_free);
//								debugout << "free indices here     : ";
//								dumpmap(debugout, term_free);
								if(*it->name=="\\sum")
									throw ConsistencyException("Free indices in different terms in a sum do not match.");
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
//			txtout << "free_so_far: " << free_so_far.size() << std::endl;
				ind_dummy.insert(new_dummy.begin(), new_dummy.end());
				}
			else {
//				ind_free.insert(free_so_far.begin(), free_so_far.end());
//				free_so_far.clear();
				classify_add_index(sit, free_so_far, ind_dummy);
				}
			++sit;
//			const Derivative *der=properties::get<Derivative>(it);
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
//		txtout << "classifying " << *it->name << std::endl;
		sibling_iterator sit=it.begin();
		index_map_t item_free;
		index_map_t item_dummy;
		while(sit!=it.end()) {
//			txtout << *sit->name << std::endl;
			if((sit->fl.parent_rel==str_node::p_sub || sit->fl.parent_rel==str_node::p_super) &&
				sit->fl.bracket==str_node::b_none /* && sit->is_integer()==false */) {
				if(*sit->name!="??") {
					const Coordinate *cdn=properties::get<Coordinate>(sit, true);
					const Symbol     *smb=Symbol::get(sit, true);
					// integer, coordinate or symbol indices always ok
					if(sit->is_integer() || cdn || smb) {
						item_free.insert(index_map_t::value_type(exptree(sit), iterator(sit)));
						}
					else {
						index_map_t::iterator fnd=item_free.find(exptree(sit));
						if(fnd!=item_free.end()) {
							if(item_dummy.find(exptree(sit))!=item_dummy.end())
								throw ConsistencyException("Triple index " + *sit->name + " inside a single factor found.");
							item_dummy.insert(*fnd);
							item_free.erase(fnd);
							item_dummy.insert(index_map_t::value_type(exptree(sit), iterator(sit)));
							}
						else {
							item_free.insert(index_map_t::value_type(exptree(sit), iterator(sit)));
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
//	txtout << "ind_free: " << ind_free.size() << std::endl;
//	txtout << "ind_dummy: " << ind_dummy.size() << std::endl;

	index_sw.stop();
	}

bool algorithm::contains(sibling_iterator from, sibling_iterator to, sibling_iterator arg)
	{
	while(from!=to) {
		if(from->name==arg->name) return true;
		++from;
		}
	return false;
	}

algorithm::range_vector_t::iterator algorithm::find_arg_superset(range_vector_t& ran, 
																		 sibling_iterator it)
	{
	sibling_iterator nxt=it;
	++nxt;
	return find_arg_superset(ran, it, nxt);
	}

void algorithm::find_argument_lists(range_vector_t& ran, bool only_comma_lists) const
	{
	sibling_iterator argit=args_begin();
	while(argit!=args_end()) {
		if(*argit->name=="\\comma") {
			ran.push_back(range_t(tr.begin(argit), tr.end(argit)));
			}
		else if(!only_comma_lists) {
			sibling_iterator argnxt=argit; ++argnxt;
			ran.push_back(range_t(argit, argnxt));
			}
		++argit;
		}	
	}

template<class Iter>
algorithm::range_vector_t::iterator algorithm::find_arg_superset(range_vector_t& ran, Iter st, Iter nd)
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

bool algorithm::is_termlike(iterator it)
	{
	if(tr.is_valid(tr.parent(it))) {
		if(*tr.parent(it)->name=="\\sum" || *tr.parent(it)->name=="\\expression" || tr.parent(it)->is_command() ) 
			return true;
		}
	return false;
	}

bool algorithm::is_factorlike(iterator it)
	{
	if(tr.is_valid(tr.parent(it))) {
		if(*tr.parent(it)->name=="\\prod")
			return true;
		}
	return false;
	}

bool algorithm::is_single_term(iterator it)
	{
	if(*it->name!="\\prod" && *it->name!="\\sum" && *it->name!="\\asymimplicit" && *it->name!="\\comma" 
		&& *it->name!="\\equals" && *it->name!="\\arrow") {
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

bool algorithm::is_nonprod_factor_in_prod(iterator it)
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

bool algorithm::prod_wrap_single_term(iterator& it)
	{
	if(is_single_term(it)) {
		force_prod_wrap(it);
		return true;
		}
	else return false;
	}

void algorithm::force_prod_wrap(iterator& it)
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

bool algorithm::prod_unwrap_single_term(iterator& it)
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

bool algorithm::separated_by_derivative(iterator i1, iterator i2, iterator check_dependence) const
	{
	iterator lca = tr.lowest_common_ancestor(i1, i2);

	// Walk up the tree from the first node until the LCA, flag any derivatives
	// with which we do not commute.

	struct {
		bool operator()(exptree& tr, iterator walk, iterator lca, iterator check_dependence) {
		   do {
				walk=exptree::parent(walk);
				if(walk == lca) break;
				const Derivative *der=properties::get<Derivative>(walk);
				if(der) {
					if(tr.is_valid(check_dependence) ) {
						const DependsBase *dep = properties::get_composite<DependsBase>(check_dependence);
						if(dep) {
							exptree deps=dep->dependencies(check_dependence);
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
											if(subtree_exact_equal(indit, depobjs))
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
	
	if(one_run(tr, i1, lca, check_dependence)) return true;
	if(one_run(tr, i2, lca, check_dependence)) return true;

	return false;
	}


bool algorithm::cleanup_anomalous_products(exptree& tr, exptree::iterator& it)
	{
	if(*(it->name)=="\\prod") {
		 if(tr.number_of_children(it)==0) {
			  it->name=name_set.insert("1").first;
			  return true;
			  }
		 else if(tr.number_of_children(it)==1) {
			  tr.begin(it)->fl.bracket=it->fl.bracket;
			  tr.begin(it)->multiplier=it->multiplier;
			  tr.flatten(it);
			  exptree::iterator tmp=tr.erase(it);
//			  txtout << "HERRE?" << std::endl;
			  pushup_multiplier(tmp);
			  it=tmp;
			  return true;
			  }
		 }
	return false;
	}

