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
#include "Exceptions.hh"
#include <iomanip>
#include <sstream>
#include <pcrecpp.h>


namespace cadabra {

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

// Expression constructor/destructor members.

Ex::Ex()
	: tree<str_node>(), state_(result_t::l_no_action)
	{
	}

Ex::Ex(tree<str_node>::iterator it)
	: tree<str_node>(it), state_(result_t::l_no_action)
	{
	}

Ex::Ex(const str_node& x)
	: tree<str_node>(x), state_(result_t::l_no_action)
	{
	}

Ex::Ex(const Ex& other)
	: tree<str_node>(other), state_(result_t::l_no_action)
	{
   //	std::cout << "Ex copy constructor" << std::endl;
	}

Ex::Ex(const std::string& str) 
	: state_(result_t::l_no_action)
	{
	set_head(str_node(str));
	}

Ex::Ex(int val) 
	: state_(result_t::l_no_action)
	{
	set_head(str_node("1"));
	multiply(begin()->multiplier, val);
	}

Ex::result_t Ex::state() const
	{
	return state_;
	}

void Ex::update_state(Ex::result_t newstate)
	{
	switch(newstate) {
		case Ex::result_t::l_error:
			state_=newstate;
			break;
		case Ex::result_t::l_applied:
			if(state_!=Ex::result_t::l_error)
				state_=newstate;
			break;
		default:
			break;
		}
	}

void Ex::reset_state() 
	{
	state_=Ex::result_t::l_checkpointed;
	}

bool Ex::changed_state()
	{
	bool ret=false;
	if(state_==result_t::l_checkpointed || state_==result_t::l_applied) ret=true;
	state_=result_t::l_no_action;
	return ret;
	}

bool Ex::is_rational() const
	{
	if(begin()!=end())
		if(begin()->is_rational())
			return true;
	return false;
	}

multiplier_t Ex::to_rational() const
	{
	if(!is_rational())
		throw InternalError("Called to_rational() on non-rational Ex");
	return *(begin()->multiplier);
	}

std::ostream& Ex::print_python(std::ostream& str, Ex::iterator it)
	{
	std::string name(*(*it).name);
	std::string res;
	if(*it->multiplier!=1)
		str << *it->multiplier;

	for(unsigned int i=0; i<name.size(); ++i) {
		if(name[i]=='#')       res+="\\#";
//		else if(name[i]=='\\') res+="\\backslash{}";
		else                   res+=name[i];
		}
	str << res;
	
	Ex::sibling_iterator beg=it.begin();
	Ex::sibling_iterator fin=it.end();
	str_node::bracket_t    current_bracket=str_node::b_invalid;
	str_node::parent_rel_t current_parent_rel=str_node::p_invalid;
	
	while(beg!=fin) {
		if(beg==it.begin() || current_parent_rel!=(*beg).fl.parent_rel) {
			switch((*beg).fl.parent_rel) {
				case str_node::p_super: str << "^"; break;
				case str_node::p_sub:   str << "_"; break;
				case str_node::p_property: str << "$"; break;
				case str_node::p_exponent: str << "&"; break;
				default: break;
				}
			current_parent_rel=(*beg).fl.parent_rel;
			}
		if(beg==it.begin() || current_bracket!=(*beg).fl.bracket || current_parent_rel!=(*beg).fl.parent_rel) {
			switch((*beg).fl.bracket) {
				case str_node::b_round:       str << "("; break;
				case str_node::b_square:      str << "["; break;
				case str_node::b_curly:       str << "{"; break;		
				case str_node::b_pointy:      str << "<"; break;
				case str_node::b_none: {
					if((*beg).fl.parent_rel==str_node::p_none) str << "("; 
					else                                       str << "{"; 
					}
					break;
				default: break;					
				}
			current_bracket=(*beg).fl.bracket;
			}
		
		print_python(str, beg);

		auto nxt=beg;
		++nxt;
		if(nxt!=fin) {
			if((*beg).fl.parent_rel!=str_node::p_none)
				str << " ";
			}
		if(nxt==fin || (*nxt).fl.bracket!=(*beg).fl.bracket || (*beg).fl.parent_rel==str_node::p_none) {
			current_bracket=str_node::b_invalid;
			current_parent_rel=str_node::p_invalid;
			switch((*beg).fl.bracket) {
				case str_node::b_round:       str << ")"; break;
				case str_node::b_square:      str << "]"; break;
				case str_node::b_curly:       str << "}";	break;			
				case str_node::b_pointy:      str << ">"; break;
				case str_node::b_none: {
					if((*beg).fl.parent_rel==str_node::p_none) str << ")"; 
					else                                       str << "}"; 
					}
					break;
				default: break;					
				}
			}
		++beg;
		}
	
	return str;
	}

std::ostream& Ex::print_repr(std::ostream& str, Ex::iterator it) const
	{
	str << *it->name;
	sibling_iterator sib=it.begin();
	while(sib!=it.end()) {
		print_repr(str, sib);
		++sib;
		}
	return str;
	}
	
std::ostream& Ex::print_recursive_treeform(std::ostream& str, Ex::iterator it) 
	{
	unsigned int num=1;
	switch((*it).fl.parent_rel) {
		case str_node::p_super: str << "^"; break;
		case str_node::p_sub:   str << "_"; break;
		case str_node::p_property: str << "$"; break;
		case str_node::p_exponent: str << "&"; break;
		default: break;
		}
	return print_recursive_treeform(str, it, num);
	}

std::ostream& Ex::print_entire_tree(std::ostream& str) const
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

std::ostream& Ex::print_recursive_treeform(std::ostream& str, Ex::iterator it, unsigned int& num)
	{
	bool compact_tree=getenv("CDB_COMPACTTREE");

	Ex::sibling_iterator beg=it.begin();
	Ex::sibling_iterator fin=it.end();

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
	str << "  (" << it->fl.bracket << " " << &(*it) << ")";
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


// std::ostream& operator<<(std::ostream& str, const Ex& tr)
// 	{
// 	unsigned int number=1;
// 	Ex::iterator it=tr.begin();
// 	while(it!=tr.end()) {
// 		tr.print_recursive_infix(str, it, number, true);
// 		it.skip_children();
// 		++it;
// 		if(it!=tr.end())
// 			str << std::endl;
// 		}
// 	return str;
// 	}

Ex::iterator Ex::named_parent(Ex::iterator it, const std::string& nm) const
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

Ex::iterator Ex::erase_expression(Ex::iterator it) 
	{
	it=named_parent(it, "\\history");
	return erase(it);
	}


hashval_t Ex::calc_hash(iterator it) const
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

Ex::sibling_iterator Ex::arg(iterator it, unsigned int num) 
	{
	if(*it->name=="\\comma") {
		assert(Ex::number_of_children(it)>num);
		return Ex::child(it,num);
		}
	else return it;
	}

unsigned int Ex::arg_size(sibling_iterator sib) 
	{
	if(*sib->name=="\\comma") return Ex::number_of_children(sib);
	else return 1;
	}

multiplier_t Ex::arg_to_num(sibling_iterator sib, unsigned int num) const
	{
	sibling_iterator nod;
	if(*sib->name=="\\comma") nod=child(sib,num);
	else                      nod=sib;
	return *nod->multiplier;
	}

unsigned int Ex::equation_number(Ex::iterator it) const
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

nset_t::iterator Ex::equation_label(Ex::iterator it) const
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
Ex::iterator Ex::equation_by_number(unsigned int i) const
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

Ex::iterator Ex::equation_by_name(nset_t::iterator nit) const
	{
	unsigned int tmp;
	return equation_by_name(nit, tmp);
	}

Ex::iterator Ex::equation_by_name(nset_t::iterator nit, unsigned int& tmp) const
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

Ex::iterator Ex::procedure_by_name(nset_t::iterator nit) const
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

Ex::iterator Ex::replace_index(iterator pos, const iterator& from, bool keep_parent_rel)
	{
//	assert(pos->fl.parent_rel==str_node::p_sub || pos->fl.parent_rel==str_node::p_super);
	str_node::bracket_t    bt=pos->fl.bracket;
	str_node::parent_rel_t pr=pos->fl.parent_rel;
	iterator ret=replace(pos, from);
	ret->fl.bracket=bt;
	if(keep_parent_rel)
		ret->fl.parent_rel=pr;
	return ret;
	}

Ex::iterator Ex::move_index(iterator pos, const iterator& from)
	{
//	assert(pos->fl.parent_rel==str_node::p_sub || pos->fl.parent_rel==str_node::p_super);
	str_node::bracket_t    bt=pos->fl.bracket;
	str_node::parent_rel_t pr=pos->fl.parent_rel;
	move_ontop(pos, from);
	from->fl.bracket=bt;
	from->fl.parent_rel=pr;
	return from;
	}

void Ex::list_wrap_single_element(iterator& it)
	{
	if(*it->name!="\\comma") {
		iterator commanode=insert(it, str_node("\\comma"));
		sibling_iterator fr=it, to=it;
		++to;
		reparent(commanode, fr, to);
		it=commanode;
		}
	}

void Ex::list_unwrap_single_element(iterator& it)
	{
	if(*it->name=="\\comma") {
		if(number_of_children(it)==1) {
			flatten(it);
			it=erase(it);
			}
		}
	}

Ex::iterator Ex::flatten_and_erase(iterator pos)
	{
//	assert(number_of_children(pos)==1);

	multiplier_t tmp=*pos->multiplier;
	flatten(pos);
	pos=erase(pos);
	multiply(pos->multiplier, tmp);
	return pos;
	}

unsigned int Ex::number_of_equations() const
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

Ex::iterator Ex::equation_by_number_or_name(iterator it, unsigned int last_used_equation, 
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

Ex::iterator Ex::equation_by_number_or_name(iterator it, unsigned int last_used_equation) const
	{
	unsigned int tmp;
	return equation_by_number_or_name(it, last_used_equation, tmp);
	}

std::string Ex::equation_number_or_name(iterator it, unsigned int last_used_equation) const
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

bool Ex::operator==(const Ex& other) const
	{
	return equal_subtree(begin(), other.begin());
	}

void Ex::push_history(Ex selector)
	{
	history.push_back(*this);
	selectors.push_back(selector);
	}

Ex Ex::pop_history()
	{
	tree<str_node>::operator=(history.back());
	history.pop_back();
	Ex ret(selectors.back().begin());
	selectors.pop_back();
	return ret;
	}

int Ex::history_size() const
	{
	return history.size();
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

bool str_node::operator<(const cadabra::str_node& other) const
	{
	if(*name<*other.name) return true;
	else return false;
	}

}	

// Keep operator overloading outside of the cadabra namespace.

std::ostream& operator<<(std::ostream& str, const cadabra::Ex& ex) 
	{
	if(ex.begin()==ex.end()) return str;
	ex.print_recursive_treeform(str, ex.begin());
//	ex.print_python(str, ex.begin());	
	return str;
	}

std::ostream& operator<<(std::ostream& str, cadabra::Ex::iterator it) 
	{
	cadabra::Ex::print_recursive_treeform(str, it);
//	ex.print_python(str, ex.begin());	
	return str;
	}

