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

#define NEW_XPERM 
//#define XPERM_DEBUG
// Do not turn this one off
#define XPERM_USE_EXT  

#include "Algebra.hh"
#include "Props.hh"
#include "YoungTab.hh"
#include "Exchange.hh"
#include "Numerical.hh"
#include "Cleanup.hh"

#include "properties/WeightInherit.hh"
#include "properties/Weight.hh"
#include "properties/Derivative.hh"
#include "properties/PartialDerivative.hh"


extern "C" {
#ifdef NEW_XPERM
  #include "xperm_new.h"
#else
  #include "xperm.h"
#endif
}
#include <utility>
#include <algorithm>
#include <set>
#include <sstream>
#include <typeinfo>

void algebra::register_properties()
	{
	properties::register_property(&create_property<Commuting>);
	properties::register_property(&create_property<AntiCommuting>);
	properties::register_property(&create_property<NonCommuting>);
	properties::register_property(&create_property<SelfCommuting>);
	properties::register_property(&create_property<SelfAntiCommuting>);
	properties::register_property(&create_property<SelfNonCommuting>);
	properties::register_property(&create_property<TableauSymmetry>);
	properties::register_property(&create_property<SatisfiesBianchi>);
	properties::register_property(&create_property<Symmetric>);
	properties::register_property(&create_property<AntiSymmetric>);
	properties::register_property(&create_property<Diagonal>);
	properties::register_property(&create_property<SelfDual>);
	properties::register_property(&create_property<AntiSelfDual>);
	properties::register_property(&create_property<DAntiSymmetric>);
	properties::register_property(&create_property<KroneckerDelta>);
	properties::register_property(&create_property<EpsilonTensor>);
	}	

void algebra::register_algorithms()
	{
	}

std::string SelfDual::name() const
	{
	return "SelfDual";
	}

std::string AntiSelfDual::name() const
	{
	return "AntiSelfDual";
	}

std::string Commuting::name() const
	{
	return "Commuting";
	}

std::string AntiCommuting::name() const
	{
	return "AntiCommuting";
	}

std::string NonCommuting::name() const
	{
	return "NonCommuting";
	}

std::string SelfCommuting::name() const
	{
	return "SelfCommuting";
	}

std::string SelfAntiCommuting::name() const
	{
	return "SelfAntiCommuting";
	}

std::string SelfNonCommuting::name() const
	{
	return "SelfNonCommuting";
	}

std::string Symmetric::name() const
	{
	return "Symmetric";
	}

std::string Diagonal::name() const
	{
	return "Diagonal";
	}

std::string DAntiSymmetric::name() const
	{
	return "DAntiSymmetric";
	}

std::string TableauSymmetry::name() const
	{
	return "TableauSymmetry";
	}

std::string SatisfiesBianchi::name() const
	{
	return "SatisfiesBianchi";
	}

std::string KroneckerDelta::name() const
	{
	return "KroneckerDelta";
	}

std::string EpsilonTensor::name() const
	{
	return "EpsilonTensor";
	}

bool EpsilonTensor::parse(exptree& tr, exptree::iterator pat, exptree::iterator prop, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("metric");
	if(kv!=keyvals.end()) metric=exptree(kv->second);

	kv=keyvals.find("delta");
	if(kv!=keyvals.end()) krdelta=exptree(kv->second);

	return true;
	}

bool TableauSymmetry::parse(exptree& tr, exptree::iterator pat, exptree::iterator prop, keyval_t& keyvals)
	{
   // Scan for the tableaux.
	keyval_t::const_iterator kvit=keyvals.begin();

	exptree::iterator indices=tr.end();
	exptree::iterator shape=tr.end();

	while(kvit!=keyvals.end()) {
		 // wrap value
		 if(kvit->first!="\\comma") 
			  tr.wrap(kvit->second, str_node("\\comma"));

		 if(kvit->first=="shape") 
			  shape=kvit->second;
		 if(kvit->first=="indices") 
			  indices=kvit->second;

		 if(shape!=tr.end() && indices!=tr.end()) {
			 // Make sure the shape and indices lists have a \comma node.
			 tr.list_wrap_single_element(shape);
			 tr.list_wrap_single_element(indices);
			 
			 exptree::sibling_iterator si=shape.begin();
			 exptree::sibling_iterator ii=indices.begin();
			 
			 tab_t tab;
			 
			 keyval_t::const_iterator tmp=kvit;
			 ++tmp;
			 if(tmp!=keyvals.end()) {
				 if(tmp->first=="selfdual")
					 tab.selfdual_column=1;
				 else if(tmp->first=="antiselfdual")
					 tab.selfdual_column=-1;
				 }
			 
			 int rowind=0;
			 unsigned int tabdown=to_long(*si->multiplier);
			 unsigned int numindices=tr.number_of_indices(pat);
			 // FIXME: we get the wrong pattern in case of a list! We should have
			 // been fed each individual item in the list, not the list itself.
//			  std::cout << numindices << " " << *pat->name << std::endl;
			 while(ii!=indices.end()) {
				 if(tabdown+1 > numindices) return false;
				 if(si==shape.end()) return false;
				 tab.add_box(rowind, to_long(*ii->multiplier));
				 ++ii;
				 if((--tabdown)==0 && ii!=indices.end()) {
					 ++si;
					 ++rowind;
					 tabdown=to_long(*si->multiplier);
					 }
				 }
			 tabs.push_back(tab);

			 tr.list_unwrap_single_element(shape);
			 tr.list_unwrap_single_element(indices);

			 shape=tr.end();
			 indices=tr.end();
			 }
		 ++kvit;
		}
	
	return true;
	}

bool Symmetric::parse(exptree& tr, exptree::iterator st, exptree::iterator it, keyval_t& kv)
	{
	return property::parse(tr,st,it,kv);
	}

bool SatisfiesBianchi::parse(exptree& tr, exptree::iterator st, exptree::iterator it, keyval_t& kv)
	{
	return property::parse(tr,st,it,kv);
	}

bool DAntiSymmetric::parse(exptree& tr, exptree::iterator st, exptree::iterator it, keyval_t& kv)
	{
	return property::parse(tr,st,it,kv);
	}

unsigned int Symmetric::size(exptree&, exptree::iterator) const
	{
	return 1;
	}

unsigned int DAntiSymmetric::size(exptree&, exptree::iterator) const
	{
	return 1;
	}

unsigned int KroneckerDelta::size(exptree&, exptree::iterator) const
	{
	return 1;
	}

unsigned int SatisfiesBianchi::size(exptree& tr, exptree::iterator it) const
	{
	exptree::sibling_iterator chld=tr.begin(it);
//	bool indexfirst=false;
	if(chld->fl.parent_rel!=str_node::p_none) {
//		indexfirst=true;
		++chld;
		}
	assert(chld->fl.parent_rel==str_node::p_none);
	const TableauBase *tb=properties::get<TableauBase>(chld);

	if(!tb) return 0;

	assert(tb->size(tr, chld)==1); // Does this make sense otherwise?

	return 1;
	}

TableauBase::tab_t SatisfiesBianchi::get_tab(exptree& tr, exptree::iterator it, unsigned int num) const
	{
	// Take the tableau of the child, increase all indices by 
	// one if the derivative index sits on the first position,
	// and then add a box on the first row corresponding to the
	// derivative.

	exptree::sibling_iterator chld=tr.begin(it);
	bool indexfirst=false;
	if(chld->fl.parent_rel!=str_node::p_none) {
		indexfirst=true;
		++chld;
		}
	assert(chld->fl.parent_rel==str_node::p_none);
//	txtout << *chld->name << std::endl;
	const TableauBase *tb=properties::get<TableauBase>(chld);
	assert(tb);
//	txtout << "got child TableauBase" << std::endl;

	assert(tb->size(tr, chld)==1);
	tab_t thetab=tb->get_tab(tr, chld, 0);
//	txtout << "got child tab" << std::endl;
	if(indexfirst) {
		for(unsigned int r=0; r<thetab.number_of_rows(); ++r)
			for(unsigned int c=0; c<thetab.row_size(r); ++c)
				thetab(r,c)+=1;
		thetab.add_box(0, 0);
		}
	else {
		exptree::index_iterator ii=tr.begin_index(it);
		unsigned int pos=0;
		while(ii!=tr.end_index(it)) {
			++ii;
			++pos;
			}
		thetab.add_box(0, pos-1);
		}

	return thetab;
	}


TableauBase::tab_t Symmetric::get_tab(exptree& tr, exptree::iterator it, unsigned int num) const
	{
	assert(num==0);

	const Symmetric *pd;
	for(;;) {
		pd=properties::get<Symmetric>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		} 

	tab_t tab;
	for(unsigned int i=0; i<tr.number_of_children(it); ++i)
		tab.add_box(0,i);
	return tab;
	}

TableauBase::tab_t SelfDual::get_tab(exptree& tr, exptree::iterator it, unsigned int num) const
	{
	tab_t ret=AntiSymmetric::get_tab(tr, it, num);
	ret.selfdual_column=1;
	return ret;
	}

TableauBase::tab_t AntiSelfDual::get_tab(exptree& tr, exptree::iterator it, unsigned int num) const
	{
	tab_t ret=AntiSymmetric::get_tab(tr, it, num);
	ret.selfdual_column=-1;
	return ret;
	}


TableauBase::tab_t DAntiSymmetric::get_tab(exptree& tr, exptree::iterator it, unsigned int num) const
	{
	assert(num==0);

	const DAntiSymmetric *pd;
	for(;;) {
		pd=properties::get<DAntiSymmetric>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		} 

	tab_t tab;
	tab.add_box(0,1);
	tab.add_box(0,0); // these were in the wrong order!!!
	for(unsigned int i=2; i<tr.number_of_children(it); ++i)
		tab.add_box(i-1,i);
	return tab;
	}

TableauBase::tab_t KroneckerDelta::get_tab(exptree& tr, exptree::iterator it, unsigned int num) const
	{
	assert(num==0);

	const KroneckerDelta *pd;
	for(;;) {
		pd=properties::get<KroneckerDelta>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		} 
	
	if(tr.number_of_children(it)%2!=0)
		 throw ConsistencyException("Encountered a KroneckerDelta object with an odd number of indices.");

//	bool onlytwo=false;
//	if(tr.number_of_children(it)==2)
//		onlytwo=true;

	tab_t tab;
	for(unsigned int i=0; i<tr.number_of_children(it); i+=2) {
		tab.add_box(i/2,i);
//		if(onlytwo)
		tab.add_box(i/2,i+1);
		}
	return tab;
	}


void TableauSymmetry::display(std::ostream& str) const
	{
	for(unsigned int i=0; i<tabs.size(); ++i)
		str << tabs[i] << std::endl;
	}

unsigned int TableauSymmetry::size(exptree&, exptree::iterator) const
	{
	return tabs.size();
	}

TableauBase::tab_t TableauSymmetry::get_tab(exptree&, exptree::iterator, unsigned int num) const
	{
	assert(num<tabs.size());
	return tabs[num];
	}

int TableauBase::get_indexgroup(exptree& tr, exptree::iterator it, int indexnum) const
	{
	const TableauBase *pd;
	for(;;) {
		pd=properties::get<TableauBase>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		} 
//	txtout << "now at " << *it->name << std::endl;

	unsigned int siz=size(tr, it);
	assert(siz==1); // FIXME: does not work yet for multi-tab symmetries
	tab_t tmptab=get_tab(tr, it, 0);
//	debugout << "searching indexgroup for " << *it->name <<  std::endl;
	if(tmptab.number_of_rows()==1) return 0;

	std::pair<int,int> loc=tmptab.find(indexnum);
//	debugout << "searching indexgroup " << loc.second << std::endl;
	assert(loc.first!=-1);
	return loc.second;
	}

bool TableauBase::is_simple_symmetry(exptree& tr, exptree::iterator it) const
	{
	const TableauBase *pd;
	for(;;) {
		pd=properties::get<TableauBase>(it);
		if(!pd)
			it=tr.begin(it);
		else break;
		} 

	for(unsigned int i=0; i<size(tr, it); ++i) {
		tab_t tmptab=get_tab(tr, it, i);
		if((tmptab.number_of_rows()==1 || tmptab.row_size(0)==1) && tmptab.selfdual_column==0)
			return true;
		}
	return false;
	}

template<class ForwardIterator>
bool is_sorted(ForwardIterator first, ForwardIterator last)
	{
   if (first != last) {
		ForwardIterator prev = first;
		for (++first; first != last; ++first) {
			if (*first < *prev)
				return false;
			prev = first;
			}
		} return true;
	}

prodrule::prodrule(exptree& tr, iterator it)
	: algorithm(tr, it), number_of_indices(0)
	{
	}

//
//  A(b*c*d)     -> A(b)*c*d + b*A(c)*d + b*c*A(d)
//  A(b*c)(e*f) 1-> A(b)(e*f)*c + b*A(c)(e*f)
//  A(b*c)(e*f) 2-> A(b*c)(e)*f + e*A(b*c)(f)

bool prodrule::can_apply(iterator it)
	{
	const Derivative *der=properties::get<Derivative>(it);
	if(der || *it->name=="\\cdb_Derivative") {
		prodnode=tr.end();
		number_of_indices=0;
		if(tr.number_of_children(it)>0) {
			sibling_iterator ch=tr.begin(it);
			while(ch!=tr.end(it)) {
				 if(prodnode==tr.end() && ( *ch->name=="\\prod" || *ch->name=="\\pow") )
					prodnode=ch; // prodnode contains the first product node, there may be more
				else {
					if(ch->is_index()) ++number_of_indices;
					}
				++ch;
				}
			if(prodnode!=tr.end()) return true;
			}
		}
	return false;
	}

algorithm::result_t prodrule::apply(iterator& it)
	{
	exptree rep; // the subtree storing the result
	iterator sm; // the sum node inside 'rep'

	// If we are distributing a multiple derivative, take out all indices except one,
	// and wrap things in a new derivative node which has these indices as child nodes.
	bool indices_at_front=true;
	if(number_of_indices>1) {
		rep.set_head(str_node(it->name));
		sm=rep.append_child(rep.begin(),str_node("\\sum"));
		sibling_iterator sib=tr.begin(it);
		while(sib->is_index()==false) {
			indices_at_front=false;
			++sib;  // find first index
			}
		++sib;
		while(sib!=tr.end(it)) { // move all other indices away FIXME: wrong order
			if(sib->is_index()) {
				sibling_iterator nxt=sib;
				++nxt;
				if(indices_at_front) rep.move_before(sm, (iterator)sib);
				else                 rep.move_after(sm, (iterator)sib);
				sib=nxt;
				}
			else ++sib;
			}
		}
	else {
		sm=rep.set_head(str_node("\\sum"));
		if(tr.begin(it)->is_index()==false)
			indices_at_front=false;
		}


	// Two cases to handle now: D(A**n) -> n D(A**(n-1)) and
	//                          D(A*B)  -> D(A)*B + A*D(B) 
	// both suitably generalised to anti-commuting derivatives.

	if(*prodnode->name=="\\pow") {
		 sibling_iterator ar=tr.begin(prodnode);
		 sibling_iterator pw=ar;
		 ++pw;
		 sm->name=name_set.insert("\\prod").first;
		 if(pw->is_integer()) 
			  multiply(sm->multiplier, *pw->multiplier);
		 else rep.append_child(sm, (iterator)pw);

		 // \partial(A**n)
		 iterator pref=rep.append_child(sm, iterator(prodnode));  // add A**n
		 iterator theD=rep.append_child(sm, it);                  // add \partial_{m}(A**n)
		 sibling_iterator repch=tr.begin(theD);                   // convert to \partial_{m}(A)
		 while(*repch->name!="\\pow") 
			  ++repch;
		 sibling_iterator pw2=tr.begin(repch);
		 rep.move_before(repch, pw2);
		 rep.erase(repch);

		 if(pw->is_integer()) {                                   // A**n -> A**(n-1)
			  if(*pw->multiplier==2) {
					iterator nn=rep.move_after(pref, iterator(tr.begin(pref)));
					multiply(nn->multiplier, *pref->multiplier);
					rep.erase(pref);
					}
			  else {
					pw2=tr.begin(pref);
					++pw2;
					add(pw2->multiplier, -1);
					}
			  }
		 else {
			  pw2=tr.begin(pref);
			  ++pw2;
			  if(*pw2->name=="\\sum") {
					iterator tmp=rep.append_child(pw2, str_node("1"));
					tmp->fl.bracket=rep.begin(pw2)->fl.bracket;
					multiply(tmp->multiplier, -1);
					}
			  else {
					iterator sumn=rep.wrap(pw2, str_node("\\sum"));	
					iterator tmp=rep.append_child(sumn, str_node("1"));
					multiply(tmp->multiplier, -1);
					}
			  }
		 }
	else {
		 // replace the '\diff' with a '\sum' of diffs.
		 unsigned int num=0;
		 sibling_iterator chl=tr.begin(prodnode); // pointer to current factor in the product
		 int sign=1; // keep track of a sign for anti-commuting derivatives

		 // In order to figure out whether a derivative is anti-commuting with
		 // a given object in the product on which it acts, we need to consider
		 // a number of cases:
		 //
		 //    D_{a}{\theta^{b}}                    with \theta^{a} Coordinate & SelfAntiCommuting
       //    D_{\theta^{a}}{\theta^{b}}           with \theta^{a} Coordinate and a,b,c AntiCommuting
		 //    D_{a}{D_{b}{G}}                      handled by making indices AntiCommuting.
		 //    D_{a}{D_{\dot{b}}{G}}                handled by making indices AntiCommuting.
		 //    D_{a}{T^{a b}}                       handled by making indices AntiCommuting.
		 //    D_{a}{\theta}                        with \theta having an ImplicitIndex of type 'a' 
		 //    D{ A B }                             not yet handled (problem is to give scalar anti-commutativity
       //                                            property to D, A, B).

		 while(chl!=tr.end(prodnode)) { // iterate over all factors in the product
			  // Add the whole product node to the replacement sum.
			  iterator dummy=rep.append_child(sm);
			  dummy=rep.replace(dummy, prodnode);
			  if(*tr.parent(it)->name=="\\expression") 
					dummy->fl.bracket=str_node::b_none;
			  else dummy->fl.bracket=str_node::b_round;

			  sibling_iterator wrap=rep.begin(dummy); 
			  // Go to the 'num'th factor in the product.
			  wrap+=num;    
			  // Replace this num'th factor with the original expression.
			  // We will then remove all that has to be removed.
			  iterator theD=rep.insert_subtree(wrap, it);  // iterator to the Derivative node
			  theD->fl.bracket=wrap->fl.bracket;
			  // Go to the 'prod' child of the \diff.
			  sibling_iterator repch=tr.begin(theD);
			  while(*repch->name!="\\prod")
					++repch;
			  // Replace this 'prod' child with 'just' the factor to be replaced, i.e.
			  // remove all the other factors which have been taken out of the derivative.
			  wrap->fl.bracket=prodnode->fl.bracket;
			  repch=tr.replace(repch,wrap);
			  // Erase the factor which we replaced with the \diff.
			  tr.erase(wrap);

			  // Handle signs for anti-commuting derivatives.
			  multiply(dummy->multiplier, sign);
			  // Update the sign. First create an exptree containing the derivative _without_
			  // the object on which it acts.
			  exptree emptyD(theD);
			  sibling_iterator theDargs=emptyD.begin(emptyD.begin());
			  while(theDargs!=emptyD.end(emptyD.begin())) {
				  if(theDargs->is_index()==false) 
					  theDargs=tr.erase(theDargs);
				  else ++theDargs;
				  }
			  
			  int stc=subtree_compare(emptyD.begin(), repch);
//			  txtout << "trying to move " << *emptyD.begin()->name << " through " << *repch->name 
//						<< " " << stc << std::endl;
			  int ret=exptree_ordering::can_swap(emptyD.begin(), repch, stc);
//			  txtout << ret << std::endl;
			  if(ret==0)
				  return l_no_action;
			  sign*=ret;

			  
			  // Avoid \partial_{a}{\partial_{b} ...} constructions in 
			  // case this child is a \partial-like too.
			  iterator repchi=repch;
			  cleanup_nests(tr, repchi);
			  
			  ++chl;
			  ++num;
			  }
		 }
//	tr.print_recursive_treeform(txtout, rep.begin());
	expression_modified=true;
	it=tr.replace(it,rep.begin()); 
//	multiply(it->multiplier, mult);
	cleanup_expression(tr, it);
	cleanup_nests(tr, it);
	return l_applied;
	}

distribute::distribute(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool distribute::can_apply(iterator st)
	{
	const Distributable *db=properties::get<Distributable>(st);
	if(!db) 
		return false;

	sibling_iterator facs=tr.begin(st);
	while(facs!=tr.end(st)) {
		if(*(*facs).name=="\\sum")
			return true;
//		if(*st->name=="\\indexbracket" || *st->name=="\\diff") break; // only first argument is object
		++facs;
		}
	return false;
	}

algorithm::result_t distribute::apply(iterator& prod)
	{
	exptree rep;
	rep.set_head(str_node("\\expression"));
	sibling_iterator top=rep.append_child(rep.begin(), str_node("\\sum", prod->fl.bracket, prod->fl.parent_rel));
	// add
	iterator ploc=rep.append_child(top, str_node(prod->name, prod->fl.bracket, prod->fl.parent_rel));
	// The multiplier should sit on each term, not on the sum.
	ploc->multiplier=prod->multiplier;
	
	// Examine each child node in turn. If it is a sum, distribute it
	// over all previously constructed nodes. Otherwise, add the child
	// node as a child to the previously constructed nodes.
	
	// "facs" iterates over all child nodes of the distributable (top-level) node
	sibling_iterator facs=tr.begin(prod);
	while(facs!=tr.end(prod)) {
		if(*(*facs).name=="\\sum") {
			sibling_iterator se=rep.begin(top);
			// "se" iterates over all nodes in the replacement \sum
			while(se!=rep.end(top)) {
				if(interrupted) 
					throw InterruptionException();

				exptree termrep;
				termrep.set_head(str_node());
				sibling_iterator sumch=tr.begin(facs);
				while(sumch!=tr.end(facs)) {
					if(interrupted) 
						throw InterruptionException();

					// add product "se" to termrep.
					sibling_iterator dup=termrep.append_child(termrep.begin(), str_node()); // dummy
					dup=termrep.replace(dup, se);
					// add term from sum as factor to product above.
					sibling_iterator newfact=termrep.append_child(dup, sumch);
					// put the multiplier up front
					multiply(dup->multiplier,*newfact->multiplier); 
					multiply(dup->multiplier,*facs->multiplier);
					one(newfact->multiplier);
					// make this child inherit the bracket from the sum node
					newfact->fl.bracket=facs->fl.bracket;
//					newfact->fl.bracket=str_node::b_none;  
					++sumch;
					}
				sibling_iterator nxt=se;
				++nxt;
				sibling_iterator sep1=se; ++sep1;
				se=rep.move_ontop(se, (sibling_iterator)(termrep.begin()));
				rep.flatten(se);
				rep.erase(se);
//				rep.replace(se, sep1, termrep.begin(termrep.begin()), termrep.end(termrep.begin()));
				se=nxt;
				}
			}
		else {
			sibling_iterator se=rep.begin(top);
			while(se!=rep.end(top)) {
				if(interrupted) 
					throw InterruptionException();
				rep.append_child(se, facs);
				++se;
				}
			}
		++facs;
		}
	if(rep.number_of_children(top)==1) { // nothing happened, no sum was present
//		prod->fl.mark=0; // handled
		return l_applied;
		}
	expression_modified=true;

// FIXME: why does this faster move lead to a crash in linear.cdb?
	iterator ret=tr.move_ontop(prod, (iterator)top);
//	assert(rep.begin()==rep.end());

//	iterator ret=tr.replace(prod, top);
//	txtout << "calling cleanup on " << *ret->name << " " << *tr.begin(ret)->name << std::endl;

	prodflatten pf(tr, tr.end());
	pf.make_consistent_only=true;
	pf.apply_recursive(ret, false);
	prodcollectnum pc(tr, tr.end());
	pc.apply_recursive(ret,false);
//	cleanup_sums_products(tr, ret);
//	txtout << "..." << *ret->name << std::endl;
	cleanup_nests_below(tr, ret, false); // CHANGED  true to false in last argument
	cleanup_nests(tr, ret, false); // CHANGED true to false in last argument

	// FIXME: if we had a flattened sum, does the apply_recursive now
	// go and examine every sum that we have created? Should we better
	// return an iterator to the last element in the sum?
	prod=ret;
	return l_applied;
	}

remove_indexbracket::remove_indexbracket(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool remove_indexbracket::can_apply(iterator it) 
	{
	if(*it->name=="\\indexbracket") {
		sibling_iterator sib=tr.begin(it);
		if(*sib->name!="\\sum" && *sib->name!="\\prod")
			return true;
		}
	return false;
	}

algorithm::result_t remove_indexbracket::apply(iterator& it)
	{
	multiply(tr.begin(it)->multiplier, *it->multiplier); 
	
	// move indices from indexbracket to the object it wraps
	sibling_iterator from=tr.begin(it);
	++from;
	sibling_iterator to=tr.end(it);
	tr.reparent(tr.begin(it), from, to);

	// remove \indexbracket itself
	tr.flatten(it);
	it=tr.erase(it);
	expression_modified=true;
	return l_applied;
	}

prodflatten::prodflatten(exptree& tr, iterator it)
	: algorithm(tr, it), make_consistent_only(false), is_diff(false)
	{
	}

bool prodflatten::can_apply(iterator it)
	{
	is_diff=false;
	if(*it->name!="\\prod") 
		 return false;

// FIXME: acting on PartialDerivative has been disabled because
// it really does not make sense anymore.
//		 if(properties::get<PartialDerivative>(it)==0)
//			  return false;
//		 else is_diff=true;

	if(!is_diff && tr.number_of_children(it)==1) return true;
	sibling_iterator facs=tr.begin(it);
	while(facs!=tr.end(it)) {
		const PartialDerivative *pd=properties::get<PartialDerivative>(facs);
		if((is_diff && pd) || (!is_diff && *facs->name=="\\prod"))
			return true;
		if(is_diff) break;
		++facs;
		}
	return false;
	}

algorithm::result_t prodflatten::apply(iterator& it)
	{
//	debugout << "acting with prodflatten at " << *it->name << std::endl;
	if(!is_diff && tr.number_of_children(it)==1) {
		tr.begin(it)->fl.bracket = it->fl.bracket;
		multiply(tr.begin(it)->multiplier, *it->multiplier);
		tr.flatten(it);
		it=tr.erase(it);
		pushup_multiplier(it);
		expression_modified=true;
		return l_applied;
		}
	sibling_iterator facs=tr.begin(it);
	str_node::bracket_t btype=facs->fl.bracket;
	while(facs!=tr.end(it)) {
		const PartialDerivative *pd=properties::get<PartialDerivative>(facs);
		if((is_diff && pd) || (!is_diff && *facs->name=="\\prod")) {
			str_node::bracket_t cbtype=tr.begin(facs)->fl.bracket;
			if(!make_consistent_only || cbtype==str_node::b_none || cbtype==str_node::b_no) {
				sibling_iterator prodch=tr.begin(facs);
				while(prodch!=tr.end(facs)) {
					prodch->fl.bracket=btype;
					++prodch;
					}
				sibling_iterator tmp=facs;
				++tmp;
				tr.flatten(facs);
				multiply(it->multiplier,*facs->multiplier);
				tr.erase(facs);
				pushup_multiplier(it);
				expression_modified=true;
				facs=tmp;
				}
			else ++facs;
			}
		else ++facs;
		if(is_diff) break;
		}
	return l_applied;
	}

sumflatten::sumflatten(exptree& tr, iterator it)
	: algorithm(tr, it), make_consistent_only(false)
	{
	}

bool sumflatten::can_apply(iterator it)
	{
	if(*it->name!="\\sum") return false;
	if(tr.number_of_children(it)==1 || tr.number_of_children(it)==0) return true;
	sibling_iterator facs=tr.begin(it);
	while(facs!=tr.end(it)) {
		if(*(*facs).name=="\\sum")
			return true;
		++facs;
		}
	return false;
	}

algorithm::result_t sumflatten::apply(iterator &it)
	{
	assert(*it->name=="\\sum");
	
	long num=tr.number_of_children(it);
	if(num==1) {
		multiply(tr.begin(it)->multiplier, *it->multiplier);
		tr.flatten(it);
		it=tr.erase(it);
		expression_modified=true;
		}
	else if(num==0) {
		node_zero(it);
		expression_modified=true;
		}
	else {
		sibling_iterator facs=tr.begin(it);
		str_node::bracket_t btype_par=facs->fl.bracket;
		while(facs!=tr.end(it)) {
			if(facs->fl.bracket!=str_node::b_none)
				btype_par=facs->fl.bracket;
			++facs;
			}
		facs=tr.begin(it);
		while(facs!=tr.end(it)) {
			if(*facs->name=="\\sum") {
				sibling_iterator terms=tr.begin(facs);
				str_node::bracket_t btype=terms->fl.bracket;
				if(!make_consistent_only || btype==str_node::b_none || btype==str_node::b_no) {
					expression_modified=true;
					sibling_iterator tmp=facs;
					++tmp;
					while(terms!=tr.end(facs)) {
						multiply(terms->multiplier,*facs->multiplier);
						terms->fl.bracket=btype_par;
//						if(terms->fl.bracket==str_node::b_none)
//							terms->fl.bracket=facs->fl.bracket;
						++terms;
						}
					tr.flatten(facs);
					tr.erase(facs);
					facs=tmp;
					}
				else ++facs;
				}
			else ++facs;
			}
		}
	return l_applied;
	}

listflatten::listflatten(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool listflatten::can_apply(iterator it)
	{
	if(*it->name!="\\comma") return false;
	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(*sib->name=="\\comma") return true;
		++sib;
		}
	return false;
	}

algorithm::result_t listflatten::apply(iterator& it)
	{
	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(*sib->name=="\\comma") {
			sibling_iterator sib2=sib;
			++sib2;
			tr.flatten(sib);
			tr.erase(sib);
			sib=sib2;
			expression_modified=true;
			}
		else ++sib;
		}
	
	return l_applied;
	}

index_object_cleanup::index_object_cleanup(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool index_object_cleanup::can_apply(iterator it)
	{
	if(*it->name=="") {
		if(tr.number_of_children(it)==1) {
			sibling_iterator sib=tr.begin(it);
			if(sib->fl.parent_rel==str_node::p_sub || sib->fl.parent_rel==str_node::p_super)
				return true;
			}
		}
	return false;
	}

algorithm::result_t index_object_cleanup::apply(iterator& it)
	{
	assert(*it->name=="");

	tr.flatten(it);
	it=tr.erase(it);
	expression_modified=true;

	return l_applied;
	}





sumsort::sumsort(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool sumsort::can_apply(iterator st) 
	{
	if(*st->name=="\\sum") return true;
	else return false;
	}

algorithm::result_t sumsort::apply(iterator& st) 
	{
	// This bubble sort is of course a disaster, but it'll have to do for now.

	sibling_iterator one, two;
	unsigned int num=tr.number_of_children(st);
	for(unsigned int i=1; i<num; ++i) {
		one=tr.begin(st);
		two=one; ++two;
		for(unsigned int j=i+1; j<=num; ++j) { // this loops too many times, no?
			int es=subtree_compare(one, two, -2, true, 0, true);
			if(should_swap(one, es)) {
				tr.swap(one);
				std::swap(one,two);  // put the iterators back in order
				expression_modified=true;
				}
			++one;
			++two;
			}
		}

	if(expression_modified) return l_applied;
	else return l_no_action;
	}

bool sumsort::should_swap(iterator obj, int subtree_comparison) const
	{
	sibling_iterator one=obj, two=obj;
	++two;

	// Find a SortOrder property which contains both one and two.
	int num1, num2;
	const SortOrder *so1=properties::get_composite<SortOrder>(one,num1);
	const SortOrder *so2=properties::get_composite<SortOrder>(two,num2);

	if(so1==0 || so2==0) { // No sort order known
		if(subtree_comparison<0) return true;
		return false;
		}
	else if(abs(subtree_comparison)<=1) { // Identical up to index names
		if(subtree_comparison==-1) return true;
		return false;
		}
	else {
		if(so1==so2) {
			if(num1>num2) return true;
			return false;
			}
		}

	return false;
	}


prodsort::prodsort(exptree& tr, iterator it)
	: algorithm(tr, it), ignore_numbers_(false)
	{
//	if(has_argument("IgnoreNumbers")) {
//		txtout << "ignoring numbers" << std::endl;
//		ignore_numbers_=true;
//		}
	}

bool prodsort::can_apply(iterator st) 
	{
	if(*st->name=="\\prod" || *st->name=="\\dot") return true;
	else return false;
	}



algorithm::result_t prodsort::apply(iterator& st) 
	{
	// This could have been done using STL's sort, but then you have to worry
	// about using stable_sort, and then the tree.sort() doesn't do that,
	// and anyhow you would perhaps want exceptions. Let's just use a bubble
	// sort since how many times do we have more than 100 items in a product?

	sibling_iterator one, two;
	unsigned int num=tr.number_of_children(st);
	for(unsigned int i=1; i<num; ++i) {
		one=tr.begin(st);
		two=one; ++two;
		for(unsigned int j=i+1; j<=num; ++j) { // this loops too many times, no?
			int es=subtree_compare(one, two, -2);
//			std::cerr << "hi " << es << std::endl;
			if(exptree_ordering::should_swap(one, es)) {
				int canswap=exptree_ordering::can_swap(one, two, es);
				if(canswap!=0) {
//					std::cout << "swapping " << *one->name << " with " << *two->name << std::endl;
					tr.swap(one);
					std::swap(one,two);  // put the iterators back in order
					if(canswap==-1)
						flip_sign(st->multiplier);
					expression_modified=true;
					}
				}
			++one;
			++two;
			}
		}

	if(expression_modified) return l_applied;
	else return l_no_action;
	}

reduce_div::reduce_div(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool reduce_div::can_apply(iterator st)
	{
	if(*st->name!="\\frac") return false;
//	sibling_iterator it=tr.begin(st);
//	++it; // first argument is allowed to be non-numerical
//	while(it!=tr.end(st)) {
//		if(*it->name!="1")
//			return false;
//		++it;
//		}
	return true;
	}

algorithm::result_t reduce_div::apply(iterator& st)
	{
	// Catch \frac{} nodes with one argument; those are supposed to be read as 1/(...).
	if(tr.number_of_children(st)==1) {
		tr.insert(tr.begin(st), str_node("1"));
		}

	assert(tr.number_of_children(st)>1);
	sibling_iterator it=tr.begin(st);
	multiplier_t rat;

	bool allnumerical=true;
	rat=*(it->multiplier);
	if(it->is_rational()==false) 
		allnumerical=false;

	one(it->multiplier);
	++it;
	while(it!=tr.end(st)) {
		if(*it->multiplier==0) {
			// CHECK: do these zeroes get handled correctly elsewhere?
			return l_applied;
			}
		rat/=*it->multiplier;
		one(it->multiplier);
		if(it->is_rational()==false) allnumerical=false;
		++it;
		}
	if(allnumerical) { // can remove the \frac altogether
		tr.erase_children(st);
		st->name=name_set.insert("1").first;
		}
	else { // just remove the all-numerical child nodes
		it=tr.begin(st);
		++it;
		while(it!=tr.end(st)) {
			if(it->is_rational()) 
				it=tr.erase(it);
			else ++it;
			}
		if(tr.number_of_children(st)==1) {
			tr.begin(st)->fl.bracket=st->fl.bracket;
			tr.begin(st)->fl.parent_rel=st->fl.parent_rel;
			multiply(tr.begin(st)->multiplier, *st->multiplier);
			tr.flatten(st);
			st=tr.erase(st);
			}
		}
	expression_modified=true;
	multiply(st->multiplier, rat);
	pushup_multiplier(st);
	return l_applied;
	}

keep_terms::keep_terms(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool keep_terms::can_apply(iterator it)
	{
	if(*it->name!="\\sum") return false;
	if(number_of_args()!=1 && number_of_args()!=2) return false;
	return true;
	}

algorithm::result_t keep_terms::apply(iterator& it)
	{
	sibling_iterator argit=args_begin();
	unsigned long firstnode=to_long(*argit->multiplier);
	long lastnode=-2;
	if(number_of_args()==2) {
		++argit;
		lastnode=to_long(*argit->multiplier);
		}
	sibling_iterator cut1=tr.begin(it);
	assert(firstnode<tr.number_of_children(it));

	assert(firstnode>=0);
	while(firstnode>0) {
		expression_modified=true;
		cut1=tr.erase(cut1);
		--firstnode;
		--lastnode;
		}
	++lastnode;
	if(lastnode>0) {
		while(lastnode>0) {
			if(cut1==tr.end()) 
				break;
			++cut1;
			--lastnode;
			}
		while(cut1!=tr.end(it)) {
			expression_modified=true;
			cut1=tr.erase(cut1);
			}
		}
	
	cleanup_sums_products(tr,it);
	return l_applied;
	}


reduce_sub::reduce_sub(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool reduce_sub::can_apply(iterator st)
	{
	if(*st->name!="\\sub") return false;
	return true;
	}

algorithm::result_t reduce_sub::apply(iterator& it)
	{
	assert(tr.number_of_children(it)>1); // To guarantee that we have really cleaned up that old stuff.

	it->name=name_set.insert("\\sum").first;
	exptree::sibling_iterator sit=tr.begin(it);

	// Make sure that all terms have the right sign, and zeroes are removed.
	if(*sit->multiplier==0) sit=tr.erase(sit);
	else                    ++sit;

	while(sit!=tr.end(it)) {
		if(*sit->multiplier==0)
			sit=tr.erase(sit);
		else {
			flip_sign(sit->multiplier);
			++sit;
			}
		}

	// Single-term situation: remove the \sum.
	assert(tr.number_of_children(it)>0);
	if(tr.number_of_children(it)==1) {
		sit=tr.begin(it);
		sit->fl.parent_rel=it->fl.parent_rel;
		sit->fl.bracket=it->fl.bracket;
		tr.flatten(it);
		it=tr.erase(it);
		}

	expression_modified=true;
	return l_applied;
	}

subseq::subseq(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool subseq::can_apply(iterator it)
	{
	if(*it->name=="\\eqn") return true;
	else                  return false;
	}

algorithm::result_t subseq::apply(iterator& it)
	{
	assert(*it->name=="\\eqn");
	assert(tr.number_of_children(it)==1);

	// FIXME: handle labels
	int eqno=atoi((*tr.begin(it)->name).c_str());
	iterator theeq=tr.equation_by_number(eqno);
	if(theeq==tr.end()) 
		return l_error;
	theeq=tr.active_expression(theeq);
	theeq=tr.begin(theeq);

	it=tr.replace(it, theeq);
	expression_modified=true;

	return l_applied;
	}


drop_keep_weight::drop_keep_weight(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}


// This algorithm acts on nodes which have Weight or inherit Weight.
// It only acts when the parent does _not_ have or inherit weight.
// This makes sure that we act on sums and products which are not
// themselves terms or factors in a sum or product.

bool drop_keep_weight::can_apply(iterator st)
	{
	if(number_of_args()!=2) return false;
	sibling_iterator argit=args_begin();
	label=*argit->name;
	++argit;
	weight=*argit->multiplier;


	const WeightInherit *gmnpar=0;
	const Weight        *wghpar=0;

	gmn=properties::get_composite<WeightInherit>(st, label);
	wgh=properties::get_composite<Weight>(st, label);
	gmnpar=properties::get_composite<WeightInherit>(tr.parent(st), label);
	wghpar=properties::get_composite<Weight>(tr.parent(st), label);

//	txtout << *st->name << ": " << gmn << ", " << wgh << ", " << gmnpar << " " << std::endl;
	if(gmn!=0 || wgh!=0) {
		bool ret = (gmnpar==0 && wghpar==0);
		return ret;
		}

	return false;
	}

algorithm::result_t drop_keep_weight::do_apply(iterator& it, bool keepthem)
	{
	if(gmn) {
		if(gmn->combination_type==WeightInherit::multiplicative) {
			if((keepthem==true && weight!=gmn->value(it, label)) || (keepthem==false && weight==gmn->value(it, label))) {
				expression_modified=true;
				zero(it->multiplier);
				}
			}
		else {
			sibling_iterator sib=tr.begin(it);
			while(sib!=tr.end(it)) {
				const WeightBase *gnb=properties::get_composite<WeightBase>(sib, label);
				if(gnb) {
					multiplier_t val;
					bool no_val=false;
					try {
						val=gnb->value(sib, label);
//						txtout << *sib->name << " has weight " << val << std::endl;
						}
					catch(WeightInherit::WeightException& we) {
//						txtout << *sib->name << " has undeterminable weight " << std::endl;
						// If we cannot determine the weight of this term because this is a sum of
						// terms with different weights: keep when in @drop, drop when in @keep.
						no_val=true;
						}
					if( (no_val==false && ( (keepthem==true && weight!=val) || (keepthem==false && weight==val) ) ) 
						 || (no_val==true && keepthem==true) ) {
						expression_modified=true;
						sib=tr.erase(sib);
						}
					else ++sib;
					}
				else {
					if( (keepthem==true && weight!=0) || (keepthem==false && weight==0) ) {
						expression_modified=true;
						sib=tr.erase(sib);
						}
					else ++sib;
					}
				}
			if(tr.number_of_children(it)==0)
				zero(it->multiplier);
			else if(tr.number_of_children(it)==1) {
				tr.flatten(it);
				it=tr.erase(it);
				}
			}
		}
	else {
		assert(wgh);
		if((keepthem==true && weight!=wgh->value(it, label)) || (keepthem==false && weight==wgh->value(it, label))) {
			expression_modified=true;
			zero(it->multiplier);
			}
		}
	
	cleanup_expression(tr, it);

	return l_applied;
	}



drop_weight::drop_weight(exptree& tr, iterator it)
	: drop_keep_weight(tr, it)
	{
	}

algorithm::result_t drop_weight::apply(iterator& it)
	{
	return drop_keep_weight::do_apply(it, false);
	}


keep_weight::keep_weight(exptree& tr, iterator it)
	: drop_keep_weight(tr, it)
	{
	}

algorithm::result_t keep_weight::apply(iterator& it)
	{
	return drop_keep_weight::do_apply(it, true);
	}




drop::drop(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool drop::can_apply(iterator st)
	{
	sibling_iterator arg=args_begin();
	if(arg==args_end())        return false;
	if(number_of_args()==2) {
		if(*st->name!="\\prod") return false;
		}
	else {
		if(!(st->name==arg->name)) return false;
		}
	return true;
	}

algorithm::result_t drop::apply(iterator& st)
	{
	if(number_of_args()==2) {
		sibling_iterator arg=args_begin();
		sibling_iterator sib=tr.begin(st);
		unsigned int count=0;
		while(sib!=tr.end(st)) {
			if(sib->name==arg->name) 
				++count;
			++sib;
			}
		++arg;
		if(count==*arg->multiplier) {
			zero(st->multiplier);
			expression_modified=true;
			}
		}
	else {
		zero(st->multiplier);
		expression_modified=true;
		}
	return l_applied;
	}



collect_factors::collect_factors(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool collect_factors::can_apply(iterator it)
	{
	if(*it->name=="\\prod") return true;
	return false;
	}

// The hash map is such that all objects which are equal have to sit in the same
// bin, but objects in the same bin do not necessarily all have to be equal.
void collect_factors::fill_hash_map(iterator it)
	{
	factor_hash.clear();
	sibling_iterator sib=tr.begin(it);
	unsigned int factors=0;
	while(sib!=tr.end(it)) {
		sibling_iterator chsib=tr.begin(sib);
		bool dontcollect=false;
		while(chsib!=tr.end(sib)) {
			 const Symbol     *smb=properties::get<Symbol>(chsib);
			 if((chsib->fl.parent_rel==str_node::p_sub || chsib->fl.parent_rel==str_node::p_super) &&
				 chsib->is_rational()==false && smb==0) {
				dontcollect=true;
				break;
				}
			++chsib;
			}
		if(!dontcollect) {
			if(*sib->name=="\\pow") 
				factor_hash.insert(std::pair<hashval_t, sibling_iterator>(tr.calc_hash(tr.begin(sib)), tr.begin(sib)));
			else
				factor_hash.insert(std::pair<hashval_t, sibling_iterator>(tr.calc_hash(sib), sib));
			++factors;
			}
		++sib;
		}
	}

algorithm::result_t collect_factors::apply(iterator& st)
	{
	assert(tr.is_valid(st));
	assert(*st->name=="\\prod");
	result_t res=l_no_action;

	fill_hash_map(st);
	factor_hash_iterator_t ht=factor_hash.begin();
	while(ht!=factor_hash.end()) {
		hashval_t curr=ht->first;  // hash value of the current set of terms
		factor_hash_iterator_t thisbin1=ht, thisbin2;
		while(thisbin1!=factor_hash.end() && thisbin1->first==curr) {
			thisbin2=thisbin1;
			++thisbin2;
			exptree expsum;
			iterator expsumit=expsum.set_head(str_node("\\sum"));
			// add the exponent of the first element in this hash bin
			if(*(tr.parent((*thisbin1).second)->name)=="\\pow") {
				sibling_iterator powch=tr.parent((*thisbin1).second).begin();
				++powch;
				iterator newch= expsum.append_child(expsumit, iterator(powch));
				newch->fl.bracket=str_node::b_round;
				}
			else {
				expsum.append_child(expsumit, str_node("1", str_node::b_round));
				}
			assert(*((*thisbin1).second->multiplier)==1);
			// find the other, identical factors
			while(thisbin2!=factor_hash.end() && thisbin2->first==curr) {
				if(subtree_exact_equal((*thisbin1).second, (*thisbin2).second)) {
					// only do something if this factor can be moved to the other one
					iterator objnode1=(*thisbin1).second;
					iterator objnode2=(*thisbin2).second;
					if(*tr.parent(objnode1)->name=="\\pow") objnode1=tr.parent(objnode1);
					if(*tr.parent(objnode2)->name=="\\pow") objnode2=tr.parent(objnode2);
					if(exptree_ordering::can_move_adjacent(st, objnode1, objnode2)) {
						// all clear
						assert(*((*thisbin2).second->multiplier)==1);
						res=l_applied;
						if(*(tr.parent((*thisbin2).second)->name)=="\\pow") {
							sibling_iterator powch=tr.parent((*thisbin2).second).begin();
							++powch;
							iterator newch=expsum.append_child(expsumit, iterator(powch));
							newch->fl.bracket=str_node::b_round;
							}
						else {
							expsum.append_child(expsumit, str_node("1", str_node::b_round));
							}
						factor_hash_iterator_t tmp=thisbin2;
						++tmp;
						if(*(tr.parent((*thisbin2).second)->name)=="\\pow")
							tr.erase(tr.parent((*thisbin2).second));
						else
							tr.erase((*thisbin2).second);
						factor_hash.erase(thisbin2);
						thisbin2=tmp;
						expression_modified=true;
						}
					else ++thisbin2;
					}
				else ++thisbin2;
				}
			// make the modification to the tree
			if(expsum.number_of_children(expsum.begin())>1) {
				cleanup_expression(expsum);
				cleanup_nests_below(expsum, expsum.begin());
				if(! (expsum.begin()->is_identity()) ) {
					collect_terms ct(expsum, expsum.end());
					iterator tp=expsum.begin();
					ct.apply(tp);

					iterator inserthere=thisbin1->second;
					if(*(tr.parent(inserthere)->name)=="\\pow")
						inserthere=tr.parent(inserthere);
					if(expsum.begin()->is_rational() && (expsum.begin()->is_identity() ||
																	 expsum.begin()->is_zero() ) ) {
						if(*(inserthere->name)=="\\pow") {
							tr.flatten(inserthere);
							inserthere=tr.erase(inserthere);
							sibling_iterator nxt=inserthere;
							++nxt;
							tr.erase(nxt);
							}
						if(expsum.begin()->is_zero()) {
							rset_t::iterator rem=inserthere->multiplier;
							node_one(inserthere);
							inserthere->multiplier=rem;
							}
						}
					else {
						exptree repl;
						repl.set_head(str_node("\\pow"));
						repl.append_child(repl.begin(), iterator((*thisbin1).second));
						repl.append_child(repl.begin(), expsum.begin());
						if(*(inserthere->name)!="\\pow") {
							inserthere=(*thisbin1).second;
							}
						tr.insert_subtree(inserthere, repl.begin());
						tr.erase(inserthere);
						}
					}
				}
//			else txtout << "only one left" << std::endl;
			++thisbin1;
			}
		ht=thisbin1;
		}
	cleanup_sums_products(tr, st);
	return res;
	}



factor_out::factor_out(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

/// Check if the expression is a sum with more than one term
bool factor_out::can_apply(iterator st)
	{
	if(*st->name=="\\sum") {
		sibling_iterator ar=args_begin();
		if(ar==args_end()) return false;

		to_factor_out.clear();
		for(unsigned int i=0; i<tr.arg_size(ar); ++i) 
			 to_factor_out.push_back(exptree(tr.arg(ar,i)));
		return true;
		}
	else return false;
	}

/* Tries to factor out the given factors from the expression.
 
   The algorithm goes through each term in the sum and extracts all factors
   from it. These are a stored in a list of factors along with the term they came from.
   Each different factor is then reinserted back into the expression but multiplied by
   the sum of all the terms that factor came from.
 
   Note that factors are extracted on both the left and right to allow for 
   non-commutative factoring, and A, B, and A * B are treated as seperate factors.
*/

algorithm::result_t factor_out::apply(iterator& it)
	{
	// For each term with factors, collector is used to store the factors
	// and the rest of the term they came from.
	typedef std::multimap<exptree, sibling_iterator, exptree_is_less> collector_t;
	collector_t collector;

	sibling_iterator st=tr.begin(it);
	while(st!=tr.end(it)) { // Loop through each term in the sum
		exptree left_factors, right_factors; // Where we will store extracted factors
		left_factors.set_head(str_node("\\prod"));
		right_factors.set_head(str_node("\\prod"));
		
		if(*st->name=="\\prod") {
			extract_factors(st, true, left_factors);
			extract_factors(st, false, right_factors);
			
			// The factors are not extracted in any particular order.
			// We want to put them in some canonical order so that equal factors are grouped.
			order_factors(st, left_factors);
			order_factors(st, right_factors);
			
			// If removing factors reduced the term to something simple, modify the
			// tree to show this.
			if(tr.number_of_children(st)==0) {
				rset_t::iterator mtmp=st->multiplier;
				node_one(st);
				st->multiplier=mtmp;
				}
			else if(tr.number_of_children(st)==1) {
				multiply(tr.begin(st)->multiplier, *st->multiplier);
				tr.flatten(st);
				st=tr.erase(st);
				}
			}
		else {
			// If the term is a factor we are looking for then we pull the factor
			// out and replace it by a 1.
			for(unsigned int tfo=0; tfo < to_factor_out.size(); ++tfo) {
				exptree_comparator comparator;
				if(comparator.equal_subtree(static_cast<iterator>(st),
				                            to_factor_out[tfo].begin())==exptree_comparator::subtree_match) {
					iterator tmp=left_factors.append_child(left_factors.begin(), static_cast<iterator>(st));
					one(tmp->multiplier);
					rset_t::iterator mtmp=st->multiplier;
					node_one(st);
					st->multiplier=mtmp;
					break;
					}
				}
			}
		
		// left_factors and right_factors are now products of the factors 
		// that were found in the current term.
		if (left_factors.number_of_children(left_factors.begin()) > 0 ||
		    right_factors.number_of_children(right_factors.begin()) > 0) {
			exptree sum;
			// Encode as left_factors + right_factors so that we can still use the
			// existing method with an exptree as the key. This is very hacky, and
			// might cause problems if A + B is regarded as the same tree as B + A.
			sum.set_head(str_node("\\sum"));
			sum.append_child(sum.begin(), left_factors.begin());
			sum.append_child(sum.begin(), right_factors.begin());
			collector.insert(std::make_pair(sum, st));
			}
		
		++st;
		}

	if(collector.size()==0) return l_no_action;
	
	// The expression is currently in a weird state - we have pulled out all of the factors from each
	// term that they appeared, but not added them back in anywhere. Since we now have a multimap with
	// keys corresponding to each type of factor (A, B, AB, etc) and members corresponding to the rest
	// of the term that they came from, we can go through and collect together terms that have the same
	// factor and reinsert them into the tree.

	collector_t::iterator ci=collector.begin();
	exptree oldkey = (*ci).first;
	while(ci!=collector.end()) {
		exptree term;
		term.set_head(str_node("\\prod")); // Create the * in (factor * (a + b + c)).
		const exptree thiskey=(*ci).first;
		
		// Extract left_factors and right_factors from how they were stored
		// as left_factors + right_factors
		sibling_iterator sum_iter = thiskey.begin(thiskey.begin());
		iterator left_factors = sum_iter;
		iterator right_factors = ++sum_iter;
		
		// Insert left factors on the left.
		if (thiskey.number_of_children(left_factors) > 0)
			term.reparent(term.begin(), left_factors);
		
		sibling_iterator sumit=term.append_child(term.begin(),str_node("\\sum"));
		size_t terms=0;
		exptree_is_equivalent cmp;
		// Go through each term with the same factor and include it in the sum.
		// We also remove it from the tree since it will be added back in in its
		// factorised form.
		while(ci!=collector.end() && cmp((*ci).first, oldkey)) {
			term.append_child(sumit, (*ci).second);
			tr.erase((*ci).second);
			++terms;
			++ci;
			}
		
		// Insert right factors on the right.
		if (thiskey.number_of_children(right_factors) > 0)
			term.reparent(term.begin(), right_factors);
		
		if(terms>1)
			expression_modified=true;
		
		if(terms==1) { // a sum with only one child
			term.flatten(sumit);
			term.erase(sumit);
			}
		
		// Put our factor * (a + b + ...) piece back into the tree.
		tr.insert_subtree(tr.begin(it), term.begin());
		
		if(ci==collector.end())
			break;
		oldkey=(*ci).first;
		}

	if(tr.number_of_children(it)==1) { // the sum has been reduced to a single term now
		tr.flatten(it);
		it=tr.erase(it);
		}
	
	if(expression_modified) return l_applied;
	else                    return l_no_action;
	}

/* Extracts all possible factors from a product node.
 
   extract_factors takes a product node in the main expression tree and
   extracts any factors which are present, respecting anti and non-commutivity.
   The factors are removed from the product node and placed into another
   provided product node. The factors will either be extracted to the left if
   left_to_right is true or from the right if not.
  
   @param product a sibling_iterator pointing to the node of the expression 
   tree to extract the factors from. This should be a product node.
   @param left_to_right If true factors are extracted to the left,
   if false they are extracted to the right.
   @param collector an exptree with a product node as the head. Extracted
   factors are inserted into this as children.
*/
void factor_out::extract_factors(sibling_iterator product, bool left_to_right, exptree& collector) 
	{
	if (tr.number_of_children(product) == 0)
		return;
	
	sibling_iterator begin, end;
	if (left_to_right) {
		// Set begin the to the left most child, end to the right most.
		begin = tr.begin(product);
		end = begin; end += tr.number_of_siblings(begin);
		}
	else {
		// Set begin the to the right most child, end to the left most.
		end = tr.begin(product);
		begin = end; begin += tr.number_of_siblings(end);
		}
	
	// factor_signs stores the sign that would need to be picked up by each factor if it
	// was to be moved through the terms we have encountered so far.
	// A value of 0 means the factor can't be moved through the terms.
	std::vector<int> factor_signs (to_factor_out.size(), 1);
	
	bool beginning_preserved = true;
	bool first_element = true;
	sibling_iterator current_term = begin;
	do { // Loop through from begin to end, including both begin and end.
		if (!first_element) {
			if (left_to_right)
				current_term++;
			else
				current_term--;
			}
		first_element = false;
		
		bool factor_removed = false;
		
		// Check to see if the current term is one of the factors we are looking for.
		for(unsigned int tfo=0; tfo<to_factor_out.size(); ++tfo) {
			exptree_comparator comparator;
			if(comparator.equal_subtree(static_cast<iterator>(current_term),
			                            to_factor_out[tfo].begin()) == exptree_comparator::subtree_match) {
				if (factor_signs[tfo] != 0) {
					// Put the term into the collector and remove it from the tree, picking up
					// a sign if necessary.
					if (left_to_right)
						collector.append_child(collector.begin(), static_cast<iterator>(current_term));
					else
						collector.prepend_child(collector.begin(), static_cast<iterator>(current_term));
					multiply(product->multiplier, factor_signs[tfo]);
					tr.erase(current_term);
					factor_removed = true;
					if (!beginning_preserved)
						// We've gone past a term which we didn't factor out
						expression_modified = true;
					}
				break;
				}
			}
		
		if (!factor_removed) {
			// Strictly speaking the beginning may still be preserved, so we don't
			// set expression_modified to true until we pull out another factor, in which
			// case it definitely isn't.
			beginning_preserved = false;
			
			// If the term wasn't moved to the front as a factor then we have to consider the
			// commutivity properties of it with other factors that we may later move through it.
			for(unsigned int tfo=0; tfo < to_factor_out.size(); ++tfo) {
				int stc = subtree_compare(to_factor_out[tfo].begin(), current_term);
				int sign = exptree_ordering::can_swap(to_factor_out[tfo].begin(), current_term, stc);
				factor_signs[tfo] *= sign;
				}
			}
		
		// The above algorithm has each term compared to each factor twice. Once to check if the
		// the term is a factor, and if not, once to check its commutitivity properties with the factors.
		// It would be nice to only have one loop, but I think we need to know we're not going to move
		// the term before we start working out the commutitivity properties.
		} while(current_term != end);
	}

/* Takes an product of factors and tries to put its term
   in the same order the factors were provided in.
 
   order_factors takes a product node of factors and orders the terms in it as closely as
   possible to the order of factors given by the user. Anti and non-commutivity
   are respected, and the sign of the term the factors came from will be updated
   as necessary.
  
   @param product a sibling_iterator corresponding to the product node that
   the factors came from. This may have its sign changed.
   @param factors an exptree with a product node as head, and the collected
   factors as children. This is what is put in order.
   @param first_unordered_term a sibling_iterator pointing to the term in
   factors from which the ordering should start. This is mostly used 
   internally by the function and omitting this argument will order from the 
   beginning.
*/
void factor_out::order_factors(sibling_iterator product, exptree& factors, sibling_iterator first_unordered_term) 
	{
	bool passed_non_commuting_term = false;
	sibling_iterator first_non_commuting_term;
	
	for (unsigned int tfo=0; tfo < to_factor_out.size(); ++tfo) {
		// Try to pull out each factor in turn from the remaining unordered part of the
		// expression.
		int sign = 1;
		for(sibling_iterator psi=first_unordered_term; psi != factors.end(factors.begin()); ++psi) {
			exptree_comparator comparator;
			if(comparator.equal_subtree(static_cast<iterator>(psi),
			                            to_factor_out[tfo].begin()) == exptree_comparator::subtree_match) {
				if (sign != 0) {
					if (psi == first_unordered_term) {
						++first_unordered_term; // This is safe if nodes are linked rather than indexed.
						}
					else {
						factors.move_before(first_unordered_term, psi);
						multiply(product->multiplier, sign);
						}
					}
				}
			else {
				int stc = subtree_compare(to_factor_out[tfo].begin(), psi);
				sign *= exptree_ordering::can_swap(to_factor_out[tfo].begin(), psi, stc);
				if (sign == 0) {
					if (!passed_non_commuting_term) {
						first_non_commuting_term = psi;
						}
					else {
						if (factors.index(psi) < factors.index(first_non_commuting_term))
							first_non_commuting_term = psi;
						}
					passed_non_commuting_term = true;
					break; // We're not going to be able to extract this factor.
					}
				}
			}
		}
	
	// A non commuting term will stop us bringing anything that doesn't
	// commute with it to the front. Thus we need to order all the terms 
	// after it again.
	if (passed_non_commuting_term) {
		sibling_iterator next_term = ++first_non_commuting_term;
		if (next_term != factors.end(factors.begin()))
			order_factors(product, factors, next_term);
		}
	}

void factor_out::order_factors(sibling_iterator product, exptree& factors) 
	{
	sibling_iterator first_unordered_term = factors.begin(factors.begin());
	order_factors(product, factors, first_unordered_term);
	}

factor_in::factor_in(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool factor_in::can_apply(iterator st)
	{
	factnodes.clear();
	assert(tr.is_valid(st));
	if(*st->name=="\\sum") {
		sibling_iterator ar=args_begin();
		if(ar==args_end()) return false;

		for(unsigned int i=0; i<tr.arg_size(ar); ++i) 
			 factnodes.insert(exptree(tr.arg(ar,i)));
		return true;
		}
	else return false;
	}

hashval_t factor_in::calc_restricted_hash(iterator it) const
	{
	if(*it->name!="\\prod") return tr.calc_hash(it);

	sibling_iterator sib=tr.begin(it);
	hashval_t ret=1;
	bool first=true;
	while(sib!=tr.end(it)) { // see storage.cc for the original calc_hash
		 if(factnodes.count(exptree(sib))==0) {
			if(first) { 
				first=false;
				ret=tr.calc_hash(sib);
				}
			else { 
				ret*=17;
				ret+=tr.calc_hash(sib);
				}
			}
		++sib;
		}
	return ret;
	}

void factor_in::fill_hash_map(iterator it)
	{
	term_hash.clear();
	sibling_iterator sib=tr.begin(it);
	unsigned int terms=0;
	while(sib!=tr.end(it)) {
		term_hash.insert(std::pair<hashval_t, sibling_iterator>(calc_restricted_hash(sib), sib));
		++terms;
		++sib;
		}
	}

bool factor_in::compare_prod_nonprod(iterator prod, iterator nonprod) const
	{
	assert(*(prod->name)=="\\prod");
	assert(*(nonprod->name)!="\\prod");
	sibling_iterator it=tr.begin(prod);
	bool found=false;
	while(it!=tr.end(prod)) {
		 if(factnodes.count(exptree(it))==0) {
			 if(nonprod->name==it->name) { // FIXME: subtree_equal
				if(found) return false; // already found
				else found=true;
				}
			else return false;
			}
		++it;
		}
	if(found || (!found && factnodes.count(nonprod)!=0)) return true;
	return false;
	}

bool factor_in::compare_restricted(iterator one, iterator two) const
	{
	if(one->name==two->name) {
		if(*one->name=="\\prod") {
			sibling_iterator it1=tr.begin(one), it2=tr.begin(two);
			while(it1!=tr.end(one) && it2!=tr.end(two)) {
				 if(factnodes.count(exptree(it1))!=0) {
					++it1;
					continue;
					}
				 if(factnodes.count(exptree(it2))!=0) {
					++it2;
					continue;
					}
				iterator nxt=it1; nxt.skip_children(); ++nxt;
				if(!tr.equal(tr.begin(it1), sibling_iterator(nxt), tr.begin(it2))) 
					return false;
				++it1; ++it2;
				}
			}
		}
	else {
		if(*one->name=="\\prod" && *two->name!="\\prod") 
			return compare_prod_nonprod(one,two);
		else if(*one->name!="\\prod" && *two->name=="\\prod") 
			return compare_prod_nonprod(two,one);
		}
	return true;
	}

algorithm::result_t factor_in::apply(iterator& it)
	{
	algorithm::result_t ret=l_no_action;
	fill_hash_map(it);

	term_hash_iterator_t ht=term_hash.begin();
	while(ht!=term_hash.end()) { // loop over hash bins
		hashval_t curr=ht->first;
		term_hash_iterator_t thisbin1=ht, thisbin2=ht;
		++thisbin2;
		if(thisbin2==term_hash.end() || thisbin2->first!=thisbin1->first) { // only one term in this bin
			++ht;
			continue;
			}

		// extract the prefactor of every term in this bin.
		std::map<iterator, exptree, exptree::iterator_base_less> prefactors;
		while(thisbin1!=term_hash.end() && thisbin1->first==curr) {
			exptree prefac;
//			txtout << "doing one" << std::endl;
			prefac.set_head(str_node("\\sum"));
			if(*(thisbin1->second->name)=="\\prod") { // search for all to-be-factored-out factors 
				iterator prefacprod=prefac.append_child(prefac.begin(), str_node("\\prod", str_node::b_round));
				sibling_iterator ps=tr.begin(thisbin1->second);
				while(ps!=tr.end(thisbin1->second)) {
					 if(factnodes.count(exptree(ps))!=0) {
						iterator theterm=prefac.append_child(prefacprod, (iterator)(ps));
						theterm->fl.bracket=str_node::b_round;
						}
					++ps;
					}
				prefacprod->multiplier=thisbin1->second->multiplier;
				switch(prefac.number_of_children(prefacprod)) {
					case 0:
						prefacprod->name=name_set.insert("1").first;
						break;
					case 1:
						multiply(prefac.begin(prefacprod)->multiplier, *(prefacprod->multiplier));
						prefac.flatten(prefacprod);
						prefacprod=prefac.erase(prefacprod);
						break;
					}
				}
			else { // just insert the constant
				str_node pf("1", str_node::b_round);
				pf.multiplier=thisbin1->second->multiplier;
				prefac.append_child(prefac.begin(), pf);
				}
			prefactors[thisbin1->second]=prefac;
			++thisbin1;
			}

		// add up prefactors for terms which differ only by the prefactor
		thisbin1=ht;
		while(thisbin1!=term_hash.end() && thisbin1->first==curr) {
			thisbin2=thisbin1;
			++thisbin2;
			while(thisbin2!=term_hash.end() && thisbin2->first==curr) {
				if(compare_restricted(thisbin1->second, thisbin2->second)) {
					ret=l_applied;
//					txtout << "found match" << std::endl;
					assert(prefactors.count(thisbin1->second)>0);
					assert(prefactors.count(thisbin2->second)>0);
					iterator sumhead1=prefactors[thisbin1->second].begin();
					iterator sumhead2=prefactors[thisbin2->second].begin();
					tr.reparent(sumhead1,tr.begin(sumhead2),tr.end(sumhead2));
//					txtout << "reparented" << std::endl;
					zero((*thisbin2).second->multiplier);
					prefactors.erase(thisbin2->second);
					term_hash_iterator_t tmp=thisbin2;
					++tmp;
					term_hash.erase(thisbin2);
					thisbin2=tmp;
					expression_modified=true;
					}
				else ++thisbin2;
				}
			++thisbin1;
			}
		// remove old prefactors and add prefactor sums
		std::map<iterator, exptree, exptree::iterator_base_less>::iterator prefit=prefactors.begin(); 
		while(prefit!=prefactors.end()) {
			if(tr.number_of_children(prefit->second.begin())>1) { // only do this if there really is more than just one term
				sibling_iterator facit=tr.begin(prefit->first);
				while(facit!=tr.end(prefit->first)) {
					 if(factnodes.count(exptree(facit))>0)
						facit=tr.erase(facit);
					else
						++facit;
					}
				iterator inserthere=prefit->first.begin();
				if(*(prefit->first->name)!="\\prod") {
					iterator prodnode=tr.insert(prefit->first, str_node("\\prod"));
					one(prefit->first->multiplier);
					tr.append_child(prodnode, prefit->first); // FIXME: we need a 'move' 
					tr.erase(prefit->first);
					inserthere=tr.begin(prodnode);
					}
				else one(prefit->first->multiplier);
				tr.insert_subtree(inserthere, (*prefit).second.begin());
				}
			++prefit;
			}

		
		ht=thisbin1;
		}

	// Remove all terms which have zero multiplier.
	sibling_iterator one=tr.begin(it);
	while(one!=tr.end(it)) {
		if(*one->multiplier==0) 
			one=tr.erase(one);
		else if(*one->name=="\\sum" && *one->multiplier!=1) {
			sibling_iterator oneit=tr.begin(one);
			while(oneit!=tr.end(one)) {
				multiply(oneit->multiplier, *one->multiplier);
				++oneit;
				}
			one->multiplier=rat_set.insert(1).first;
			++one;
			}
		else ++one;
		}
	
	// If there is only one term left, flatten the tree.
	if(tr.number_of_children(it)==1) {
		tr.begin(it)->fl.bracket=it->fl.bracket;
		tr.begin(it)->fl.parent_rel=it->fl.parent_rel;
		tr.flatten(it);
		it=tr.erase(it);
		}
	else if(tr.number_of_children(it)==0) {
		it->multiplier=rat_set.insert(0).first;
		}


	return ret;
	}



collect_terms::collect_terms(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

//bool collect_terms::check_index_consistency(iterator it) 
//	{
//	index_map_t ind_free, ind_dummy;
//	classify_indices(it, ind_free, ind_dummy);
//	return true;
//	}

bool collect_terms::can_apply(iterator st)
	{
	assert(tr.is_valid(st));
	if(*st->name=="\\sum") return true;
	return false;
	}

void collect_terms::fill_hash_map(iterator it)
	{
	fill_hash_map(tr.begin(it), tr.end(it));
	}

void collect_terms::fill_hash_map(sibling_iterator sib, sibling_iterator end)
	{
	term_hash.clear();
	while(sib!=end) {
		term_hash.insert(std::pair<hashval_t, sibling_iterator>(tr.calc_hash(sib), sib));
		++sib;
		}
	}

void collect_terms::remove_zeroed_terms(sibling_iterator from, sibling_iterator to)
	{
	// Remove all terms which have zero multiplier.
	sibling_iterator one=from;
	while(one!=to) {
		if(*one->multiplier==0) 
			one=tr.erase(one);
		else if(*one->name=="\\sum" && *one->multiplier!=1) {
			sibling_iterator oneit=tr.begin(one);
			while(oneit!=tr.end(one)) {
				multiply(oneit->multiplier, *one->multiplier);
				++oneit;
				}
			one->multiplier=rat_set.insert(1).first;
			++one;
			}
		else ++one;
		}
	}

//algorithm::result_t collect_terms::apply(sibling_iterator& from, sibling_iterator& to)
//	{
//	assert(*tr.parent(from)->name=="\\sum");
//	fill_hash_map(from, to);
//	result_t res=collect_from_hash_map();
//	remove_zeroed_terms(from, to);
//	return res;
//	}

algorithm::result_t collect_terms::apply(iterator& st)
	{
	assert(tr.is_valid(st));
	assert(*st->name=="\\sum");
	fill_hash_map(st);
	result_t res=collect_from_hash_map();
	remove_zeroed_terms(tr.begin(st), tr.end(st));

	// If there is only one term left, flatten the tree.
	if(tr.number_of_children(st)==1) {
		tr.begin(st)->fl.bracket=st->fl.bracket;
		tr.begin(st)->fl.parent_rel=st->fl.parent_rel;
		tr.flatten(st);
		st=tr.erase(st);
		// We may have to propagate the multiplier up the tree to make it consistent.
		pushup_multiplier(st);
		}
	else if(tr.number_of_children(st)==0) {
//		zero(st->multiplier);
		node_zero(st);
		}
	return res;
	}

algorithm::result_t collect_terms::collect_from_hash_map()
	{
	result_t res=l_no_action;
	term_hash_iterator_t ht=term_hash.begin();
	while(ht!=term_hash.end()) {
		hashval_t curr=ht->first;  // hash value of the current set of terms
		term_hash_iterator_t thisbin1=ht, thisbin2;
		while(thisbin1!=term_hash.end() && thisbin1->first==curr) {
			thisbin2=thisbin1;
			++thisbin2;
			while(thisbin2!=term_hash.end() && thisbin2->first==curr) {
				 if(subtree_exact_equal((*thisbin1).second, (*thisbin2).second, -2, true, 0, true)) {
					res=l_applied;
					add((*thisbin1).second->multiplier, *((*thisbin2).second->multiplier));
					zero((*thisbin2).second->multiplier);
					term_hash_iterator_t tmp=thisbin2;
					++tmp;
					term_hash.erase(thisbin2);
					thisbin2=tmp;
					expression_modified=true;
					}
				else ++thisbin2;
				}
			++thisbin1;
			}
		ht=thisbin1;
		}
		
	return res;
	}


sym_asym::sym_asym(exptree& tr, iterator it)
	: algorithm(tr, it), locate(tr, it)
	{
	if(number_of_args()<1 || !(*args_begin()->name=="\\comma")) {
		throw ArgumentException("@sym needs a comma-separated list of objects over which to symmetrise.");
		}
	}

bool sym_asym::can_apply(iterator it)
	{
	if(*it->name!="\\prod") 
		if(!is_single_term(it))
			return false;

	argloc_2_treeloc.clear();
	prod_wrap_single_term(it);
	bool located=locate_(tr.begin(it), tr.end(it), argloc_2_treeloc);
	prod_unwrap_single_term(it);
	return located;
	}

// bool sym_asym::can_apply(sibling_iterator st, sibling_iterator nd)
// 	{
// 	if(*(tr.parent(st)->name)!="\\prod") return false;
// 	argloc_2_treeloc.clear();
// 	if(locate_(st, nd, argloc_2_treeloc)) return true;
// 	return false;
// 	}

bool locate::compare_(const str_node& one, const str_node& two)
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

locate::locate(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

unsigned int locate::locate_single_object_(exptree::iterator obj_to_find, 
														 exptree::iterator st, exptree::iterator nd,
														 std::vector<unsigned int>& store)
	{
	unsigned int count=0;
	unsigned int index=0;
	while(st!=nd) {
		exptree::iterator it1=st; it1.skip_children(); ++it1;
		if(tr.equal(st, it1, obj_to_find, locate::compare_)) {
			++count;
			store.push_back(index);
			}
		++st;
		++index;
		}
	return count;
	}

bool locate::locate_object_set_(exptree::iterator set_parent,
										  exptree::iterator st, exptree::iterator nd,
										  std::vector<unsigned int>& store)
	{
	exptree::sibling_iterator fst=tr.begin(set_parent);
	exptree::sibling_iterator fnd=tr.end(set_parent);
	while(fst!=fnd) {
		exptree::iterator aim=fst;
		if((*aim->name)=="\\comma") {
			if(locate_object_set_(aim, st, nd, store)==false)
				return false;
			}
		else {
			if((*aim->name).size()==0 && tr.number_of_children(aim)==1)
				aim=tr.begin(aim);
			if(locate_single_object_(aim, st, nd, store)!=1)
				return false;
			}
		++fst;
		}
	return true;
	}

bool locate::locate_(exptree::sibling_iterator st, exptree::sibling_iterator nd,
							std::vector<unsigned int>& storage)
	{
	// Locate the objects in which to symmetrise. We use an integer
	// index (offset wrt. 'st') rather than an iterator because the
	// latter only apply to a single tree, not to its copies.

	exptree::iterator it=st;
	exptree::iterator end=nd;

	return locate_object_set_(args_begin(), it, end, storage);
	}

sym::sym(exptree& tr, iterator it)
	: algorithm(tr, it), locate(tr, it), sym_asym(tr, it)
	{
	}

algorithm::result_t sym::apply(iterator& it)
	{
	prod_wrap_single_term(it);
	sibling_iterator st=tr.begin(it);
	sibling_iterator nd=tr.end(it);
	result_t res=doit(st,nd,false);
	if(res==l_applied)
		it=tr.parent(st);
	return res;
	}

algorithm::result_t asym::apply(iterator& it)
	{
	prod_wrap_single_term(it);
	sibling_iterator st=tr.begin(it);
	sibling_iterator nd=tr.end(it);
	result_t res=doit(st,nd,true);
	if(res==l_applied)
		it=tr.parent(st);
	return res;
	}

//algorithm::result_t sym::apply(sibling_iterator& st, sibling_iterator& nd)
//	{
//	return doit(st, nd, false);
//	}

asym::asym(exptree& tr, iterator it)
	: algorithm(tr, it), locate(tr, it), sym_asym(tr, it)
	{
	}

//algorithm::result_t asym::apply(sibling_iterator& st, sibling_iterator& nd)
//	{
//	return doit(st, nd, true);
//	}


algorithm::result_t sym_asym::doit(sibling_iterator& st, sibling_iterator& nd, bool sign)
	{
	assert(*tr.parent(st)->name=="\\prod");
	// Setup combinations class. First construct original and block length.
	sibling_iterator fst=tr.begin(args_begin());
	sibling_iterator fnd=tr.end(args_end());
	raw_ints.clear();
	raw_ints.block_length=0;
	
//	debugout << "arglog " << argloc_2_treeloc.size() << std::endl;

	for(unsigned int i=0; i<argloc_2_treeloc.size(); ++i)
		raw_ints.original.push_back(i);
	while(fst!=fnd) {
		if(*(fst->name)=="\\comma") {
			if(raw_ints.block_length==0) raw_ints.block_length=tr.number_of_children(fst);
			else                         assert(raw_ints.block_length==tr.number_of_children(fst));
			}
		else if(fst->name->size()>0 || (fst->name->size()==0 && tr.number_of_children(fst)==1)) {
			if(raw_ints.block_length==0) raw_ints.block_length=1;
			else                         assert(raw_ints.block_length==1);
			}
		++fst;
		}	
	long start_=-1, end_=-1;
	sibling_iterator other_args=args_begin();
	++other_args;
	while(other_args!=args_end()) {
		if(*(other_args->name)=="\\setoption") {
			if(*tr.child(other_args,0)->name=="Start")
				start_=to_long(*tr.child(other_args,1)->multiplier);
			else if(*tr.child(other_args,0)->name=="End")
				end_=to_long(*tr.child(other_args,1)->multiplier);
			}
		++other_args;
		}
	
	raw_ints.set_unit_sublengths();
	// Sort within the blocks, if any
	if(raw_ints.block_length!=1) {
		std::vector<unsigned int>::iterator fr=argloc_2_treeloc.begin();
		std::vector<unsigned int>::iterator to=argloc_2_treeloc.begin();
		to+=raw_ints.block_length;
		for(unsigned int i=0; i<raw_ints.original.size()/raw_ints.block_length; ++i) {
			std::sort(fr, to);
			fr+=raw_ints.block_length;
			to+=raw_ints.block_length;
			}
		}
//	txtout << raw_ints.original.size() << " original size" << std::endl;
//	txtout << raw_ints.block_length << " block length" << std::endl;

	// Add output asym ranges.
	if(number_of_args()>1) {
		sibling_iterator ai=args_begin();
		++ai;
		while(ai!=args_end()) {
			if(*ai->name=="\\comma") {
				sibling_iterator cst=tr.begin(ai);
				combin::range_t asymrange;
				while(cst!=tr.end(ai)) {
					asymrange.push_back(to_long(*cst->multiplier));
					++cst;
					}
//					sibling_iterator aim=cst;
//					if((*aim->name).size()==0 && tr.number_of_children(aim)==1)
//						aim=tr.begin(aim);
//					for(unsigned int i1=0; i1<argloc_2_treeloc.size(); ++i1) {
//						iterator walk=st;
//						int num=argloc_2_treeloc[i1];
//						while(num--)
//							++walk;
//						if(walk->name==aim->name) {
//							asymrange.push_back(i1);
//							break;
//							}
//						}
//					++cst;
//					}
				raw_ints.input_asym.push_back(asymrange);
				}
			++ai;
			}
		}

	raw_ints.permute(start_, end_);
//	txtout << "generating " << raw_ints.size() << " terms" << std::endl;
//	for(unsigned int i=0; i<raw_ints.size(); ++i) {
//		for(unsigned int j=0; j<raw_ints[i].size(); ++j)
//			txtout << raw_ints[i][j] << " ";
//		txtout << std::endl;
//		}

	// Build replacement tree.
	exptree rep;
	sibling_iterator top=rep.set_head(str_node("\\sum"));
	sibling_iterator dummy=rep.append_child(top, str_node("dummy"));

	for(unsigned int i=0; i<raw_ints.size(); ++i) {
		exptree copytree(tr.parent(st));// CORRECT?
		copytree.begin()->fl.bracket=str_node::b_none;
		copytree.begin()->fl.parent_rel=str_node::p_none;
		
		std::map<iterator, iterator, exptree::iterator_base_less> replacement_map;
		
		for(unsigned int j=0; j<raw_ints[i].size(); ++j) {
			iterator repl=copytree.begin(), orig=tr.parent(st); // CORRECT?
			++repl; ++orig;
			for(unsigned int k=0; k<argloc_2_treeloc[raw_ints[i][j]]; ++k)
				++orig;
			for(unsigned int k=0; k<argloc_2_treeloc[raw_ints.original[j]]; ++k)
				++repl;

			// We cannot just replace here, because then walking along the tree
			// in the next step may no longer work (we may be swapping objects
			// with different numbers of indices, as in
			//
			//   A_{a b} B_{c};
			//   @sym!(%){A_{a b}, B_{c}};
			// 
			// so we store iterators first.

			if((*orig->name).size()==0)
				replacement_map[repl]=tr.begin(orig);
			else
				replacement_map[repl]=orig;
			}

		// All replacement rules now figured out, let's do them.
		std::map<iterator, iterator>::iterator rit=replacement_map.begin();
		while(rit!=replacement_map.end()) {
			str_node::bracket_t cbr=rit->first->fl.bracket;
			iterator repl=copytree.replace(rit->first, rit->second);
			// FIXME: think about whether this is what we want: the bracket
			// type 'stays', while the parent rel is moved together with the
			// index. A(x)*Z[y] -> A(y)*Z[x] ,
			//        A^m_n     -> A_n^m .
			repl->fl.bracket=cbr;
			++rit;
			}

		// Some final multiplier stuff and cleanup

		multiply(copytree.begin()->multiplier, 1/multiplier_t(raw_ints.total_permutations()));
//		multiply(copytree.begin()->multiplier, *st->multiplier);
		if(sign)
			multiply(copytree.begin()->multiplier, raw_ints.ordersign(i));

		iterator tmp=copytree.begin();
		prod_unwrap_single_term(tmp);
		rep.insert_subtree(dummy, copytree.begin());
		}
	expression_modified=true;
	dont_iterate=true;
	rep.erase(dummy);
	// show replacement tree
//	txtout << "replacement : " << std::endl;
//	eo.print_infix(rep.begin());
//	txtout << std::endl;

	iterator reploc=tr.replace(tr.parent(st), rep.begin());
	if(*(tr.parent(reploc)->name)=="\\sum") {
		tr.flatten(reploc);
		reploc=tr.erase(reploc);
		}
	st=tr.begin(reploc);
	nd=tr.end(reploc);
	return l_applied;
	}

order::order(exptree& tr, iterator it)
	: algorithm(tr, it), locate(tr, it)
	{
	}

bool order::can_apply(iterator st)
	{
	if(*(st->name)!="\\prod")
		return(is_single_term(st));
	return true;
	}

algorithm::result_t order::doit(iterator& st, bool sign)
	{
	prod_wrap_single_term(st);

	std::vector<unsigned int> locs;
	if(locate_(tr.begin(st), tr.end(st), locs)) {
		if(!(::is_sorted(locs.begin(), locs.end()))) {
			std::vector<unsigned int> ordered(locs);
			std::sort(ordered.begin(), ordered.end());
			if(sign) {
				int osign=combin::ordersign(ordered.begin(), ordered.end(), locs.begin(), locs.end());
				if(osign!=1) {
					multiply(st->multiplier, osign);
					}
				}
			
			iterator orig_st=tr.begin(args_begin());
			for(unsigned int i=0; i<ordered.size(); ++i) {
				iterator dest_st=tr.begin(st);
				for(unsigned int k=0; k<ordered[i]; ++k)
					++dest_st;
//				txtout << "replacing " << *dest_st->name << " with " << *orig_st->name << std::endl;
				if((*orig_st->name).size()==0)
					tr.replace(dest_st, tr.begin(orig_st));
				else
					tr.replace(dest_st, orig_st);
				expression_modified=true;
				orig_st.skip_children();
				++orig_st;
				}
			}
		}
	prod_unwrap_single_term(st);

	return l_applied;
	}

canonicalise::canonicalise(exptree& tr, iterator it)
	: algorithm(tr, it), reuse_generating_set(false) 
	{
	}

bool canonicalise::can_apply(iterator it) 
	{
	if(*(it->name)!="\\prod")
		return(is_single_term(it));
	
	sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(*sib->name=="\\sum" || *sib->name=="\\prod" )
			return false;
		++sib;
		}
	return true;
	}

bool canonicalise::remove_traceless_traces(iterator& it)
	{
	// Remove any traces of traceless tensors (this is best done early).
	sibling_iterator facit=tr.begin(it);
	while(facit!=tr.end(it)) {
		const Traceless *trl=properties::get_composite<Traceless>(facit);
		if(trl) {
			std::set<exptree, tree_exact_less_mod_prel_obj> countmap;
			exptree::index_iterator indit=tr.begin_index(facit);
			while(indit!=tr.end_index(facit)) {
				if(countmap.find(exptree(indit))==countmap.end()) {
					countmap.insert(exptree(indit));
					++indit;
					}
				else {
					zero(it->multiplier);
					expression_modified=true;
					return true;
					}
				}
			}
		++facit;
		}
	return false;
	}

bool canonicalise::remove_vanishing_numericals(iterator& it)
	{
	// Remove Diagonal objects with numerical indices which are not all the same.
	sibling_iterator facit=tr.begin(it);
	while(facit!=tr.end(it)) {
		const Diagonal *dgl=properties::get_composite<Diagonal>(facit);
		if(dgl) {
			exptree::index_iterator indit=tr.begin_index(facit), indit2;
			if(indit->is_rational()) {
				indit2=indit; ++indit2;
				while(indit2!=tr.end_index(facit)) {
					if(indit2->is_rational()==false) 
						break;
					if(indit2->multiplier!=indit->multiplier) {
						zero(it->multiplier);
						expression_modified=true;
						return true;
						}
					++indit2;
					}
				}
			}
		++facit;
		}	
	return false;
	}

Indices::position_t canonicalise::position_type(iterator it) const
	{
	const Indices *ind=properties::get<Indices>(it, true);
	if(ind) 
		return ind->position_type;
	return Indices::free;
	}

std::string canonicalise::get_index_set_name(iterator it) const
	{
	const Indices *ind=properties::get<Indices>(it, true);
	if(ind) {
		return ind->set_name;
// TODO: The logic was once as below, but it is no longer clear to
// me why that would ever make sense.
//		if(ind->parent_name!="") return ind->parent_name;
//		else                     return ind->set_name;
		}
	else return " undeclared";
	}

bool canonicalise::only_one_on_derivative(iterator i1, iterator i2) const
	{
	int num=0;
	iterator p1=tr.parent(i1);
	const Derivative *der1=properties::get<Derivative>(p1);
	if(der1) ++num;

	iterator p2=tr.parent(i2);
	const Derivative *der2=properties::get<Derivative>(p2);
	if(der2) ++num;

	if(num==1) return true;
	else return false;
	}

algorithm::result_t canonicalise::apply(iterator& it)
	{
#ifdef XPERM_DEBUG
	txtout << "canonicalising the following expression:" << std::endl;
	tr.print_recursive_treeform(txtout, it);
	txtout << is_single_term(it) << std::endl;
#endif

	stopwatch totalsw;
	totalsw.start();
	prod_wrap_single_term(it);
	
	if(remove_traceless_traces(it)) 
		return l_applied;

	if(remove_vanishing_numericals(it)) 
		return l_applied;


	// Now the real thing...
	index_map_t ind_free, ind_dummy;
	classify_indices(it, ind_free, ind_dummy);
	index_position_map_t ind_pos_free, ind_pos_dummy;
	fill_index_position_map(it, ind_free, ind_pos_free);
	fill_index_position_map(it, ind_dummy, ind_pos_dummy);
	const unsigned int total_number_of_indices=ind_free.size()+ind_dummy.size();
	
	// If there are no indices, there is nothing to do here...
	if(total_number_of_indices==0) {
		prod_unwrap_single_term(it);
		return l_no_action;
		}

	// Construct the "name to slot" map from the order in ind_free & ind_dummy.
	// Also construct the free and dummy lists.
	// And a map from index number to iterator (for later).
	std::vector<int> vec_perm;
	int              *free_indices=new int[ind_free.size()];
	

	// We need two arrays: one which maps from the order in which slots appear in 
	//	the tensor to the corresponding iterator (this is provided by the standard
	// index routines already), and one which maps from the order in which the indices 
	// appear in the base map to an exptree object (so that we can replace).

	std::vector<exptree::iterator> num_to_it_map(total_number_of_indices);
	std::vector<exptree>           num_to_tree_map;

	// Handle free indices.
	
	index_map_t::iterator sorted_it=ind_free.begin();
	int curr_index=0;
	while(sorted_it!=ind_free.end()) {
		index_position_map_t::iterator ii=ind_pos_free.find(sorted_it->second);
		num_to_it_map[ii->second]=ii->first;
		num_to_tree_map.push_back(exptree(ii->first));
		free_indices[curr_index++]=ii->second+1;
		vec_perm.push_back(ii->second+1);
		
		++sorted_it;
		}
	curr_index=0;


	// Handle dummy indices
	// In order to ensure that dummy indices from different index types do not
	// get mixed up, we need to collect information about the types of all
	// dummy indices.
	// The key in the maps below is the set_name of the Indices property, or the
	// set_name of the parent if applicable. If none, it will be ' undeclared'.

	typedef std::map<std::string, std::vector<int> > dummy_set_t;
	dummy_set_t dummy_sets;

	sorted_it=ind_dummy.begin();
	while(sorted_it!=ind_dummy.end()) {
		// We insert the dummy indices in pairs (canonicalise only acts on
		// expressions which have dummies coming in doublets, not the more
		// general cadabra dummy concept).
		// The lower index come first, and then the upper index. 

		index_position_map_t::const_iterator ii=ind_pos_dummy.find(sorted_it->second);
		index_map_t::const_iterator          next_it=sorted_it;
		++next_it;
		index_position_map_t::const_iterator i2=ind_pos_dummy.find(next_it->second);

#ifdef XPERM_DEBUG
		txtout << *(ii->first->name) << " at pos " << ii->second+1 << " " << ii->first->fl.parent_rel << std::endl;
#endif

		switch(ii->first->fl.parent_rel) {
			case str_node::p_super:
			case str_node::p_none:
//				vec_perm.push_back(ii->second+1);
//				vec_perm.push_back(i2->second+1);
//				num_to_tree_map.push_back(exptree(ii->first));
//				num_to_tree_map.push_back(exptree(i2->first));
				break;
			case str_node::p_sub:
				std::swap(ii, i2);
//				vec_perm.push_back(i2->second+1);
//				vec_perm.push_back(ii->second+1);
//				num_to_tree_map.push_back(exptree(i2->first));
//				num_to_tree_map.push_back(exptree(ii->first));
				break;
			}

		vec_perm.push_back(ii->second+1);
		vec_perm.push_back(i2->second+1);
		num_to_tree_map.push_back(exptree(ii->first));
		num_to_tree_map.push_back(exptree(i2->first));

		num_to_it_map[ii->second]=ii->first;
		num_to_it_map[i2->second]=i2->first;

		// If the indices are not in canonical order and they are separated 
		// by a Derivative, we cannot raise/lower, so they should stay in this order.
		// Have to do this by putting those indices in a different set and then
		// setting the metric flag to 0. Ditto when only one index is on a derivative
		// (canonicalising usually makes the expression uglier in that case).
		iterator tmp;
		if( ( (separated_by_derivative(ii->first, i2->first,tmp) 
				 || only_one_on_derivative(ii->first, i2->first) )
				&& position_type(ii->first)==Indices::fixed ) ||
			 position_type(ii->first)==Indices::independent ) {
			dummy_sets[" NR "+get_index_set_name(ii->first)].push_back(ii->second+1);
			dummy_sets[" NR "+get_index_set_name(i2->first)].push_back(i2->second+1);
			}
		else {
			if( properties::get<AntiCommuting>(ii->first, true) != 0 ) {
				dummy_sets[" AC "+get_index_set_name(ii->first)].push_back(ii->second+1);
				dummy_sets[" AC "+get_index_set_name(i2->first)].push_back(i2->second+1);
				}
			else {
				dummy_sets[get_index_set_name(ii->first)].push_back(ii->second+1);
				dummy_sets[get_index_set_name(i2->first)].push_back(i2->second+1);
				}
			}

		++sorted_it;
		++sorted_it;
		}

	// FIXME: handle 'repeated' sets (numerical indices)
	// FIXME: kludge to handle numerical indices; should be done through lookup
	// in Integer properties. This one does NOT work when there is more than
	// one index set; we would need more clever logic to figure out which
	// index type the numerical index corresponds to.
//	debugout << index_sets.size() << std::endl;
//	if(index_sets.size()==1) {
//		for(unsigned int kk=0; kk<indexpos_to_indextype.size(); ++kk) 
//			if(indexpos_to_indextype[kk]=="numerical")
//				indexpos_to_indextype[kk]=index_sets.begin()->first;
//		}
//
	
	// Construct the generating set.

	std::vector<unsigned int> base_here;

	if(!reuse_generating_set || generating_set.size()==0) {
		generating_set.clear();
		// Symmetry of individual tensors.
		sibling_iterator facit=tr.begin(it);
		int curr_pos=0;
		while(facit!=tr.end(it)) {
			const TableauBase *tba=properties::get_composite<TableauBase>(facit);
			if(tba) {
				unsigned int number_of_indices=tr.number_of_indices(facit);

				// Add indices to the base. We used to add everything except the last one, but that
				// seems to be the wrong thing to do after the XPERM -> XPERM_EXT upgrade (see Jose's email).
				for(unsigned int kk=0; kk<number_of_indices; ++kk) 
					base_here.push_back(curr_pos+kk+1);
				
				// loop over tabs
				for(unsigned int ti=0; ti<tba->size(tr, facit); ++ti) {
					TableauBase::tab_t tmptab=tba->get_tab(tr,facit,ti);
					if(tmptab.number_of_rows()>0) {
						for(unsigned int col=0; col<tmptab.row_size(0); ++col) { // anti-symmetry in all inds in a col
							if(tmptab.column_size(col)>1) {
								// all pairs NEW: SGS
								for(unsigned int indnum1=0; indnum1<tmptab.column_size(col)-1; ++indnum1) {
//								for(unsigned int indnum2=indnum1+1; indnum2<tmptab.column_size(col); ++indnum2) {
									std::vector<int> permute(total_number_of_indices+2);
									for(unsigned int kk=0; kk<permute.size(); ++kk)
										permute[kk]=kk+1;
									std::swap(permute[tmptab(indnum1,col)+curr_pos],
												 permute[tmptab(indnum1+1,col)+curr_pos]);
									std::swap(permute[total_number_of_indices+1],
												 permute[total_number_of_indices]); // anti-symmetry
									generating_set.push_back(permute);
									}
								}
							}
						}
					if(tmptab.number_of_rows()==1 && tmptab.row_size(0)>1) { // symmetry, if all cols of size 1
						// all pairs
						for(unsigned int indnum1=0; indnum1<tmptab.row_size(0)-1; ++indnum1) {
//						for(unsigned int indnum2=indnum1+1; indnum2<tmptab.row_size(0); ++indnum2) {
							std::vector<int> permute(total_number_of_indices+2);
							for(unsigned int kk=0; kk<permute.size(); ++kk)
								permute[kk]=kk+1;
							std::swap(permute[tmptab(0,indnum1)+curr_pos],
										 permute[tmptab(0,indnum1+1)+curr_pos]);
							generating_set.push_back(permute);
							}
						}
					else if(tmptab.number_of_rows()>0) { // find symmetry under equal-length column exchange
						unsigned int column_height=tmptab.column_size(0);
						unsigned int this_set_start=0;
						for(unsigned int col=1; col<=tmptab.row_size(0); ++col) {
							if(col==tmptab.row_size(0) || column_height!=tmptab.column_size(col)) {
								if(col-this_set_start>1) {
									// two or more equal-length columns found, make generating set
									for(unsigned int col1=this_set_start; col1+1<=col-1; ++col1) {
//									for(unsigned int col2=this_set_start+1; col2<col; ++col2) {
										std::vector<int> permute(total_number_of_indices+2);
										for(unsigned int kk=0; kk<permute.size(); ++kk)
											permute[kk]=kk+1;
										for(unsigned int row=0; row<column_height; ++row) {
//										txtout << row << " " << col1 << std::endl;
											std::swap(permute[tmptab(row,col1)+curr_pos],
														 permute[tmptab(row,col1+1)+curr_pos]);
											}
										generating_set.push_back(permute);
										}
									}
								this_set_start=col;
								if(col<tmptab.row_size(0)) 
									column_height=tmptab.column_size(col);
								}
							}
						}
					}
//					txtout << "loop over tabs done" << std::endl;
				curr_pos+=number_of_indices;
				}
			else {
				unsigned int number_of_indices=tr.number_of_indices(facit);
				if(number_of_indices==1)
					base_here.push_back(curr_pos+1);
				else {
					for(unsigned int kk=0; kk<number_of_indices; ++kk) 
						base_here.push_back(curr_pos+kk+1);
					}
				curr_pos+=tr.number_of_indices(facit); // even if tba=0, this factor may contain indices
				}
			++facit;
			}
		// Symmetry under tensor exchange.
		if(exchange::get_node_gs(tr, it, generating_set)==false) {
			zero(it->multiplier);
			expression_modified=true;
			}
		}
	// End of construction of generating set.

#ifdef XPERM_DEBUG
	txtout << generating_set.size() << " " << *it->multiplier << std::endl;
#endif
	if(*it->multiplier!=0) {
		// Fill data for the xperm routines.
		int *gs=0;

		if(generating_set.size()>0) {
			gs=new int[generating_set.size()*generating_set[0].size()];
			for(unsigned int i=0; i<generating_set.size(); ++i) {
				for(unsigned int j=0; j<total_number_of_indices+2; ++j) {
					gs[i*(total_number_of_indices+2)+j]=generating_set[i][j];
#ifdef XPERM_DEBUG
					txtout << gs[i*(total_number_of_indices+2)+j] << " ";
#endif				
					}
#ifdef XPERM_DEBUG
				txtout << std::endl;
#endif
				}
			}
		
		// Setup the arrays for xperm from our own data structures.

		int    *base=new int[base_here.size()];
		int    *perm=new int[total_number_of_indices+2];
		int   *cperm=new int[total_number_of_indices+2];

		for(unsigned int i=0; i<base_here.size(); ++i)
			base[i]=base_here[i];
		assert(vec_perm.size()==total_number_of_indices);
		for(unsigned int i=0; i<total_number_of_indices; ++i) 
			perm[i]=vec_perm[i];
		perm[total_number_of_indices]=total_number_of_indices+1;
		perm[total_number_of_indices+1]=total_number_of_indices+2;

		int  *lengths_of_dummy_sets=new int[dummy_sets.size()];
		int  *dummies              =new int[ind_dummy.size()];
		int  *metric_signatures    =new int[dummy_sets.size()];
		int  dsi=0; 
		int  cdi=0;
		dummy_set_t::iterator ds=dummy_sets.begin();
		while(ds!=dummy_sets.end()) {
			lengths_of_dummy_sets[dsi]=ds->second.size();
			for(unsigned int k=0; k<ds->second.size(); ++k) 
				dummies[cdi++]=(ds->second)[k];
			if(ds->first.substr(0,4)==" NR ")
				metric_signatures[dsi]=0;
			else {
				if(ds->first.substr(0,4)==" AC ")
					metric_signatures[dsi]=-1;
				else
					metric_signatures[dsi]=1;
				}
			++ds;
			++dsi;
			}

		int  *lengths_of_repeated_sets=new int[1];
		int          *repeated_indices=new int[1];

		lengths_of_repeated_sets[0]=0;
		
#ifdef XPERM_DEBUG
			txtout << "perm:" << std::endl;
			for(unsigned int i=0; i<total_number_of_indices+2; ++i)
			txtout << perm[i] << " "; 
			txtout << std::endl;
			txtout << "base:" << std::endl;
			for(unsigned int i=0; i<base_here.size(); ++i)
			txtout << base[i] << " "; 
			txtout << std::endl;
			txtout << "free indices:" << std::endl;
			for(unsigned int i=0; i<ind_free.size(); ++i)
			txtout << free_indices[i] << " "; 
			txtout << std::endl;
			txtout << "lengths_of_dummy_sets:" << std::endl;
			for(unsigned int i=0; i<dummy_sets.size(); ++i)
				txtout << lengths_of_dummy_sets[i] 
						 << " (metric=" << metric_signatures[i] << ") "; 
			txtout << std::endl;
			txtout << "dummies:" << std::endl;
			for(unsigned int i=0; i<ind_dummy.size(); ++i)
				txtout << dummies[i] << " "; 
			txtout << std::endl;
#endif

		stopwatch sw;
		sw.start();

		// JMM now uses a different convention. 
		int *perm1 = new int[total_number_of_indices+2];
		int *perm2 = new int[total_number_of_indices+2];
		int *free_indices_new_order = new int[ind_free.size()];
		int *dummies_new_order      = new int[ind_dummy.size()];

		inverse(perm, perm1, total_number_of_indices+2);
		for(size_t i=0; i<ind_free.size(); i++) {
			free_indices_new_order[i] = onpoints(free_indices[i], perm1, total_number_of_indices+2);
			}
		for(size_t i=0; i<ind_dummy.size(); i++) {
			dummies_new_order[i] = onpoints(dummies[i], perm1, total_number_of_indices+2);
			}
#ifdef XPERM_DEBUG
		txtout << "perm1: ";
		for(unsigned int i=0; i<total_number_of_indices; ++i) {
			txtout << perm1[i] << " ";
			}
		txtout << std::endl;
#endif
		// Brief reminder of the meaning of the various arrays, using the example in
		// Jose's xPerm paper (not yet updated to reflect the _ext version which allows
		// for multiple dummy sets and repeated (numerical) indices):
		//
      // expression        = R_{b}^{1d1} R_{c}^{bac}
		//
      // sorted_index_set  = {a,d,b,-b,c,-c,1,1}
		// base              = {1,2,3, 4,5, 6,7,8}
		// dummies           = {3,4,5,6}
		//    sorted_index_set[3] and [4] is a dummy pair
		//    sorted_index_set[5] and [6] is a dummy pair
		//    these can just be in sorted order, i.e. only referring
		//    to the sorted_index_set.
		//
		// frees             = {1,2}
		//    sorted_index_set[1] and [2] are free indices
		//
		// dummies_new_order = {dn[1], dn[2], ...}
		//    slot dn[1] and dn[2] form a dummy pair ?
		//
		// free_indices_new_order = {...}
		//    these slots contain free indices ?
		//
		// perm = {4,7,2,8,6,3,1,5,9,10}
      //    1st slot contains sorted_index_set[4] ( = -b )
      //    2nd slot contains sorted_index_set[7] ( = 1 )
      //    etc.
      //
      // perm1 = {p1[1], p1[2], ...}
		//
		//    1st index sits in slot p1[1] ?
		//
		// cperm = {1,3,4,5,2,7,6,8,9,10}
		//
		//    1st slot gets sorted_index_set[1] ( = a )
      //    2nd slot gets sorted_index_set[3] ( = b )
		//    ...

		canonical_perm_ext(perm1,                       // permutation to be canonicalised
								 total_number_of_indices+2,  // degree (+2 for the overall sign)
								 1,                          // is this a strong generating set?
								 base,                       // base for the strong generating set
								 base_here.size(),           //    its length
								 gs,                         // generating set
								 generating_set.size(),      //    its size
								 free_indices_new_order,     // free indices
								 ind_free.size(),            // number of free indices
								 lengths_of_dummy_sets,      // list of lengths of dummy sets
								 dummy_sets.size(),          //    its length
								 dummies_new_order,          // list with pairs of dummies
								 ind_dummy.size(),           //    its length
								 metric_signatures,          // list of symmetries of metric
								 0, //lengths_of_repeated_sets,   // list of lengths of repeated-sets
								 0,                          //    its length
								 0, //repeated_indices,           // list with repeated indices
								 0,                          //    its length
								 perm2);                     // output

		if (perm2[0] != 0) inverse(perm2, cperm, total_number_of_indices+2);
		else copy_list(perm2, cperm, total_number_of_indices+2);

		delete [] dummies_new_order;
		delete [] free_indices_new_order;
		delete [] perm1;
		delete [] perm2;

		sw.stop();
//		txtout << "xperm took " << sw << std::endl;

#ifdef XPERM_DEBUG		
		txtout << "cperm:" << std::endl;
		for(unsigned int i=0; i<total_number_of_indices+2; ++i)
			txtout << cperm[i] << " ";
		txtout << std::endl;
#endif

		if(cperm[0]!=0) {
			bool has_changed=false;
			for(unsigned int i=0; i<total_number_of_indices+1; ++i) {
				if(perm[i]!=cperm[i]) {
					has_changed=true;
					break;
					}
				}
			if(has_changed) {
				if(static_cast<unsigned int>(cperm[total_number_of_indices+1])==total_number_of_indices+1) {
					flip_sign(it->multiplier);
					}
				expression_modified=true;
				
				for(unsigned int i=0; i<total_number_of_indices; ++i) {
					// In the new final permutation, e.g.
					// 
					// 1 5 6 8 7 2 3 4 10 9
					//  
					// we place first the first index (m), which goes to the first slot. Then
					// we put n, which can only go the fifth slot. Then we put p, which can go
					// to 6,7,8, so that it goes to 6. Then we put r (not q), which can go to 7
					// and 8, and so we put it at 7, etc.

#ifdef XPERM_DEBUG					
					txtout << "putting index " << i+1 << "(" << *num_to_tree_map[i].begin()->name 
							 << ", " << num_to_tree_map[i].begin()->fl.parent_rel 
							 << ") in slot " << cperm[i] << std::endl;
#endif
					
					iterator ri = tr.replace_index(num_to_it_map[cperm[i]-1], num_to_tree_map[i].begin());
//					assert(ri->fl.parent_rel==num_to_tree_map[i].begin()->fl.parent_rel);
					ri->fl.parent_rel=num_to_tree_map[i].begin()->fl.parent_rel;
					}
				}
			}
		else {
			zero(it->multiplier);
			expression_modified=true;
			}
		
		if(gs) 
			delete [] gs;
		delete [] base;

		delete [] repeated_indices;
		delete [] lengths_of_repeated_sets;
		delete [] metric_signatures;
		delete [] lengths_of_dummy_sets;
		delete [] dummies;
		delete [] cperm;
		delete [] perm;
		}
	
	cleanup_expression(tr, it);

	delete [] free_indices;

	totalsw.stop();
//	txtout << "total canonicalise took " << totalsw << std::endl;
	
	if(expression_modified) return l_applied;
	else return l_no_action;
	}

reduce::reduce(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool reduce::can_apply(iterator it) 
	{
	if(*it->name!="\\sum") return false;
	
	return false;
	}

algorithm::result_t reduce::apply(iterator& it)
	{
	return l_no_action;
	}

acanonicalorder::acanonicalorder(exptree& tr, iterator it)
	: algorithm(tr, it), locate(tr, it), order(tr, it)
	{
	}

canonicalorder::canonicalorder(exptree& tr, iterator it)
	: algorithm(tr, it), locate(tr, it), order(tr, it)
	{
	}

algorithm::result_t canonicalorder::apply(iterator& st)
	{
	return doit(st, false);
	}

algorithm::result_t acanonicalorder::apply(iterator& st)
	{
	return doit(st, true);
	}



ratrewrite::ratrewrite(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool ratrewrite::can_apply(iterator st)
	{
	if(*st->name!="1" && st->is_unsimplified_rational() 
		/* && st->fl.parent_rel!=str_node::p_sub && st->fl.parent_rel!=str_node::p_super */) return true;
	else return false;
	}

algorithm::result_t ratrewrite::apply(iterator& st)
	{
	multiplier_t num(*st->name);
	st->name=name_set.insert("1").first;
	multiply(st->multiplier,num);
	expression_modified=true;
	return l_applied;
	}


impose_asym::impose_asym(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool impose_asym::can_apply(iterator it)
	{
	const Symmetric      *s=properties::get<Symmetric>(it);
	const KroneckerDelta *d=properties::get<KroneckerDelta>(it);
	if(s || ( d && tr.number_of_children(it)==2) ) return true;
	else return false;
	}

algorithm::result_t impose_asym::apply(iterator& st)
	{
	unsigned int number_found=0;

	if(number_of_args()>0) {
		sibling_iterator obj=tr.begin(args_begin());
		while(obj!=tr.end(args_begin())) {
			iterator thename=obj;
			if((*obj->name).size()==0)
				thename=tr.begin(obj);
//			debugout << "impose_asym: searching " << *thename->name << std::endl;
			sibling_iterator it=tr.begin(st);
			while(it!=tr.end(st)) {
				if(thename->fl.parent_rel==it->fl.parent_rel) 
					if(*thename->name==*it->name) {
//						debugout << "impose_asym: found " << *it->name << std::endl;
						if(++number_found>1)
							break;
						}
				++it;
				}
			++obj;
			}
		if(number_found>1) {
			st->multiplier=rat_set.insert(0).first;
			expression_modified=true;
			}
		}
	return l_applied;
	}


eqn::eqn(exptree& tr, iterator it)
	: algorithm(tr,it)
	{
	}

bool eqn::can_apply(iterator it)
	{
	if(tr.number_of_children(tr.parent(it))!=1) {
		throw ArgumentException("@(...) does not have any child nodes.");
		}
	return true;
	}

algorithm::result_t eqn::apply(iterator& st)
	{
	if(st==tr.end()) return l_error;

	// st points to the equation number or name
	iterator theeq=tr.equation_by_number_or_name(st, last_used_equation_number); // FIXME: cleanup
	if(theeq==tr.end()) {
		if(st->is_integer()) {
			std::ostringstream str;
			str << "Expression (" << *st->multiplier << ") does not exist.";
			throw ConsistencyException(str.str());
			}
		else 
			throw ConsistencyException("Expression (" + *st->name + ") does not exist.");
		}
//	tr.print_recursive_treeform(txtout, theeq);
	theeq=tr.active_expression(theeq);
	if(theeq==tr.named_parent(st, "\\expression")) {
		throw ConsistencyException("Infinite recursion.");
		}

	theeq=tr.begin(theeq);

	str_node::bracket_t    br=st->fl.bracket;
	str_node::parent_rel_t pr=st->fl.parent_rel;
	st=tr.replace(st, theeq);
	st->fl.bracket=br;
	st->fl.parent_rel=pr;
	expression_modified=true;
	return l_applied;
	}


indexsort::indexsort(exptree& tr, iterator it) 
	: algorithm(tr, it), tb(0)
	{
	}

bool indexsort::can_apply(iterator st)
	{
	if(tr.number_of_indices(st)<2) return false;
	tb=properties::get<TableauBase>(st);
	if(tb) return true;
	return false;
	}

indexsort::less_indexed_treenode::less_indexed_treenode(exptree& t, iterator i)
	: tr(t), it(i)
	{
	}

bool indexsort::less_indexed_treenode::operator()(unsigned int i1, unsigned int i2) const
	{
	return subtree_exact_less(tr.tensor_index(it,i1), tr.tensor_index(it,i2));
	}

algorithm::result_t indexsort::apply(iterator& st)
	{
//	txtout << "indexsort acting on " << *st->name << std::endl;
//	txtout << properties::get<TableauBase>(st) << std::endl;

	exptree backup(st);

	for(unsigned int i=0; i<tb->size(tr, st); ++i) {
		TableauSymmetry::tab_t tmptab(tb->get_tab(tr, st, i));
		TableauSymmetry::tab_t origtab(tmptab);
		less_indexed_treenode comp(tr,st);
		tmptab.canonicalise(comp, false); // KP: why is this here? tb->only_column_exchange());
		TableauSymmetry::tab_t::iterator it1=origtab.begin();
		TableauSymmetry::tab_t::iterator it2=tmptab.begin();
		while(it2!=tmptab.end()) {
			if(*it1!=*it2) {
				tr.replace_index(tr.tensor_index(st,*it1), backup.tensor_index(backup.begin(),*it2));
//				tr.tensor_index(st,*it1)->multiplier=backup.tensor_index(backup.begin(),*it2)->multiplier;
				expression_modified=true;
				}
			++it1;
			++it2;
			}
		if(*(tr.parent(st)->name)=="\\prod") {
			multiply(tr.parent(st)->multiplier, tmptab.multiplicity*origtab.multiplicity);
			pushup_multiplier(tr.parent(st));
			}
		else {
			multiply(st->multiplier, tmptab.multiplicity*origtab.multiplicity);
			pushup_multiplier(st);
			}
		}
	return l_applied;
	}

asymprop::asymprop(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool asymprop::can_apply(iterator st)
	{
	tb=properties::get<TableauBase>(st);
	if(tb) return true;
	return false;
	}

algorithm::result_t asymprop::apply(iterator& st)
	{
	for(unsigned int i=0; i<tb->size(tr, st); ++i) {
		TableauSymmetry::tab_t tmptab(tb->get_tab(tr, st, i));
		for(unsigned int c=0; c<tmptab.row_size(0); ++c) {
			TableauSymmetry::tab_t::in_column_iterator it1=tmptab.begin_column(c);
			while(it1!=tmptab.end_column(c)) {
				TableauSymmetry::tab_t::in_column_iterator it2=it1;
				++it2;
				// FIXME: this is slow
				while(it2!=tmptab.end_column(c)) {
					if(tr.child(st, *it1)->name == tr.child(st, *it2)->name) {
						expression_modified=true;
						zero(st->multiplier);
						return l_applied;
						}
					++it2;
					}
				++it1;
				}
			} 
		}
	return l_applied;
	}


young_project::young_project(exptree& tr, iterator it)
	: algorithm(tr,it), remove_traces(false)
	{
	if(it==tr.end()) return; // for in-program calls

	if(number_of_args()==2) {
		sibling_iterator arg=args_begin();
		if(*arg->name=="\\comma") {
			++arg;
			if(*arg->name=="\\comma") {
				sibling_iterator arg=args_begin();
				sibling_iterator ycb=tr.begin(arg), yce=tr.end(arg);
				++arg;
				sibling_iterator icb=tr.begin(arg), ice=tr.end(arg);
				unsigned int rownum=0;
				while(ycb!=yce) {
					for(unsigned int r=0; r<*ycb->multiplier; ++r) {
						if(icb==ice) 
							throw ConsistencyException("out of range");
						if(icb->is_rational()) { // index given by position
							if(*icb->multiplier<0)
								throw ConsistencyException("index labels out of range");
							tab.add_box(rownum,to_long(*icb->multiplier));
							}
						else { // index given by name; store in the other tableau
							nametab.add_box(rownum, icb);
							}
						++icb;
						}
					++rownum;
					++ycb;
					}
				return;
				}
			}
		}
	throw algorithm::constructor_error();
	}

bool young_project::can_apply(iterator it)
	{
	if(*it->name!="\\prod") {
		if(!is_single_term(it)) {
			return false;
			}
		}

	prod_wrap_single_term(it);
	if(nametab.number_of_rows()>0) { // have to convert names to numbers
		tab.copy_shape(nametab);
		pos_tab_t::iterator pi=tab.begin();
		name_tab_t::iterator ni=nametab.begin();
		while(ni!=nametab.end()) {
			exptree::index_iterator ii=tr.begin_index(it);
			unsigned int indexnum=0;
			while(ii!=tr.end_index(it)) {
				if(subtree_exact_equal(ii, *ni)) {
					*pi=indexnum;
					break;
					}
				++indexnum;
				++ii;
				}
			if(indexnum==tr.number_of_indices(it)) {
				prod_unwrap_single_term(it);
				return false; // cannot find indicated index in expression
				}
			++pi;
			++ni;
			}
		}

	prod_unwrap_single_term(it);
	return true;
	}

exptree::iterator young_project::nth_index_node(iterator it, unsigned int num)
	{
	exptree::fixed_depth_iterator indname=tr.begin_fixed(it, 2);
	indname+=num;
	return indname;
	}

algorithm::result_t young_project::apply(iterator& it)
	{
	prod_wrap_single_term(it);
	sym.clear();
	
	if(asym_ranges.size()>0) {
		// Convert index locations to box numbers.
		combin::range_vector_t sublengths_scattered;
//		txtout << "asym_ranges: ";
		for(unsigned int i=0; i<asym_ranges.size(); ++i) {
			combin::range_t newr;
			for(unsigned int j=0; j<asym_ranges[i].size(); ++j) {
				// Search asym_ranges[i][j]
				int offs=0;
				pos_tab_t::iterator tt=tab.begin();
				while(tt!=tab.end()) {
					if((*tt)==asym_ranges[i][j]) {
						newr.push_back(offs);
//						txtout << asym_ranges[i][j] << " ";
						break;
						}
					++tt;
					++offs;
					}
				}
			sublengths_scattered.push_back(newr);
//			txtout << std::endl;
			} 
		tab.projector(sym, sublengths_scattered);
		}
	else tab.projector(sym);

	// FIXME: We can also compress the result by sorting all 
	// locations which belong to the same asym set. This could actually
	// be done in combinatorics already.

	exptree rep;
	rep.set_head(str_node("\\sum"));
	for(unsigned int i=0; i<sym.size(); ++i) {
		// Generate the term.
		exptree repfac(it);
		for(unsigned int j=0; j<sym[i].size(); ++j) {
			exptree::index_iterator src_fd=tr.begin_index(it);
			exptree::index_iterator dst_fd=tr.begin_index(repfac.begin());
			src_fd+=sym[i][j];        // take the index at location sym[i][j]
			dst_fd+=sym.original[j];  // and store it in location sym.original[j]
			tr.replace_index(dst_fd, src_fd); 
			}
		// Remove traces of antisymmetric objects. This can really
		// only be done here, since combinatorics.hh does not know
		// about index values, only about index locations. Note: we also
		// have to remove the entry in sym.original & sym.multiplicity if
		// we decide that a term vanishes.
		// IMPORTANT: if there are still permutations by value to be
		// done afterwards, do not use this!
		if(remove_traces) {
			for(unsigned int k=0; k<asym_ranges.size(); ++k) {
				for(unsigned int kk=0; kk<asym_ranges[k].size(); ++kk) {
					exptree::index_iterator it1=repfac.begin_index(repfac.begin());
					it1+=asym_ranges[k][kk];
					for(unsigned int kkk=kk+1; kkk<asym_ranges[k].size(); ++kkk) {
						exptree::index_iterator it2=repfac.begin_index(repfac.begin());
						it2+=asym_ranges[k][kkk];
						if(subtree_exact_equal(it1,it2)) {
							sym.set_multiplicity(i,0);
							goto traceterm;
							}
						}
					}
				} 
			}

		{ multiply(repfac.begin()->multiplier, sym.signature(i));
		multiply(repfac.begin()->multiplier, tab.projector_normalisation());
		iterator repfactop=repfac.begin();
		prod_unwrap_single_term(repfactop);
		rep.append_child(rep.begin(), repfac.begin()); }

	   traceterm: ;
		}
	it=tr.replace(it,rep.begin());
	expression_modified=true;

	sym.remove_multiplicity_zero();

	return l_applied;
	}

young_project_tensor::young_project_tensor(exptree& tr, iterator it)
	: algorithm(tr,it), modulo_monoterm(false)
	{
	}

bool young_project_tensor::can_apply(iterator it)
	{
	tb=properties::get_composite<TableauBase>(it);
	if(tb) {
		if(has_argument("ModuloMonoterm"))
			modulo_monoterm=true;
		return true;
		}
	else return false;
	}

algorithm::result_t young_project_tensor::apply(iterator& it)
	{
	assert(tb);
//	txtout << typeid(*tb).name() << std::endl;
	TableauBase::tab_t tab=tb->get_tab(tr, it, 0);
//	txtout << tab << std::endl;

	if(modulo_monoterm) {
		if(tab.number_of_rows()==1) // Nothing happends with fully symmetric tensors modulo monoterm.
			return l_applied;
		if(tab.row_size(0)==1 && tab.selfdual_column==0) // Ditto for fully anti-symmetric tensors & modmono.
			return l_applied;
		}

	// For non-trivial tableau shapes, apply the Young projector.
	exptree rep;
	rep.set_head(str_node("\\sum"));
	if(tab.row_size(0)>0) {
		sym.clear();
		tab.projector(sym); //, modulo_monoterm);
	
//		txtout << sym.size() << std::endl;
		for(unsigned int i=0; i<sym.size(); ++i) {
			exptree repfac(it);
			for(unsigned int j=0; j<sym[i].size(); ++j) {
				exptree::index_iterator src_fd=tr.begin_index(it);
				exptree::index_iterator dst_fd=tr.begin_index(repfac.begin());
//			txtout << sym[i][j] << " " << sym.original[j] << std::endl;
				src_fd+=sym[i][j];
				dst_fd+=sym.original[j];
//			txtout << *src_fd->name  << std::endl;
//			txtout << *dst_fd->name  << std::endl;
				dst_fd->name=src_fd->name;
				}
			multiply(repfac.begin()->multiplier, sym.signature(i));
			multiply(repfac.begin()->multiplier, tb->get_tab(tr, it, 0).projector_normalisation());
			iterator newtensor=rep.append_child(rep.begin(), repfac.begin());
			if(modulo_monoterm) { // still necessary for column exchange
				indexsort isort(rep, rep.end());
				assert(isort.can_apply(newtensor)); // to set tb
				isort.apply(newtensor);
				}
			}
		collect_terms cterms(rep, rep.end());
		iterator rephead=rep.begin();
		cterms.apply(rephead);
		}
	else {
		rep.append_child(rep.begin(), it);
		}

	// If there is a selfdual or anti-selfdual component, we need to add a term
	// for each generated term, which contains an epsilon. In the generated term, 
	// find the positions of the indices which originally sat on the selfdual tensor,
	// then replace these with dummies which are repeated on the epsilon tensor.

   //	We should do all of this _here_, not before the Young projector, because we
	// want to avoid using indexsort.

	if(tab.selfdual_column!=0) {
		// Classify indices so we can insert dummies.
		index_map_t one, two, three, four, added_dummies;
		classify_indices_up(it, one, two);
		classify_indices(it, three, four);

		// Figure out the properties of the indices for which we want dummy partners.
		exptree::index_iterator iit=tr.begin_index(it);
		iit+=tab(0,abs(tab.selfdual_column)-1);
		const Integer *itg=properties::get<Integer>(iit, true);
		const Indices *ind=properties::get<Indices>(iit, true);
		if(itg==0)
			throw ConsistencyException("Need to know the range of the indices.");
		if(ind==0)
			throw ConsistencyException("Need to have a set of dummy indices.");

		// Step through all generated terms and give each one an epsilon partner term.
		sibling_iterator tt=rep.begin(rep.begin());
		while(tt!=rep.end(rep.begin())) {
			exptree repfac(tt);
			iterator prodit=repfac.wrap(repfac.begin(), str_node("\\prod"));
			iterator tensit=repfac.begin(prodit);
         // FIXME: take care of Euclidean signature cases.
         //			repfac.insert(repfac.begin(prodit), str_node("I"));
			iterator epsit =repfac.append_child(prodit, str_node("\\epsilon"));

			// Normalise the epsilon term appropriately.
			multiply(prodit->multiplier, 
						multiplier_t(1)/combin::factorial(to_long(*(itg->difference.begin()->multiplier)/2)));
			if(tab.selfdual_column<0)
				flip_sign(prodit->multiplier);

			// Move the free indices to the epsilon factor.
			for(unsigned int row=0; row<tab.column_size(abs(tab.selfdual_column)-1); ++row) {
				// Find index of original in newly generated term (all indices are different,
				// so this works).
				iit=repfac.begin_index(tensit);
				exptree::index_iterator iit_orig=tr.begin_index(it);
				iit_orig+=tab(row, abs(tab.selfdual_column)-1);
				while(subtree_exact_equal(iit, iit_orig)==false) 
					++iit;
				
				exptree dum=get_dummy(ind, &one, &two, &three, &four, &added_dummies);
				repfac.append_child(epsit, iterator(iit)); // move index to eps
				iterator repind=rep.replace_index(iterator(iit), dum.begin()); // replace index on tens
				added_dummies.insert(index_map_t::value_type(dum, repind));
				}
			// Now insert the new dummies in the epsilon.
			index_map_t::iterator adi=added_dummies.begin();
			while(adi!=added_dummies.end()) {
				iterator ni=repfac.append_child(epsit, adi->first.begin());
				ni->fl.parent_rel=str_node::p_sub;
				++adi;
				}
			added_dummies.clear();

			++tt;
			rep.insert_subtree(tt, repfac.begin());
			}
		}

	// Final cleanup
	it=tr.replace(it,rep.begin());
	expression_modified=true;

	cleanup_nests(tr, it);
	cleanup_expression(tr, it);
	return l_applied;
	}

expand_power::expand_power(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	}

bool expand_power::can_apply(iterator it)
	{
	if(*it->name=="\\pow") {
		sibling_iterator exponent=tr.begin(it);
		++exponent;
		if(exponent->is_integer())
			return true;
		}
	return false;
	}

algorithm::result_t expand_power::apply(iterator& it) 
	{
	iterator argument=tr.begin(it);
	sibling_iterator exponent=tr.begin(it);
	++exponent;

	int num=to_long(*exponent->multiplier);
	if(num<=1) 
		return l_no_action;

	iterator prodn=tr.insert(argument,str_node("\\prod"));

	// If the current \pow is inside a sum, do not discard the bracket
	// type on \pow but copy it onto each generated \prod element.
	if(*tr.parent(it)->name=="\\sum") 
		prodn->fl.bracket=it->fl.bracket;
	
	sibling_iterator beg=argument;
	sibling_iterator nd=beg;
	++nd;
   argument=tr.reparent(prodn,beg,nd);
	tr.erase(exponent);
	tr.flatten(it);
	multiply(prodn->multiplier, *it->multiplier);
	it=tr.erase(it);

	// Now duplicate the factor num-1 times.
	multiplier_t tot=*argument->multiplier;
	for(int i=0; i<num-1; ++i) {
		iterator tmp=tr.append_child(prodn);
		tot *= *argument->multiplier;
		iterator ins=tr.replace(tmp, argument);
		one(ins->multiplier);
		rename_replacement_dummies(ins);
		}
	one(argument->multiplier);
	multiply(prodn->multiplier, tot);

	cleanup_nests_below(tr,it);
	cleanup_nests(tr,it);

	expression_modified=true;
	return l_applied;
	}

