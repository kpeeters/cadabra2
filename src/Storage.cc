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

#include "Storage.hh"
#include "Combinatorics.hh"
#include "Props.hh"
#include <iomanip>
#include <sstream>
#include <pcrecpp.h>

nset_t    name_set;
rset_t    rat_set;

long to_long(multiplier_t mul)
	{
	return mul.get_num().get_si();
	}

std::string to_string(long num)
	{
	std::ostringstream str;
	str << num;
	return str.str();
	}

exptree::exptree()
	: tree<str_node>()
	{
	}

exptree::exptree(tree<str_node>::iterator it)
	: tree<str_node>(it)
	{
	}

exptree::exptree(const str_node& x)
	: tree<str_node>(x)
	{
	}

std::ostream& exptree::print_recursive_treeform(std::ostream& str, exptree::iterator it) 
	{
	unsigned int num=1;
	return print_recursive_treeform(str, it, num);
	}

std::ostream& exptree::print_entire_tree(std::ostream& str) const
	{
	sibling_iterator sib=begin();
	unsigned int num=1;
	while(sib!=end()) {
		print_recursive_treeform(str, sib, num);
		++sib;
		++num;
		}
	return str;
	}

std::ostream& exptree::print_recursive_treeform(std::ostream& str, exptree::iterator it, unsigned int& num)
	{
	bool compact_tree=getenv("CDB_COMPACTTREE");

	exptree::sibling_iterator beg=it.begin();
	exptree::sibling_iterator fin=it.end();

	if((*it).fl.bracket   ==str_node::b_round)       str << "(";
	else if((*it).fl.bracket   ==str_node::b_square) str << "[";
	else if((*it).fl.bracket   ==str_node::b_curly)  str << "\\{";
	else if((*it).fl.bracket   ==str_node::b_pointy) str << "\\<";
	else if((*it).fl.bracket   ==str_node::b_none)   str << "{";
//	if((*it).fl.mark) str << "\033[1m";
	str << *(*it).name;
//	if((*it).fl.mark) str << "\033[0m";
	if((*it).fl.bracket   ==str_node::b_round)       str << ")";
	else if((*it).fl.bracket   ==str_node::b_square) str << "]";
	else if((*it).fl.bracket   ==str_node::b_curly)  str << "\\}";
	else if((*it).fl.bracket   ==str_node::b_pointy) str << "\\>";
	else if((*it).fl.bracket   ==str_node::b_none)   str << "}";

	if(*it->multiplier!=multiplier_t(1)) {
		if(compact_tree)
			str << "#" << *it->multiplier;
		else
			str << "  " << *it->multiplier;
		}
//	str << "  (" << calc_hash(it) << ")";
//	str << "  (" << depth(it) << ")";
	str << "  (" << &(*it) << ")";
	if(!compact_tree) str << std::endl;

	while(beg!=fin) {
		int offset=1;
		if(num && !compact_tree) {
			str << std::setw(3) << num << ":";
			offset=1;
			}
		if(!compact_tree)
			 for(int i=offset; i<depth(beg); ++i) 
				str << "  ";
		switch((*beg).fl.parent_rel) {
			case str_node::p_super: str << "^"; break;
			case str_node::p_sub:   str << "_"; break;
			case str_node::p_property: str << "$"; break;
			case str_node::p_exponent: str << "&"; break;
			default: break;
			}
		if(num) ++num;
		print_recursive_treeform(str, beg, num);
		++beg;
		}
	return str;
	}


// std::ostream& operator<<(std::ostream& str, const exptree& tr)
// 	{
// 	unsigned int number=1;
// 	exptree::iterator it=tr.begin();
// 	while(it!=tr.end()) {
// 		tr.print_recursive_infix(str, it, number, true);
// 		it.skip_children();
// 		++it;
// 		if(it!=tr.end())
// 			str << std::endl;
// 		}
// 	return str;
// 	}

exptree::iterator exptree::named_parent(exptree::iterator it, const std::string& nm) const
	{
//	std::cout << "!!" << *it->name << std::endl << std::flush;
	assert(is_valid(it));
	while(*it->name!=nm /* && it->is_command()==false */) {
		it=parent(it);
		assert(is_valid(it));
//		std::cout << "  !!" << *it->name << std::endl << std::flush;
		}
//	std::cout << "  out" << std::endl;
	return it;
	}

exptree::iterator exptree::erase_expression(exptree::iterator it) 
	{
	it=named_parent(it, "\\history");
	return erase(it);
	}

exptree::iterator exptree::keep_only_last(exptree::iterator it)
	{
	it=named_parent(it, "\\history");
	if(begin(it)==end(it)) return it;

	sibling_iterator expit=end(it);
	--expit;
//	std::cout << *expit->name << std::endl;
	while(expit.node!=0) { // FIXME: this is a hack, how does one do 'rbegin'?
		if(*expit->name=="\\expression") {
//			std::cout << "found expression node" << std::endl;
			sibling_iterator prev=begin(it);
			while(prev!=expit) {
				if(*prev->name=="\\expression") {
//					std::cout << "erasing old expression" << std::endl;
					prev=erase(prev);
					}
				else
					++prev;
				}
			return expit;
			}
		--expit;
		}
	return it;
	}

hashval_t exptree::calc_hash(iterator it) const
	{
	// Hash values do not contain info about the multiplier field,
	// nor do they know about the type of the links (FIXME: is the latter
	// the correct thing to do?)
	//
	// If this algorithm is changed, factorise::calc_restricted_hash in 
	// modules/algebra.cc should also be modified!

	hashval_t ret=(hashval_t)(&(*it->name));

	sibling_iterator sub=begin(it);
	while(sub!=end(it)) {
		ret*=17;
		ret+=calc_hash(sub);
		++sub;
		}

	return ret;
	}

exptree::sibling_iterator exptree::arg(iterator it, unsigned int num) 
	{
	if(*it->name=="\\comma") {
		assert(exptree::number_of_children(it)>num);
		return exptree::child(it,num);
		}
	else return it;
	}

unsigned int exptree::arg_size(sibling_iterator sib) 
	{
	if(*sib->name=="\\comma") return exptree::number_of_children(sib);
	else return 1;
	}

multiplier_t exptree::arg_to_num(sibling_iterator sib, unsigned int num) const
	{
	sibling_iterator nod;
	if(*sib->name=="\\comma") nod=child(sib,num);
	else                      nod=sib;
	return *nod->multiplier;
	}

exptree::sibling_iterator exptree::tensor_index(const iterator_base& position, unsigned int num) const
	{
	index_iterator ret=begin_index(position);
	while(num-- > 0)
		++ret;
	return ret;

//	const Derivative *der=properties::get<Derivative>(position);
//	if(der) {
//		unsigned int tensor_children=number_of_children(begin(position));
//		if(num<tensor_children) return child(begin(position), num);
//		else                    return child(position, num+1-tensor_children);
//		}
//	else return child(position, num);
	}

// Given an iterator somewhere inside an expression (can be the
// \\history node), this member returns an iterator pointing to the
// \expression node of the active expression. 
exptree::iterator exptree::active_expression(exptree::iterator it) const
	{
	it=named_parent(it, "\\history");
	exptree::sibling_iterator sube=end(it);
	--sube;
	return sube;
	}

/*unsigned int exptree::number_of_steps(exptree::iterator it) const
	{
	it=named_parent(it, "\\expression");
	sibling_iterator sib=begin(it);
	unsigned int ret=0;
	while(sib!=end(it)) {
		if(*sib->name=="\\history")
			++ret;
		++sib;
		}
	return ret;
	}
*/

//void exptree::select(unsigned int node, unsigned int mark)
//	{
//	iterator it=begin();
//	unsigned int here=1;
//	while(it!=end()) {
//		if(here==node) {
//			it->fl.mark=mark;
//			break;
//			  iterator nd=it;
//			  nd.skip_children();
//			  ++nd;
//			  while(it!=nd) {
//				  it->fl.mark=mark;
//				  ++it;
//				  }
//			  break;
//			}
//		++here;
//		++it;
//		}
//	}

//void exptree::select(iterator it, unsigned int mark)
//	{
//	it->fl.mark=mark;
//	  iterator nd=it;
//	  nd.skip_children();
//	  ++nd;
//	  while(it!=nd) {
//		  it->fl.mark=mark;
//		  ++it;
//		  }
//	}

//void exptree::unselect(unsigned int node)
//	{
//	select(node, 0);
//	}

//void exptree::unselect(iterator it, bool deep)
//	{
//	select(it, 0);
//	if(deep) {
//		iterator nxt=it;
//		nxt.skip_children();
//		++nxt;
//		iterator ch=begin(it);
//		while(ch!=nxt) {
//			select(ch,0);
//			++ch;
//			}
//		}
//	}

//void exptree::select_all(unsigned int mark)
//	{
//	iterator it=begin();
//	while(it!=end()) {
//		(*it).fl.mark=mark;
//		++it;
//		}
//	}
//
//void exptree::unselect_all(unsigned int mark)
//	{
//	iterator it=begin();
//	while(it!=end()) {
//		if((*it).fl.mark==mark)
//			(*it).fl.mark=0;
//		++it;
//		}
//	}
//
//void exptree::unselect_all()
//	{
//	iterator it=begin();
//	while(it!=end()) {
//		(*it).fl.mark=0;
//		++it;
//		}
//	}

//void exptree::marked_nodes(std::vector<iterator>& v) const
//	{
//	iterator it=begin();
//	while(it!=end()) {
//		if(it->fl.mark) {
//			v.push_back(it);
//			it.skip_children();
//			}
//		++it;
//		}
//	}

unsigned int exptree::equation_number(exptree::iterator it) const
	{
	iterator historynode=named_parent(it, "\\history");
	unsigned int num=0;
	iterator sit=begin();
//	long totravel=0;
	while(sit!=end()) {
//		++totravel;
		if(*sit->name=="\\history") {
			++num;
			if(historynode==sit) {
//				txtout << "had to travel " << totravel << std::endl;
				return num;
				}
			}
		sit.skip_children();
		++sit;
		}
	return 0;
	}

nset_t::iterator exptree::equation_label(exptree::iterator it) const
	{
	nset_t::iterator ret=name_set.end();

	iterator sit=begin();
	while(sit!=end()) {
		if(*sit->name=="\\history") {
			if(it==sit)
				goto found;
			iterator eit=begin(sit);
			iterator endit=sit;
			endit.skip_children();
			++endit;
			while(eit!=endit) {
				if(it==eit)
					goto found;
				++eit;
				}
			sit.skip_children();
			}
		++sit;
		}
	found:
	if(sit!=end()) {
		sibling_iterator lit=begin(sit);
		while(lit!=end(sit)) {
			if(*lit->name=="\\label") {
				ret=begin(lit)->name;
				break;
				}
			++lit;
			}
		}
	return ret;
	}

// Always returns the \\history node of the equation (i.e. the top node).
exptree::iterator exptree::equation_by_number(unsigned int i) const
	{
	iterator it=begin();
	unsigned int num=1;
	while(it!=end()) {
		if(*it->name=="\\history") {
			if(num==i) return it;
			else       ++num;
			}
		it.skip_children();
		++it;
		}
	return it;
//	if(num==number_of_siblings(begin()))
//		return end();
//	return it;
	}

exptree::iterator exptree::equation_by_name(nset_t::iterator nit) const
	{
	unsigned int tmp;
	return equation_by_name(nit, tmp);
	}

exptree::iterator exptree::equation_by_name(nset_t::iterator nit, unsigned int& tmp) const
	{
	unsigned int num=0;
	iterator it=begin();
	while(it!=end()) {
		if(*it->name=="\\history") {
			++num;
			sibling_iterator lit=begin(it);
			while(lit!=end(it)) {
				if(*lit->name=="\\label") {
					if(begin(lit)->name==nit) {
						tmp=num;
						return it;
						}
					}
				++lit;
				}
			}
		it.skip_children();
		++it;
		}
	return end();
	}

exptree::iterator exptree::procedure_by_name(nset_t::iterator nit) const
	{
	iterator it=begin();
	while(it!=end()) {
		if(*it->name=="\\procedure") {
			sibling_iterator lit=begin(it);
			while(lit!=end(it)) {
				if(*lit->name=="\\label") {
					if(begin(lit)->name==nit)
						return it;
					}
				++lit;
				}
			}
		it.skip_children();
		++it;
		}
	return end();
	}

exptree::iterator exptree::replace_index(iterator pos, const iterator& from)
	{
//	assert(pos->fl.parent_rel==str_node::p_sub || pos->fl.parent_rel==str_node::p_super);
	str_node::bracket_t    bt=pos->fl.bracket;
	str_node::parent_rel_t pr=pos->fl.parent_rel;
	iterator ret=replace(pos, from);
	ret->fl.bracket=bt;
	ret->fl.parent_rel=pr;
	return ret;
	}

exptree::iterator exptree::move_index(iterator pos, const iterator& from)
	{
//	assert(pos->fl.parent_rel==str_node::p_sub || pos->fl.parent_rel==str_node::p_super);
	str_node::bracket_t    bt=pos->fl.bracket;
	str_node::parent_rel_t pr=pos->fl.parent_rel;
	move_ontop(pos, from);
	from->fl.bracket=bt;
	from->fl.parent_rel=pr;
	return from;
	}

void exptree::list_wrap_single_element(iterator& it)
	{
	if(*it->name!="\\comma") {
		iterator commanode=insert(it, str_node("\\comma"));
		sibling_iterator fr=it, to=it;
		++to;
		reparent(commanode, fr, to);
		it=commanode;
		}
	}

void exptree::list_unwrap_single_element(iterator& it)
	{
	if(*it->name=="\\comma") {
		if(number_of_children(it)==1) {
			flatten(it);
			it=erase(it);
			}
		}
	}

exptree::iterator exptree::flatten_and_erase(iterator pos)
	{
	multiplier_t tmp=*pos->multiplier;
	flatten(pos);
	pos=erase(pos);
	multiply(pos->multiplier, tmp);
	return pos;
	}

unsigned int exptree::number_of_indices(iterator it) 
	{
	unsigned int res=0;
	index_iterator indit=begin_index(it);
	while(indit!=end_index(it)) {
		++res;
		++indit;
		}
	return res;
	}

unsigned int exptree::number_of_direct_indices(iterator it) const
	{
	unsigned int res=0;
	sibling_iterator sib=begin(it);
	while(sib!=end(it)) {
		if(sib->fl.parent_rel==str_node::p_sub || sib->fl.parent_rel==str_node::p_super)
			++res;
		++sib;
		}
	return res;
	}

exptree::index_iterator::index_iterator()
	: iterator_base()
	{
	}

//exptree::index_iterator::index_iterator(tree_node *tn)
//	: iterator_base(tn), halt(tn), walk(tn), roof(tn)
//	{
//	halt.skip_children();
//	++halt;
//	operator++(); // FIXME: should we check whether we are at a valid index?
//	}
//
exptree::index_iterator exptree::index_iterator::create(const iterator_base& other)
	{
	index_iterator ret;
	ret.node=other.node;
	ret.halt=other;
	ret.walk=other;
	ret.roof=other;

	ret.halt.skip_children();
	++ret.halt;
	ret.operator++(); 
	return ret;
	}

exptree::index_iterator::index_iterator(const index_iterator& other) 
	: iterator_base(other.node), halt(other.halt), walk(other.walk), roof(other.roof)
	{
	}

bool exptree::index_iterator::operator!=(const index_iterator& other) const
	{
	if(other.node!=this->node) return true;
	else return false;
	}

bool exptree::index_iterator::operator==(const index_iterator& other) const
	{
	if(other.node==this->node) return true;
	else return false;
	}

// \bar{\prod{A}{B}} 's indices are undefined, as \bar inherits
// the Product property of \prod. So the worst-case scenario is
// of the type \bar{\hat{A_\mu}} in which the objects with Inherit
// property are strictly nested. However, we can also have
// things like \bar{\diff{\diff{A_\mu}_{\nu}}_{\rho}}, for which 
// we have to collect indices at multiple levels.

/*
  \bar{?}::Accent.
  \bar{\diff{\diff{A_\mu}_{\nu}}_{\rho}};
  @indexlist(%);
  \diff{\diff{A_{\mu}}_{\nu}}_{\rho};
  @indexlist(%);
  \diff{\diff{A}_{\nu}}_{\rho};
  @indexlist(%);
  \bar{\psi_{m}} * \Gamma_{q n p} * \psi_{m} * H_{n p q};
  @indexlist(%);
  q*A_{d c b a};
  @indexlist(%);
  A_{d c b a}*q;
  @indexlist(%);
  \diff{\phi}_s A_\mu \diff{\phi}_t;
  @indexlist(%);
  \Gamma_{a b c};
  @indexlist(%);
  \diff{\sin(x_\mu)}_{\nu};
  @indexlist(%);
  \equals{A_{i}}{B_{i j} Z_{j}};
  @indexlist(%);

*/
exptree::index_iterator& exptree::index_iterator::operator+=(unsigned int num)
	{
	while(num != 0) {
		--num;
		operator++();
		}
	return *this;
	}


exptree::index_iterator& exptree::index_iterator::operator++()
	{
	assert(this->node!=0);
	
	// Increment the iterator. As long as we are at an inherit
	// node, keep incrementing. As long as the parent does not inherit,
   // and as long as we are not at the top node,
	// skip children. As long as we are not at an index, keep incrementing.

	const IndexInherit *this_inh=0, *parent_inh=0;
	while(walk!=halt) {
		this_inh=properties::get<IndexInherit>(walk);
		
		if(this_inh==0 && (walk!=roof && walk.node->parent!=0)) {
			parent_inh=properties::get<IndexInherit>(walk.node->parent);
			if(parent_inh==0)
				walk.skip_children();
			}
		
		++walk;

//		txtout << "walking " << *walk->name << std::endl;
		if(walk!=halt)
			 if(walk->is_index()) 
				  break;
//		if(this_inh==false && walk->is_index())
//			break;
		}
	if(walk==halt) {
		this->node=0;
		return *this;
		}
	else this->node=walk.node;

	return *this;
	}

exptree::index_iterator exptree::begin_index(iterator it)
	{
	return index_iterator::create(it);
	}

exptree::index_iterator exptree::end_index(iterator it)
	{
	index_iterator tmp=index_iterator::create(it);
	tmp.node=0;

	return tmp;
	}

unsigned int exptree::number_of_equations() const
	{
	unsigned int last_eq=0;
	iterator eq=begin();
	while(eq!=end()) {
		if(*eq->name=="\\history")
			++last_eq;
		eq.skip_children();
		++eq;
		}
	return last_eq;
	}

exptree::iterator exptree::equation_by_number_or_name(iterator it, unsigned int last_used_equation, 
																		unsigned int& real_eqno) const
	{
	iterator ret;
	if(it->is_rational()) {
		int eqno=static_cast<int>(it->multiplier->get_d());
		real_eqno=eqno;
		ret=equation_by_number(eqno);
		}
	else {
		if(*it->name=="%") {
			ret=equation_by_number(last_used_equation);
			real_eqno=last_used_equation;
			}
		else {
			ret=equation_by_name(it->name, real_eqno);
			}
		}
	return ret;
	}

exptree::iterator exptree::equation_by_number_or_name(iterator it, unsigned int last_used_equation) const
	{
	unsigned int tmp;
	return equation_by_number_or_name(it, last_used_equation, tmp);
	}

std::string exptree::equation_number_or_name(iterator it, unsigned int last_used_equation) const
	{
	std::stringstream ss;
	if(it->is_rational()) {
		int eqno=static_cast<int>(it->multiplier->get_d());
		ss << eqno;
		}
	else {
		if(*it->name=="%") ss << last_used_equation;
		else               ss << *it->name;
		}
	return ss.str();
	}


str_node::str_node(void)
	{
	multiplier=rat_set.insert(1).first;
//	fl.modifier=m_none;
	fl.bracket=b_none;
	fl.parent_rel=p_none;
//	fl.mark=0;
	}

str_node::str_node(nset_t::iterator nm, bracket_t br, parent_rel_t pr)
	{
	multiplier=rat_set.insert(1).first;
	name=nm;
//	fl.modifier=m_none;
	fl.bracket=br;
	fl.parent_rel=pr;
//	fl.mark=0;
	}

str_node::str_node(const std::string& nm, bracket_t br, parent_rel_t pr)
	{
	multiplier=rat_set.insert(1).first;
	name=name_set.insert(nm).first;
//	fl.modifier=m_none;
	fl.bracket=br;
	fl.parent_rel=pr;
//	fl.mark=0;
	}

void str_node::flip_parent_rel() 
	{
	if(fl.parent_rel==p_super)       fl.parent_rel=p_sub;
	else if(fl.parent_rel==p_sub)    fl.parent_rel=p_super;
	else throw std::logic_error("flip_parent_rel called on non-index");
	}

bool str_node::is_zero() const
	{
	if(*multiplier==0) return true;
	return false;
	}

bool str_node::is_identity() const
	{
	if(*name=="1" && *multiplier==1) return true;
	return false;
	}

bool str_node::is_rational() const
	{
	return (*name=="1");
	}

bool str_node::is_integer() const
	{
	if(*name=="1") {
		if(multiplier->get_den()==1)
			return true;
		}
	return false;
	}

bool str_node::is_unsimplified_rational() const
	{
	if((*name).size()==0) return false;
	for(unsigned int i=0; i<(*name).size(); ++i) {
		if(!isdigit((*name)[i]) && (*name)[i]!='/' && (*name)[i]!='-')
			return false;
		}
	return true;
	}

bool str_node::is_unsimplified_integer() const
	{
	if((*name).size()==0) return false;
	for(unsigned int i=0; i<(*name).size(); ++i) {
		if(!isdigit((*name)[i]) && (*name)[i]!='-')
			return false;
		}
	return true;
	}

bool str_node::is_index() const
	{
	if(fl.parent_rel==p_sub || fl.parent_rel==p_super) return true;
	return false;
	}



bool str_node::is_quoted_string() const
	{
	if((*name).size()<2) return false;
	if((*name)[0]!='\"') return false;
	if((*name)[(*name).size()-1]!='\"') return false;
	return true;
	}

bool str_node::is_command() const
	{
	if((*name).size()>0)
		if((*name)[0]=='@') {
			if((*name).size()>1) {
				if((*name)[1]!='@')
					return true;
				}
			else return true;
			}
	return false;
	}

bool str_node::is_inert_command() const
	{
	if((*name).size()>1)
		if((*name)[0]=='@')
			if((*name)[1]=='@')
				return true;
	return false;
	}

bool str_node::is_name_wildcard() const
	{
	if((*name).size()>0)
		if((*name)[name->size()-1]=='?') {
			if(name->size()>1) {
				if((*name)[name->size()-2]!='?')
					return true;
				}
			else return true;
			}
	return false;
	}

bool str_node::is_object_wildcard() const
	{
	if((*name).size()>1)
		if((*name)[name->size()-1]=='?')
			if((*name)[name->size()-2]=='?')
				return true;
	return false;
	}

bool str_node::is_range_wildcard() const
	{
	if(name->size()>0) {
		if((*name)[0]=='#')
			return true;
		}
	return false;
	}

bool str_node::is_autodeclare_wildcard() const
	{
	if(name->size()>0)
		if((*name)[name->size()-1]=='#')
			return true;
	return false;
	}

bool str_node::is_indexstar_wildcard() const
	{
	if((*name).size()>1)
		if((*name)[name->size()-1]=='?')
			if((*name)[name->size()-2]=='*')
				return true;
	return false;
	}

bool str_node::is_indexplus_wildcard() const
	{
	if((*name).size()>1)
		if((*name)[name->size()-1]=='?')
			if((*name)[name->size()-2]=='+')
				return true;
	return false;
	}

bool str_node::is_numbered_symbol() const
	{
	int len=(*name).size();
	if(len>1)
		if(isdigit((*name)[len-1]))
			return true;
	return false;
	}

nset_t::iterator str_node::name_only()
	{
	if(is_name_wildcard()) {
		std::string tmp=(*name).substr(0, name->size()-1);
		return name_set.insert(tmp).first;
		}
	else if(is_object_wildcard()) {
		std::string tmp=(*name).substr(0, name->size()-2);
		return name_set.insert(tmp).first;
		}
	else if(is_autodeclare_wildcard()) {
		size_t pos=name->find('#');
		std::string tmp=(*name).substr(0, pos);
		return name_set.insert(tmp).first;
		}
	else if(is_numbered_symbol()) {
		size_t pos=name->find_first_of("0123456789");
		std::string tmp=(*name).substr(0, pos);
		return name_set.insert(tmp).first;
		}
	return name;
	}

bool str_node::operator==(const str_node& other) const
	{
	if(*name==*other.name &&
		fl.bracket==other.fl.bracket &&
		fl.parent_rel==other.fl.parent_rel &&
		multiplier==other.multiplier)
		return true;
	else return false;
	}

bool str_node::operator<(const str_node& other) const
	{
	if(*name<*other.name) return true;
	else return false;
	}

bool str_node::compare_names_only(const str_node& one, const str_node& two)
	{
	if(one.name==two.name) return true;
	else                   return false;
	}

bool str_node::compare_name_brack_par(const str_node& one, const str_node& two)
	{
	if(one.name==two.name &&
		one.fl.bracket==two.fl.bracket &&
		one.fl.parent_rel==two.fl.parent_rel) return true;
	else                                     return false;
	}

bool str_node::compare_name_inverse_par(const str_node& one, const str_node& two)
	{
	if(one.name==two.name &&
		( (one.fl.parent_rel==str_node::p_super && two.fl.parent_rel==str_node::p_sub) ||
		  (one.fl.parent_rel==str_node::p_sub && two.fl.parent_rel==str_node::p_super)))
		return true;
	return false;
	}

bool nset_it_less::operator()(nset_t::iterator first, nset_t::iterator second) const
	{
	if(*first < *second)
		return true;
	return false;
	}


void multiply(rset_t::iterator& num, multiplier_t fac) 
	{
	fac*=*num;
	num=rat_set.insert(fac).first;
	}

void add(rset_t::iterator& num, multiplier_t fac) 
	{
	fac+=*num;
	num=rat_set.insert(fac).first;
	}

void zero(rset_t::iterator& num)
	{
	num=rat_set.insert(0).first;
	}

void one(rset_t::iterator& num)
	{
	num=rat_set.insert(1).first;
	}

void flip_sign(rset_t::iterator& num)
	{
	num=rat_set.insert(-(*num)).first;
	}

void half(rset_t::iterator& num)
	{
	num=rat_set.insert((*num)/2).first;
	}

int subtree_compare(exptree::iterator one, exptree::iterator two, 
						  int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards) 
	{
	// The logic is to compare successive aspects of the two objects, returning a
	// no-match code if a difference is found at a particular level, or continuing
	// further down the line if there still is a match.
	
	// Compare multipliers. Skip this step if one of the objects is a rational and the
	// other not, as in that case we are matching values to symbols.
	if( one->is_rational()==two->is_rational() ) {
		if(compare_multiplier==-2 && !two->is_name_wildcard() && !one->is_name_wildcard())
			if(one->multiplier != two->multiplier) {
				if(*one->multiplier < *two->multiplier) return 2;
				else return -2;
				}
		}

	// First lookup some information about the index sets, if any.
	// (note: to avoid having properties::get enter here recursively, we
	// perform this check only when both objects are sub/superscripts, i.e. is_index()==true).
	// If one and two are sub/superscript, and sit in the same Indices, we keep mult=1, all
	// other cases get mult=2. 

	int  mult=1;
	if(one->is_index() && two->is_index() && one->is_rational() && two->is_rational()) mult=2;
	Indices::position_t position_type=Indices::free;
	if(one->is_index() && two->is_index()) {
		if(checksets) {
			// Strip off the parent_rel because Indices properties are declared without
			// those.
			const Indices *ind1=properties::get<Indices>(one, true);
			const Indices *ind2=properties::get<Indices>(two, true);
			if(ind1!=ind2) { 
				// It may still be that one set is a subset of the other, i.e that the
				// parent argument of Indices has been used.
				mult=2;
				// FIXME: this is required for implicit symmetry patterns on split_index objects
				//			if(ind1!=0 && ind2!=0) 
				//				if(ind1->parent_name==ind2->set_name || ind2->parent_name==ind1->set_name)
				//					mult=1;
				}
			if(ind1!=0 && ind1==ind2) 
				position_type=ind1->position_type;
			}
		}
	else mult=2;
	
	// Compare sub/superscript relations.
	if((mod_prel==-2 && position_type!=Indices::free) && one->is_index() && two->is_index() ) {
		if(one->fl.parent_rel!=two->fl.parent_rel) {
			if(one->fl.parent_rel==str_node::p_sub) return 2;
			else return -2;
			}
		}

	// Handle object wildcards and comparison
	if(!literal_wildcards) {
		if(one->is_object_wildcard() || two->is_object_wildcard())
			return 0;
		}

	// Handle mismatching node names.
	if(one->name!=two->name) {
		if(literal_wildcards) {
			if(*one->name < *two->name) return mult;
			else return -mult;
			}

		if( (one->is_autodeclare_wildcard() && two->is_numbered_symbol()) || (two->is_autodeclare_wildcard() && one->is_numbered_symbol()) ) {
			if( one->name_only() != two->name_only() ) {
				if(*one->name < *two->name) return mult;
				else return -mult;
				}
			}
		else if( one->is_name_wildcard()==false && two->is_name_wildcard()==false ) {
			if(*one->name < *two->name) return mult;
			else return -mult;
			}
		}

	// Now turn to the child nodes. Before comparing them directly, first compare
	// the number of children, taking into account range wildcards.
	int numch1=exptree::number_of_children(one);
	int numch2=exptree::number_of_children(two);

//	if(numch1>0 && one.begin()->is_range_wildcard()) {
		// FIXME: insert the code from props.cc here, ditto in the next if.
//		return 0;
//		}

//	if(numch2>0 && two.begin()->is_range_wildcard()) return 0;

	if(numch1!=numch2) {
		if(numch1<numch2) return 2;
		else return -2;
		}

	// Compare actual children.
	exptree::sibling_iterator sib1=one.begin(), sib2=two.begin();
	int remember_ret=0;
	if(mod_prel==0) mod_prel=-2;
	else if(mod_prel>0)  --mod_prel;
	if(compare_multiplier==0) compare_multiplier=-2;
	else if(compare_multiplier>0)  --compare_multiplier;

	while(sib1!=one.end()) {
		int ret=subtree_compare(sib1,sib2, mod_prel, checksets, compare_multiplier, literal_wildcards);
		if(abs(ret)>1)
			return ret/abs(ret)*mult;
		if(ret!=0 && remember_ret==0) 
			remember_ret=ret;
		++sib1;
		++sib2;
		}
	return remember_ret;
	}

bool tree_less(const exptree& one, const exptree& two, int mod_prel, bool checksets, int compare_multiplier)
	{
	return subtree_less(one.begin(), two.begin(), mod_prel, checksets, compare_multiplier);
	}

bool tree_equal(const exptree& one, const exptree& two, int mod_prel, bool checksets, int compare_multiplier)
	{
	return subtree_equal(one.begin(), two.begin(), mod_prel, checksets, compare_multiplier);
	}

bool tree_exact_less(const exptree& one, const exptree& two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	return subtree_exact_less(one.begin(), two.begin(), mod_prel, checksets, compare_multiplier, literal_wildcards);
	}

bool tree_exact_equal(const exptree& one, const exptree& two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	return subtree_exact_equal(one.begin(), two.begin(), mod_prel, checksets, compare_multiplier, literal_wildcards);
	}

bool subtree_less(exptree::iterator one, exptree::iterator two, int mod_prel, bool checksets, int compare_multiplier)
	{
	int cmp=subtree_compare(one, two, mod_prel, checksets, compare_multiplier);
	if(cmp==2) return true;
	return false;
	}

bool subtree_equal(exptree::iterator one, exptree::iterator two, int mod_prel, bool checksets, int compare_multiplier)
	{
	int cmp=subtree_compare(one, two, mod_prel, checksets, compare_multiplier);
	if(abs(cmp)<=1) return true;
	return false;
	}

bool subtree_exact_less(exptree::iterator one, exptree::iterator two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	int cmp=subtree_compare(one, two, mod_prel, checksets, compare_multiplier, literal_wildcards);
	if(cmp>0) return true;
	return false;
	}

bool subtree_exact_equal(exptree::iterator one, exptree::iterator two, int mod_prel, bool checksets, int compare_multiplier, bool literal_wildcards)
	{
	int cmp=subtree_compare(one, two, mod_prel, checksets, compare_multiplier, literal_wildcards);
	if(cmp==0) return true;
	return false;
	}

bool tree_less_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_less(one, two);
	}

bool tree_less_modprel_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_less(one, two, 0);
	}

bool tree_equal_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_equal(one, two);
	}

bool tree_exact_less_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_less(one, two);
	}

bool tree_exact_less_no_wildcards_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_less(one, two, -2, true, 0, true);
	}

bool tree_exact_less_no_wildcards_mod_prel_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_less(one, two, 0, true, -2, true);
	}

bool tree_exact_equal_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_equal(one, two);
	}

bool tree_exact_less_mod_prel_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_less(one, two, 0, true, -2, true);
	}

bool tree_exact_equal_mod_prel_obj::operator()(const exptree& one, const exptree& two) const
	{
	return tree_exact_equal(one, two, 0, true, -2, true);
	}

bool operator==(const exptree& first, const exptree& second)
	{
	return tree_exact_equal(first, second, 0, true, -2, true);
	}


void exptree_comparator::clear()
	{
	replacement_map.clear();
	subtree_replacement_map.clear();
	factor_locations.clear();
	factor_moving_signs.clear();
	}

exptree_comparator::match_t exptree_comparator::equal_subtree(exptree::iterator i1, exptree::iterator i2)
	{
	exptree::sibling_iterator i1end(i1);
	exptree::sibling_iterator i2end(i2);
	++i1end;
	++i2end;

	bool first_call=true;
	while(i1!=i1end && i2!=i2end) {
		match_t mm=compare(i1,i2,first_call);
		first_call=false;
		switch(mm) {
			case no_match_less:
			case no_match_greater:
				return mm;
			case node_match: {
				size_t num1=exptree::number_of_children(i1);
				size_t num2=exptree::number_of_children(i2);
				if(num1 < num2)      return no_match_less;
				else if(num1 > num2) return no_match_greater;
				break;
				}
			case subtree_match:
				i1.skip_children();
				i2.skip_children();
				break;
			}
		++i1;
		++i2;
		}

	return subtree_match;
	}

exptree_comparator::match_t exptree_comparator::compare(const exptree::iterator& one, 
																		  const exptree::iterator& two, 
																		  bool nobrackets) 
	{
	// nobrackets also implies 'no multiplier', i.e. 'toplevel'.
	// one is the substitute pattern, two the expression under consideration
	
//	std::cerr << "matching " << *one->name << " to " << *two->name << std::endl;

	if(nobrackets==false && one->fl.bracket != two->fl.bracket) 
		return (one->fl.bracket < two->fl.bracket)?no_match_less:no_match_greater;

//	std::cerr << "one passed" << std::endl;

	// FIXME: this needs to be relaxed for position-free indices
	if(one->fl.parent_rel != two->fl.parent_rel)                
		return (one->fl.parent_rel < two->fl.parent_rel)?no_match_less:no_match_greater;

//	std::cerr << "two passed" << std::endl;

	// Determine whether we are dealing with one of the pattern types.
	bool pattern=false;
	bool objectpattern=false;
	bool implicit_pattern=false;
	bool is_index=false;
	
	if(one->fl.bracket==str_node::b_none && one->is_index() ) 
		is_index=true;
	if(one->is_name_wildcard())
		pattern=true;
	else if(one->is_object_wildcard())
		objectpattern=true;
	else if(is_index && one->is_integer()==false) {
		const Coordinate *cdn1=properties::get<Coordinate>(one, true);
		if(cdn1==0)
			implicit_pattern=true;
		}
		
	if(pattern || (implicit_pattern && two->is_integer()==false)) { 
		// The above is to ensure that we never match integers to implicit patterns.

		// We want to search the replacement map for replacement rules which we have
		// constructed earlier, and discard the current match if it conflicts those 
		// rules. This is to make sure that e.g. a pattern k1_a k2_a does not match 
		// an expression k1_c k2_d.
		// 
		// In order to ensure that a replacement rule for a lower index is also
		// triggering a rule for an upper index, we simply store both rules (see
		// below) so that searching for rules can remain simple.

		replacement_map_t::iterator loc=replacement_map.find(one);

		bool tested_full=true;

		// If this is a pattern with a non-zero number of children, 
		// also search the pattern without the children.
		if(loc == replacement_map.end() && exptree::number_of_children(one)!=0) {
			exptree tmp1(one);
			tmp1.erase_children(tmp1.begin());
			loc = replacement_map.find(tmp1);
			tested_full=false;
			}

		if(loc!=replacement_map.end()) {
//			std::cerr << "found!" << std::endl;
			// If this is an index/pattern, try to match the whole index/pattern.
			int cmp;

			if(tested_full) 
				cmp=subtree_compare((*loc).second.begin(), two, -2 /* KP: do not switch this to -2 (kk.cdb fails) */); 
			else {
				exptree tmp2(two);
				tmp2.erase_children(tmp2.begin());
				cmp=subtree_compare((*loc).second.begin(), tmp2.begin(), -2 /* KP: see above */); 
				}
//			std::cerr << " pattern " << *two->name
//						 << " should be " << *((*loc).second.begin()->name)  
//						 << " because that's what " << *one->name 
//						 << " was set to previously; result " << cmp << std::endl;

			if(cmp==0)      return subtree_match;
			else if(cmp>0)  return no_match_less;
			else            return no_match_greater;
			}
		else {
			// This index/pattern was not encountered earlier. Check that the index types in pattern
			// and object agree (if known, otherwise assume they match)

//			std::cerr << "index check " << *one->name << " " << *two->name << std::endl;

			const Indices *t1=properties::get<Indices>(one, true);
			const Indices *t2=properties::get<Indices>(two, true);
//			std::cerr << t1 << " " << t2 << std::endl;
			if( (t1 || t2) && implicit_pattern ) {
				if(t1 && t2) {
					if((*t1).set_name != (*t2).set_name) {
						if((*t1).set_name < (*t2).set_name) return no_match_less;
						else                                return no_match_greater;
						}
					}
				else {
					if(t1) return no_match_less;
					else   return no_match_greater;
					}
				}
			// The index types match, so register this replacement rule.
//			std::cerr << "registering ";
//			if(one->fl.parent_rel==str_node::p_super) std::cerr << "^";
//			if(one->fl.parent_rel==str_node::p_sub)   std::cerr << "_";
//			std::cerr << *one->name << " ";
//			if(two->fl.parent_rel==str_node::p_super) std::cerr << "^";
//			if(two->fl.parent_rel==str_node::p_sub)   std::cerr << "_";
//			std::cerr << *two->name << std::endl;

			replacement_map[one]=two;
			
			// if this is an index, also store the pattern with the parent_rel flipped
			if(one->is_index()) {
				exptree cmptree1(one);
				exptree cmptree2(two);
				cmptree1.begin()->flip_parent_rel();
				if(two->is_index())
					cmptree2.begin()->flip_parent_rel();
				replacement_map[cmptree1]=cmptree2;
				}
			
			// if this is a pattern and the pattern has a non-zero number of children,
			// also add the pattern without the children
			if(exptree::number_of_children(one)!=0) {
				exptree tmp1(one), tmp2(two);
				tmp1.erase_children(tmp1.begin());
				tmp2.erase_children(tmp2.begin());
				replacement_map[tmp1]=tmp2;
				}
			// and if this is a pattern also insert the one without the parent_rel
			if(one->is_name_wildcard()) {
				exptree tmp1(one), tmp2(two);
				tmp1.begin()->fl.parent_rel=str_node::p_none;
				tmp2.begin()->fl.parent_rel=str_node::p_none;
				replacement_map[tmp1]=tmp2;
				}
			}
		
		// Return a match of the appropriate type
		if(is_index) return subtree_match;
		else         return node_match;
		}
	else if(objectpattern) {
		subtree_replacement_map_t::iterator loc=subtree_replacement_map.find(one->name);
		if(loc!=subtree_replacement_map.end()) {
			return equal_subtree((*loc).second,two);
			}
		else subtree_replacement_map[one->name]=two;
		
		return subtree_match;
		}
	else { // object is not dummy
		if(one->is_rational() && two->is_rational() && one->multiplier!=two->multiplier) {
			if(*one->multiplier < *two->multiplier) return no_match_less;
			else                                    return no_match_greater;
			}
		
		if(one->name==two->name) {
			if(nobrackets || (one->multiplier == two->multiplier) ) 
				return node_match;

			if(*one->multiplier < *two->multiplier) return no_match_less;
			else                                    return no_match_greater;
			}
		else {
			if( *one->name < *two->name ) return no_match_less;
			else                          return no_match_greater;
			}
		}
	
	assert(1==0); // should never be reached

	return no_match_less; 
	}


// Find a subproduct in a product. The 'lhs' iterator points to the product which
// we want to find, the 'tofind' iterator to the current factor which we are looking
// for. The product in which to search is pointed to by 'st'.
//
// Once 'tofind' is found, this routine calls itself to find the next factor in
// 'lhs'. If the next factor cannot be found, we backtrack and try to find the
// previous factor again (it may have appeared multiple times).
//
exptree_comparator::match_t exptree_comparator::match_subproduct(exptree::sibling_iterator lhs, 
																					  exptree::sibling_iterator tofind, 
																					  exptree::sibling_iterator st)
	{
	replacement_map_t         backup_replacements(replacement_map);
	subtree_replacement_map_t backup_subtree_replacements(subtree_replacement_map);

	exptree::sibling_iterator start=st.begin();
	while(start!=st.end()) {
		if(std::find(factor_locations.begin(), factor_locations.end(), start)==factor_locations.end()) {  
			if(equal_subtree(tofind, start)==subtree_match) { // found factor
				// If a previous factor was found, verify that the factor found now can be
				// moved next to the previous factor (nontrivial if factors do not commute).
				int sign=1;
				if(factor_locations.size()>0) {
					sign=exptree_ordering::can_move_adjacent(st, factor_locations.back(), start);
					}
				if(sign==0) { // object found, but we cannot move it in the right order
					replacement_map=backup_replacements;
					subtree_replacement_map=backup_subtree_replacements;
					}
				else {
					factor_locations.push_back(start);
					factor_moving_signs.push_back(sign);
					
					exptree::sibling_iterator nxt=tofind; 
					++nxt;
					if(nxt!=lhs.end()) {
						match_t res=match_subproduct(lhs, nxt, st);
						if(res==subtree_match) return res;
						else {
//						txtout << tofind.node << "found factor useless " << start.node << std::endl;
							factor_locations.pop_back();
							factor_moving_signs.pop_back();
							replacement_map=backup_replacements;
							subtree_replacement_map=backup_subtree_replacements;
							}
						}
					else return subtree_match;
					}
				}
			else {
//				txtout << tofind.node << "does not match" << std::endl;
				replacement_map=backup_replacements;
				subtree_replacement_map=backup_subtree_replacements;
				}
			}
		++start;
		}
	return no_match_less; // FIXME not entirely true
	}


// Determine whether the two objects can be moved next to each other,
// with 'one' to the left of 'two'. Return the sign, or zero.
//
int exptree_ordering::can_move_adjacent(exptree::iterator prod,
													 exptree::sibling_iterator one, exptree::sibling_iterator two) 
	{
	assert(exptree::parent(one)==exptree::parent(two));
	assert(exptree::parent(one)==prod);

	// Make sure that 'one' points to the object which occurs first in 'prod'.
	bool onefirst=false;
	exptree::sibling_iterator probe=one;
	while(probe!=prod.end()) {
		if(probe==two) {
			onefirst=true;
			break;
			}
		++probe;
		}
	int sign=1;
	if(!onefirst) {
		std::swap(one,two);
		int es=subtree_compare(one,two);
		sign*=can_swap(one,two,es);
//		txtout << "swapping one and two: " << sign << std::endl;
		}

	if(sign!=0) {
		// Loop over all pair flips which are necessary to move one to the left of two.
		probe=one;
		++probe;
		while(probe!=two) {
			assert(probe!=prod.end());
			int es=subtree_compare(one,probe);
			sign*=can_swap(one,probe,es);
			if(sign==0) break;
			++probe;
			}
		}
	return sign;
	}



// Should obj and obj+1 be swapped, according to the SortOrder
// properties?
//
bool exptree_ordering::should_swap(exptree::iterator obj, int subtree_comparison) 
	{
	exptree::sibling_iterator one=obj, two=obj;
	++two;

	// Find a SortOrder property which contains both one and two.
	int num1, num2;
	const SortOrder *so1=properties::get_composite<SortOrder>(one,num1);
	const SortOrder *so2=properties::get_composite<SortOrder>(two,num2);

//	std::cerr << so1 << " " << so2 << " " << subtree_comparison << std::endl;

	if(so1==0 || so2==0) { // No sort order known
		if(subtree_comparison<0) return true;
		return false;
		}
	else if(abs(subtree_comparison)<=1) { // Identical up to index names
		if(subtree_comparison==-1) return true;
		return false;
		}
	else {
//		std::cerr << num1 << " " << num2 << std::endl;
		if(so1==so2) {
			if(num1>num2) return true;
			return false;
			}
		}

	return false;
	}

// Various tests about whether two non-elementary objects can be swapped.
//
int exptree_ordering::can_swap_prod_obj(exptree::iterator prod, exptree::iterator obj, 
													 bool ignore_implicit_indices) 
	{
//	std::cout << "prod_obj " << *prod->name << " " << *obj->name << std::endl;
	// Warning: no check is made that prod is actually a product!
	int sign=1;
	exptree::sibling_iterator sib=prod.begin();
	while(sib!=prod.end()) {
		const Indices *ind1=properties::get_composite<Indices>(sib, true);
		const Indices *ind2=properties::get_composite<Indices>(obj, true);
		if(! (ind1!=0 && ind2!=0) ) { // If both objects are actually real indices, 
			                           // then we do not include their commutativity property
			                           // in the sign. This is because the routines that use
                                    // can_swap_prod_obj all test for such index-index 
                                    // swaps separately.
			int es=subtree_compare(sib, obj, 0);
//			std::cout << "  " << *sib->name << " " << *obj->name << " " << es << std::endl;
			sign*=can_swap(sib, obj, es, ignore_implicit_indices);
			if(sign==0) break;
			}
		++sib;
		}
	return sign;
	}

int exptree_ordering::can_swap_prod_prod(exptree::iterator prod1, exptree::iterator prod2, 
													 bool ignore_implicit_indices)  
	{
//	std::cout << "prod_prod " << *prod1->name << " " << *prod2->name;
	// Warning: no check is made that prod1,2 are actually products!
	int sign=1;
	exptree::sibling_iterator sib=prod2.begin();
	while(sib!=prod2.end()) {
		sign*=can_swap_prod_obj(prod1, sib, ignore_implicit_indices);
		if(sign==0) break;
		++sib;
		}
//	std::cout << "  -> " << sign << std::endl;
	return sign;
	}

int exptree_ordering::can_swap_sum_obj(exptree::iterator sum, exptree::iterator obj, 
													bool ignore_implicit_indices) 
	{
	// Warning: no check is made that sum is actually a sum!
	int sofar=2;
	exptree::sibling_iterator sib=sum.begin();
	while(sib!=sum.end()) {
		int es=subtree_compare(sib, obj);
		int thissign=can_swap(sib, obj, es, ignore_implicit_indices);
		if(sofar==2) sofar=thissign;
		else if(thissign!=sofar) {
			sofar=0;
			break;
			}
		++sib;
		}
	return sofar;
	}

int exptree_ordering::can_swap_prod_sum(exptree::iterator prod, exptree::iterator sum, 
													 bool ignore_implicit_indices) 
	{
	// Warning: no check is made that sum is actually a sum or prod is a prod!
	int sign=1;
	exptree::sibling_iterator sib=prod.begin();
	while(sib!=prod.end()) {
//		const Indices *ind=properties::get_composite<Indices>(sib);
//		if(ind==0) {
		sign*=can_swap_sum_obj(sum, sib, ignore_implicit_indices);
			if(sign==0) break;
//			}
		++sib;
		}
	return sign;
	}

int exptree_ordering::can_swap_sum_sum(exptree::iterator sum1, exptree::iterator sum2,
													bool ignore_implicit_indices) 
	{
	int sofar=2;
	exptree::sibling_iterator sib=sum1.begin();
	while(sib!=sum1.end()) {
		int thissign=can_swap_sum_obj(sum2, sib, ignore_implicit_indices);
		if(sofar==2) sofar=thissign;
		else if(thissign!=sofar) {
			sofar=0;
			break;
			}
		++sib;
		}
	return sofar;
	}

int exptree_ordering::can_swap_ilist_ilist(exptree::iterator obj1, exptree::iterator obj2) 
	{
	int sign=1;

	exptree::index_iterator it1=exptree::begin_index(obj1);
	while(it1!=exptree::end_index(obj1)) {
		exptree::index_iterator it2=exptree::begin_index(obj2);
		while(it2!=exptree::end_index(obj2)) {
			// Only deal with real indices here, i.e. those carrying an Indices property.
			const Indices *ind1=properties::get_composite<Indices>(it1, true);
			const Indices *ind2=properties::get_composite<Indices>(it2, true);
			if(ind1!=0 && ind2!=0) {
				const CommutingBehaviour *com1 =properties::get_composite<CommutingBehaviour>(it1, true);
				const CommutingBehaviour *com2 =properties::get_composite<CommutingBehaviour>(it2, true);
				
				if(com1!=0  &&  com1 == com2) 
					sign *= com1->sign();
				
				if(sign==0) break;
				}
			++it2;
			}
		if(sign==0) break;
		++it1;
		}

	return sign;
	}

// Can obj and obj+1 be exchanged? If yes, return the sign,
// if no return zero. This is the general entry point for 
// two arbitrary nodes (which may be a product or sum). 
// Do not call the functions above directly!
//
// The last flag ('ignore_implicit_indices') is used to disable 
// all checks dealing with implicit indices (this is useful for
// algorithms which re-order objects with implicit indices, which would
// otherwise always receive a 0 from this function).
//
int exptree_ordering::can_swap(exptree::iterator one, exptree::iterator two, int subtree_comparison,
										 bool ignore_implicit_indices) 
	{
//	std::cout << "can_swap " << *one->name << " " << *two->name << ignore_implicit_indices << std::endl;

	const ImplicitIndex *ii1 = properties::get_composite<ImplicitIndex>(one);
	const ImplicitIndex *ii2 = properties::get_composite<ImplicitIndex>(two);

	// When both objects carry an implicit index but the index lines are not connected,
	// we should not be using explicit commutation rules, as this would mess up the
	// index lines and make the expression meaningless.
	// FIXME: this would ideally make use of index and conjugate index lines.

	const DiracBar *db2 = properties::get_composite<DiracBar>(two);
	if(! (ii1 && ii2 && db2) ) {

		// First of all, check whether there is an explicit declaration for the commutativity 
		// of these two symbols.
//		std::cout << *one->name << " explicit " << *two->name << std::endl;
		const CommutingBehaviour *com = properties::get_composite<CommutingBehaviour>(one, two, true);
		
		if(com) {
//			std::cout << "explicit " << com->sign() << std::endl;
			return com->sign();
			}
		}
	
	if(ignore_implicit_indices==false) {
		// Two implicit-index objects cannot move through each other if they have the
		// same type of implicit index.
//		std::cout << "can_swap " << *one->name << " " << *two->name << std::endl;

		if(ii1 && ii2) {
			if(ii1->set_names.size()==0 && ii2->set_names.size()==0) return 0; // empty index name
			for(size_t n1=0; n1<ii1->set_names.size(); ++n1)
				for(size_t n2=0; n2<ii2->set_names.size(); ++n2)
					if(ii1->set_names[n1]==ii2->set_names[n2])
						return 0;
			}
		}

	// Do we need to use Self* properties?
	const SelfCommutingBehaviour *sc1 =properties::get_composite<SelfCommutingBehaviour>(one, true);
	const SelfCommutingBehaviour *sc2 =properties::get_composite<SelfCommutingBehaviour>(two, true);
	if( (sc1!=0 && sc1==sc2) ) 
		return sc1->sign();

	// One or both of the objects are not in an explicit list. So now comes the generic
	// part. The first step is to look at all explicit indices of the two objects and determine 
	// their commutativity. 
	// Note: this does not yet look at arguments (non-index children).

	int tmpsign=can_swap_ilist_ilist(one, two);
	if(tmpsign==0) return 0;
	
	// The second step is to check for product-like and sum-like behaviour. The following
	// take into account all commutativity properties of explict with implicit indices,
	// as well as hard-specified commutativity of factors.

	const CommutingAsProduct *comap1 = properties::get_composite<CommutingAsProduct>(one);
	const CommutingAsProduct *comap2 = properties::get_composite<CommutingAsProduct>(two);
	const CommutingAsSum     *comas1 = properties::get_composite<CommutingAsSum>(one);
	const CommutingAsSum     *comas2 = properties::get_composite<CommutingAsSum>(two);
	
	if(comap1 && comap2) return tmpsign*can_swap_prod_prod(one,two,ignore_implicit_indices);
	if(comap1 && comas2) return tmpsign*can_swap_prod_sum(one,two,ignore_implicit_indices);
	if(comap2 && comas1) return tmpsign*can_swap_prod_sum(two,one,ignore_implicit_indices);
	if(comas1 && comas2) return tmpsign*can_swap_sum_sum(one,two,ignore_implicit_indices);
	if(comap1)           return tmpsign*can_swap_prod_obj(one,two,ignore_implicit_indices);
	if(comap2)           return tmpsign*can_swap_prod_obj(two,one,ignore_implicit_indices);
	if(comas1)           return tmpsign*can_swap_sum_obj(one,two,ignore_implicit_indices);
	if(comas2)           return tmpsign*can_swap_sum_obj(two,one,ignore_implicit_indices);
	
	return 1; // default: commuting.
	}

bool exptree_comparator::satisfies_conditions(exptree::iterator conditions, std::string& error) 
	{
	for(unsigned int i=0; i<exptree::arg_size(conditions); ++i) {
		exptree::iterator cond=exptree::arg(conditions, i);
		if(*cond->name=="\\unequals") {
			exptree::sibling_iterator lhs=cond.begin();
			exptree::sibling_iterator rhs=lhs;
			++rhs;
			// Lookup the replacement rules for the two given objects, and return true if 
			// those rules give a different result. But first check that there are rules
			// to start with.
//			std::cerr << *lhs->name  << " !=? " << *rhs->name << std::endl;
			if(replacement_map.find(exptree(lhs))==replacement_map.end() ||
				replacement_map.find(exptree(rhs))==replacement_map.end()) return true;
//			std::cerr << *lhs->name  << " !=?? " << *rhs->name << std::endl;
			if(tree_exact_equal(replacement_map[exptree(lhs)], replacement_map[exptree(rhs)])) {
				return false;
				}
			}
		else if(*cond->name=="\\indexpairs") {
			int countpairs=0;
			replacement_map_t::const_iterator it=replacement_map.begin(),it2;
			while(it!=replacement_map.end()) {
				it2=it;
				++it2;
				while(it2!=replacement_map.end()) {
					if(tree_exact_equal(it->second, it2->second)) {
						++countpairs;
						break;
						}
					++it2;
					}
				++it;
				}
//			txtout << countpairs << " pairs" << std::endl;
			if(countpairs!=*(cond.begin()->multiplier))
				return false;
			}
		else if(*cond->name=="\\regex") {
//			txtout << "regex matching..." << std::endl;
			exptree::sibling_iterator lhs=cond.begin();
			exptree::sibling_iterator rhs=lhs;
			++rhs;
			// If we have a match, all indices have replacement rules.
			std::string pat=(*rhs->name).substr(1,(*rhs->name).size()-2);
//			txtout << "matching " << *comp.replacement_map[lhs->name]
//					 << " with pattern " << pat << std::endl;
			pcrecpp::RE reg(pat);
			if(reg.FullMatch(*(replacement_map[exptree(lhs)].begin()->name))==false)
				return false;
			}
		else if(*cond->name=="\\hasprop") {
			exptree::sibling_iterator lhs=cond.begin();
			exptree::sibling_iterator rhs=lhs;
			++rhs;
			properties::registered_property_map_t::iterator pit=
				properties::registered_properties.store.find(*rhs->name);
			if(pit==properties::registered_properties.store.end()) {
				std::ostringstream str;
				str << "Property \"" << *rhs->name << "\" not registered." << std::endl;
				error=str.str();
				return false;
				}
			const property_base *aprop=pit->second();

			subtree_replacement_map_t::iterator subfind=subtree_replacement_map.find(lhs->name);
			replacement_map_t::iterator         patfind=replacement_map.find(exptree(lhs));

			if(subfind==subtree_replacement_map.end() && patfind==replacement_map.end()) {
				std::ostringstream str;
				str << "Pattern " << *lhs->name << " in \\hasprop did not occur in match." << std::endl;
				delete aprop;
				error=str.str();
				return false;
				}
			
			bool ret=false;
			if(subfind==subtree_replacement_map.end()) 
				 ret=properties::has(aprop, (*patfind).second.begin());
			else
				 ret=properties::has(aprop, (*subfind).second);
			delete aprop;
			return ret;
			}
		else {
			std::ostringstream str;
			str << "substitute: condition involving " << *cond->name << " not understood." << std::endl;
			error=str.str();
			return false;
			}
		}
	return true;
	}

bool exptree_is_equivalent::operator()(const exptree& one, const exptree& two)
	{
	int ret=subtree_compare(one.begin(), two.begin());
	if(ret==0) return true;
	else       return false;
	}

bool exptree_is_less::operator()(const exptree& one, const exptree& two)
	{
	int ret=subtree_compare(one.begin(), two.begin());
	if(ret < 0) return true;
	else        return false;
	}

