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

//exptree::sibling_iterator exptree::tensor_index(const iterator_base& position, unsigned int num) const
//	{
//	index_iterator ret=begin_index(position);
//	while(num-- > 0)
//		++ret;
//	return ret;

//	const Derivative *der=properties::get<Derivative>(position);
//	if(der) {
//		unsigned int tensor_children=number_of_children(begin(position));
//		if(num<tensor_children) return child(begin(position), num);
//		else                    return child(position, num+1-tensor_children);
//		}
//	else return child(position, num);
//	}

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

bool exptree::operator==(const exptree& other) const
	{
	return equal_subtree(begin(), other.begin());
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

bool str_node::is_siblings_wildcard() const
	{
	if(name->size()>0) {
		if((*name)[name->size()-1]=='@')
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



