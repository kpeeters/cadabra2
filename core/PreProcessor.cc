/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2015  Kasper Peeters <cadabra@phi-sci.com>

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

/*
	
   Bugs: - This code is ugly. Then again, this is only because human
			  beings cannot be bothered to type in prefix notation. Since
			  it ain't broke, I won't fix it. Should really be replaced by
			  a parser written using ANTLR or Bisonc++ or something like that.

   Remember: we want brackets around infix operators to be around every child,
	          not around the operator itself. 

*/

#include <ctype.h>
#include <stdexcept>
#include <sstream>
#include "PreProcessor.hh"

const unsigned char preprocessor::orders[]     ={ '!', tok_pow, '/', '*', tok_wedge, 
																  '-', '+', tok_sequence, '=', tok_unequals, 
																  '<', '>', '|', tok_arrow, tok_set_option, 
																  tok_declare, ',', '~', 0 };
const char * const  preprocessor::order_names[]={ "\\factorial", 
																  "\\pow", "\\frac", "\\prod", "\\wedge", 
																  "\\sub", "\\sum", "\\sequence", 
																  "\\equals", "\\unequals", "\\less",
																  "\\greater", "\\conditional", 
																  "\\arrow", "\\setoption", "\\declare", 
																  "\\comma", "\\tie", 0 };
const unsigned char preprocessor::open_brackets[]    ={ '{', '(', '[' };
const unsigned char preprocessor::close_brackets[]   ={ '}', ')', ']' };

std::istream& operator>>(std::istream& s, preprocessor& p)
	{
	std::string inp;
	while(std::getline(s,inp)) {
		p.parse_(inp);
		}
	return s;
	}

std::ostream& operator<<(std::ostream& s, const preprocessor& p)
	{
	while(p.accus.size()>0)
		p.unwind_(sizeof(preprocessor::orders));
	p.unwind_(sizeof(preprocessor::orders));
	p.strip_outer_brackets();
	s << p.cur.accu;
	return s;
	}

preprocessor::preprocessor()
	: verbatim_(false), next_is_product_(false), eat_initial_whitespace_(true), cur_pos(0)
	{
	}

bool preprocessor::is_link_(unsigned char c) const
	{
	return (c=='_' || c=='^' || c=='$' || c=='&'); 
	}

bool preprocessor::is_infix_operator_(unsigned char c) const
	{
	for(unsigned int i=0; i<sizeof(orders); ++i)
		if(orders[i]==c) return true;
	return false;
	}

void preprocessor::print_stack() const
	{
	for(unsigned int i=0; i<accus.size(); ++i) {
		std::cerr << "LEVEL " << i << std::endl
					 << "  head gen= " << accus[i].head_is_generated << std::endl
					 << "  bracket = " << accus[i].bracket << std::endl
					 << "  order   = " << accus[i].order << std::endl
					 << "  parts   = " << accus[i].parts.size() << std::endl;
		for(unsigned int k=0; k<accus[i].parts.size(); ++k) 
			std::cerr << "             " << accus[i].parts[k] << std::endl;
		std::cerr << "  accu    = " << accus[i].accu << std::endl;
		}
	std::cerr << "CURRENT "  << std::endl
	          << "  head gen= " << cur.head_is_generated << std::endl
				 << "  bracket = " << cur.bracket << std::endl
				 << "  order   = " << cur.order << std::endl
				 << "  parts   = " << cur.parts.size() << std::endl;
	for(unsigned int k=0; k<cur.parts.size(); ++k) 
		std::cerr << "             " << cur.parts[k] << std::endl;
	std::cerr << "  accu    = " << cur.accu << std::endl;
	}

unsigned int preprocessor::current_bracket_(bool deep) const
	{
	unsigned int i;
	if((i=accus.size())>0) {
		do {
			if(accus[--i].bracket)
				return accus[i].bracket;
			} while(i>0 && deep);
		}
	return 0;
	}

bool preprocessor::default_is_product_() const
	{
	if(cur.order==order_prod || cur.order==order_frac || cur.order==order_arrow || cur.order==order_comma ||
		cur.order==order_minus || cur.order==order_plus || cur.order==order_equals ||
		cur.order==order_unequals ) 
		return true; // spaces in comma-separated lists/products are stars
	int n=current_bracket_(true);
	return (n==2 || n==3 || n==0 || (n==1 && cur.is_index==false) );
	}

bool preprocessor::is_digits_(const std::string& str) const
	{
	if(str.size()==0) return false;
	for(unsigned int i=0; i<str.size(); ++i) 
		if(!isdigit(str[i]) && str[i]!='.') return false;
	return true;
	}

unsigned int preprocessor::is_closing_bracket_(unsigned char c) const
	{
	for(unsigned int i=0; i<sizeof(close_brackets); ++i) {
		if(c==close_brackets[i]) return i+1;
		if(c==close_brackets[i]+128) return i+1+128;
		}
	return 0;
	}

unsigned int preprocessor::is_opening_bracket_(unsigned char c) const
	{
	for(unsigned int i=0; i<sizeof(open_brackets); ++i) {
		if(c==open_brackets[i])     return i+1;
		if(c==open_brackets[i]+128) return i+1+128;
		}
	return 0;
	}

unsigned int preprocessor::is_bracket_(unsigned char c) const
	{
	unsigned int ret;
	if((ret=is_opening_bracket_(c))) return ret;
	return is_closing_bracket_(c);
	}

bool preprocessor::is_already_bracketed_(const std::string& str) const
	{
	if(str.size()>=2) 
		if(is_bracket_(str[0])) // && str[0]!='{')
			if(is_opening_bracket_(str[0])==is_closing_bracket_(str[str.size()-1]))
				return true;
	return false;
	}

unsigned char preprocessor::get_token_(unsigned char prev_token)
	{
	unsigned char candidate=0;  
   // Candidate is set to the code of an infix operator if
   // encountered. Because we have to treat spaces as multiplication
	// operators, yet these can also just denote spaces by themselves if 
   // followed by an explicit operator, the loop below iterates until a
	// non-space character is found. 

//	std::cout << "cur char=" << cur_str[cur_pos] << "\n";

	if(verbatim_) {
		return cur_str[cur_pos];
		}
	if(next_is_product_) {
		next_is_product_=false;
		candidate='*';
//		if(cur_pos<cur_str.size()) {
//			--cur_pos;
//			return '*';
//			}
		}
	if(is_opening_bracket_(prev_token)) {
		while(isblank(cur_str[cur_pos])) {
			++cur_pos;
			}
		}
	while(cur_pos<cur_str.size()) {
		unsigned char c=cur_str[cur_pos];
//		std::cerr << "|" << c << "|" << std::endl;
		if(c==':' && cur_str[cur_pos+1]=='=') { // ':' and ':=' are the same thing
			++cur_pos;
			cur_str[cur_pos]=tok_declare;
			c=tok_declare;
			}
		if(c==':' && cur_str[cur_pos+1]==':') {
			++cur_pos;
			cur_str[cur_pos]='$';
			c='$'; // FIXME: Another hack... (property)
			}
		if(c=='*' && cur_str[cur_pos+1]=='*') {
			++cur_pos;
			cur_str[cur_pos]=tok_pow;
			c=tok_pow; // FIXME: Another hack... (exponent)
			}
		if(c=='-' && cur_str[cur_pos+1]=='>') {
			++cur_pos;
			cur_str[cur_pos]=tok_arrow;
			c=tok_arrow; // FIXME: Another hack... (arrow)
			}
		if(c==':' && cur_str[cur_pos+1]=='>') {
			++cur_pos;
			cur_str[cur_pos]=tok_set_option;
			c=tok_set_option; // FIXME: Another hack... (set_option)
			}
		if(c=='!' && cur_str[cur_pos+1]=='=') {
			++cur_pos;
			cur_str[cur_pos]=tok_unequals;
			c=tok_unequals; // FIXME: Another hack... (unequals)
			}
		if(c=='.') {
			if(cur_str[cur_pos+1]=='.') {
				++cur_pos;
				if(cur_str[cur_pos+1]=='.') {
					cur_str[cur_pos]=tok_siblings;
					cur_str[cur_pos+1]=tok_siblings;
					++cur_pos;
					c=tok_siblings;
					return '@'; // FIXME: worst hack of them all, we're running out of tricks...
					}
				else {
					cur_str[cur_pos]=tok_sequence;
					c=tok_sequence; // FIXME: Another hack... (sequence)
					}
				}
			else return c;
			}
		
//	HERE: how do we force get_token to return a '*' for the space separating .... and b ?

		if(isblank(c)) {
//			std::cout << "blank " << (int)(c) << "\n";
			if(candidate==0) candidate=' ';
			++cur_pos;
			continue;
			}
		else if(c=='^' && candidate==' ') { // Isolated '^' with space prefixing it.
			cur_str[cur_pos]=tok_wedge;
			++cur_pos;
			return tok_wedge;
			}
		else if(is_infix_operator_(c)) {
			if(candidate && candidate!=' ') {
				--cur_pos;
				return candidate;
				// FIXME: have to test whether this operator allows for missing first child.
//				throw std::logic_error("two subsequent operators without intermediate operand");
				}
			candidate=c;
			++cur_pos;
			continue;
			}
		else if(c=='@' && isdigit(cur_str[cur_pos+1]) ) { // eat labels completely
			while(isdigit(cur_str[++cur_pos]));
			continue;
			}
		if(candidate==0 && c=='\\' && is_bracket_(cur_str[cur_pos+1])) {
			++cur_pos;
			c=cur_str[cur_pos]+128;
			}
		if(candidate!=0) {
//			std::cout << "candidate " << (int)candidate << "\n";
			--cur_pos;
			if(candidate==' ' && default_is_product_()) return '*';
			return candidate;
			}
		else {
			std::string acplusone=cur.accu + (char)(c);
			if( ( is_closing_bracket_(prev_token) && is_opening_bracket_(c) && cur.head_is_generated ) ||
				 ( is_closing_bracket_(prev_token) && !is_infix_operator_(c) && !is_bracket_(c) && !is_link_(c) ) ||
				 ( is_digits_(cur.accu) && !is_digits_(acplusone) && !is_link_(c) && !is_closing_bracket_(c) ) ||
				 ( !is_infix_operator_(prev_token) && !is_opening_bracket_(prev_token) && !is_link_(prev_token) && prev_token!=' ' && c=='\\' ) ) {
				--cur_pos;
				if( default_is_product_() ) return '*';
				else                        return ' ';
				}
			if(is_link_(prev_token) && !(is_opening_bracket_(c) || c=='\\' || prev_token=='$')) {
				if(cur.accu.size()==0 || ! (cur.accu[0]=='@' || (cur.accu[0]=='\\' && cur.accu[1]=='@'))) { 
					next_is_product_=true; // turn aaa_bbb into aaa_b bb (but keep @aaa_bbb)
					}
				}
			return c;
			}
		}
	if(candidate) {
		--cur_pos;
		return candidate;
		}
	throw std::range_error("get_token out of range");
	}

void preprocessor::bracket_strings_(unsigned int cb, std::string& obrack, std::string& cbrack) const
	{
	obrack=""; cbrack="";
	if(cb==0) return;
	if(cb>128) {
		obrack="\\"; cbrack="\\";
		cb-=128;
		}
	obrack+=open_brackets[cb-1];
	cbrack+=close_brackets[cb-1];
	}


// unwind the "stack" until we reach infix operator level "onum" or bottom of stack.
// if onum==sizeof(orders), unwind to the bracket level of the second (optional) argument.

bool preprocessor::unwind_(unsigned int onum, unsigned int bracketgoal, bool usebracket) const
	{
//	std::cout << "unwinding to " << onum << " " << usebracket << std::endl;
	
//	static long iters=0;
	bool generated_head=false;
	bool bracket_reached=false;

	do {
		generated_head=false;
		bracket_reached=false;

		std::string tmp;
		if(cur.accu.size()>0)
			cur.parts.push_back(cur.accu);
		unsigned int cb=current_bracket_();

//		std::cout << "current bracket = " << cb << std::endl;
//		print_stack();

		if(onum==sizeof(orders) && bracketgoal) {
			if(cb!=0) {
				if(cb==bracketgoal)
					bracket_reached=true;
				else {
//					std::cerr << "wanted " << bracketgoal << " got " << cb << std::endl;
					show_and_throw_("Bracket mismatch.");					
					}
				}
			}
		else {
			// if we are unwinding to reach a different operator level, never unwind beyond
			// a bracket (in that case, we have to push up again).
			if(current_bracket_())
				bracket_reached=true;
			}

		std::string obrack, cbrack;
		if(cb==0 || !usebracket || (onum==sizeof(orders) && bracket_reached && accus.back().accu.size()>0)) {
			obrack="{";
			cbrack="}";
			}
		else bracket_strings_(cb, obrack, cbrack);

		if(cur.parts.size()>1 || cur.order==order_factorial) { // More than one argument to the function.
			if(cur.order<sizeof(orders)) {
				bool special=(cb==2 && std::string(order_names[cur.order])=="\\comma");
				if(!special)
					tmp+=order_names[cur.order];
				for(unsigned int k=0; k<cur.parts.size(); ++k) 
					if(cur.parts[k].size()>0) {
						if(is_already_bracketed_(cur.parts[k]) && (cur.parts[k][0]==obrack[0] || obrack[0]=='{')) 
							tmp+=cur.parts[k];
						else {
							if(special) {
								if(k>0) tmp+="(";
								}
							else 
								tmp+=obrack;
							tmp+=cur.parts[k];
							if(special) {
								if(k<cur.parts.size()-1) tmp+=")";
								}
							else 
								tmp+=cbrack;
							}
						}
				generated_head=true;
				}
			else {
				tmp=obrack;
				for(unsigned int k=0; k<cur.parts.size(); ++k) {
					tmp+=cur.parts[k];
					if(k!=cur.parts.size()-1) tmp+=" ";
					}
				tmp+=cbrack;
				}
			}
		else { // Function with only one argument.
			if(cur.parts.size()>0) {
				bracket_strings_(cb, obrack, cbrack);
//				std::cout << cur.parts[0] << " : " << is_already_bracketed_(cur.parts[0]) << std::endl;
				if(onum!=sizeof(orders) || 
					( is_already_bracketed_(cur.parts[0]) && ( cur.parts[0][0]==obrack[0] || obrack[0]=='{')))
					tmp=cur.parts[0];
				else {
					tmp=obrack;
					tmp+=cur.parts[0];
					tmp+=cbrack;
					}
				}
			}
//		std::cerr << "string to unwind: " << tmp << std::endl;
		if(accus.size()==0) {
			cur.erase();
			cur.order=onum;
			cur.accu=tmp;
			break;
			}
		if(onum!=sizeof(orders) && bracket_reached) {
			cur.accu=tmp;
			cur.parts.clear();
			break;
			}
		cur=accus.back();
		accus.pop_back();
		// if there was something at this level already, wrap the string just constructed
		// in brackets
		if(cur.accu.size()>0) {
			if(tmp.size()>0) {
				bracket_strings_(cb, obrack, cbrack);
				// FIXME: next line might want to check for {(aaa)} problems (like all other
				// lines that check for is_already_bracketed_)
				if(!is_opening_bracket_(tmp[0]) && !(tmp[0]=='\\' && is_opening_bracket_(tmp[1])) && ( !is_already_bracketed_(tmp) || tmp[0]!=obrack[0])) {
					cur.accu+=obrack;
					cur.accu+=tmp;
					cur.accu+=cbrack;
					}
				else 
					cur.accu+=tmp;
				}
			generated_head=false;
			}
		else cur.accu+=tmp;

		} while(cur.order<onum && !bracket_reached);

	// Reset the head_is_generated flag, otherwise we end up with
	// situations where e.g. a \prod{}{} node pushed into the parts
	// vector sets this flag, then a '+' sign is read, and any
	// subsequent elements are automatically treated as if they got the
	// head generated too.
	cur.head_is_generated=false;

	if(onum!=sizeof(orders) && cur.accu.size()>0 && onum>=cur.order && !bracket_reached) {
//		std::cout << cur.accu.size() << std::endl;
		cur.parts.push_back(cur.accu);
		cur.accu.erase();
		}
	if(generated_head && cur.accu.size()>0)
		cur.head_is_generated=true;
	return bracket_reached;
	}

preprocessor::accu_t::accu_t()
	: head_is_generated(false), order(sizeof(orders)), bracket(0), is_index(false)
	{
	}

void preprocessor::accu_t::erase()
	{
	head_is_generated=false;
	accu.erase();
	order=sizeof(orders);
	parts.clear();
	bracket=0;
	}

void preprocessor::strip_outer_brackets() const
	{
	if(is_already_bracketed_(cur.accu))
		cur.accu=cur.accu.substr(1,cur.accu.size()-2);
	}

void preprocessor::erase()
	{
	cur_pos=0;
	cur_str.erase();
	cur.erase();
	accus.clear();
	next_is_product_=false;
	}

void preprocessor::parse_(const std::string& str)
	{
	cur_str=str;
	parse_internal_();
	}

void preprocessor::show_and_throw_(const std::string& str) const
	{
	std::stringstream ss;
	ss << std::endl << cur_str << std::endl;
	for(unsigned int i=0; i<cur_pos; ++i)
		ss << " ";
	ss << "^" << std::endl
		<< str;
	ss << std::endl << cur.order << std::endl;
	throw std::logic_error(ss.str());
	throw std::logic_error(str);
	}

void preprocessor::parse_internal_()
	{
	static long chars_parsed=0;
	cur_pos=0;
	unsigned char c=0, prev_token=0;
	while(cur_pos<cur_str.size()) {
		unsigned onum=0;
		prev_token=c;
		c=get_token_(c);
		if(eat_initial_whitespace_ && c=='*')
			goto loopdone;
		eat_initial_whitespace_=false;
		//std::cerr << c << ", brac=" << cur.bracket << ", order=" << cur.order << std::endl;
		if(verbatim_) {
			if(c=='\"')
				verbatim_=false;
			cur.accu+=c;
			goto loopdone;
			}
		for(; onum<sizeof(orders); ++onum) {
			if(c==orders[order_factorial] && cur.accu.size()>0 && cur.accu[0]=='@')
				break; // keep the '!' in '@command!(foobar)'
			if(c==orders[onum]) {
				if(cur.order==sizeof(orders) || onum==cur.order) {
//					std::cerr << "same order, was " << cur.order << " now " << onum << std::endl;
					// FIXME: this is a hack
					if(onum==order_minus && cur.accu.size()==0)
						cur.accu="0";
					cur.order=onum;
					++chars_parsed;
//					if(chars_parsed%(100L)==0) 
//						std::cout << chars_parsed << std::endl;
					cur.parts.push_back(cur.accu);
					cur.accu.erase();
					cur.head_is_generated=false;
//					print_stack();
					}
				else if(onum<cur.order) {
					std::string tmp=cur.accu;
					cur.accu.erase();
					cur.head_is_generated=false;
					if(tmp.size()==0 && onum==order_minus) // hack for "foo: -a-b"
						tmp="0";
//					std::cerr << "pushing up |" << tmp << "| to " << orders[onum] << std::endl;
					accus.push_back(cur);
					cur.erase();
					cur.parts.push_back(tmp);
					cur.order=onum;
//					std::cerr << "next character:" << cur_str[cur_pos+1] << std::endl;
					}
				else {
//					std::cerr << "need to unwind from " << cur.order << " to " << onum << std::endl;
					if(unwind_(onum, 0, false) || onum<cur.order) {
//						std::cerr << "lifting up again, previous was " << cur.accu << ", " << cur.order << std::endl;
						std::string tmp(cur.accu);
						cur.accu.erase();
						// for "a = b c + e" we need to push back
						// for "[a b, c]"    we don't.
						if(onum<cur.order)
							accus.push_back(cur);
						cur.parts.clear();
						cur.parts.push_back(tmp);
						cur.order=onum;
//						print_stack();
						goto loopdone;
						}
					if(cur.accu.size()>0)
						cur.parts.push_back(cur.accu);
					cur.accu.erase();
					cur.head_is_generated=false;
					}
				goto loopdone;
				}
			}
		if(c==' ' && cur.order!=sizeof(orders)) {
			if(cur_pos==cur_str.size()-1)
				goto loopdone;
			else show_and_throw_("Whitespace encountered at unexpected location.");
			}
		unsigned int ind;
		if((ind=is_opening_bracket_(c))) {
//			 std::cerr << "entering bracket level " << ind << " " << prev_token << std::endl;
//			 std::cerr << "is index before entering: " << cur.is_index << std::endl;
			cur.bracket=ind; // remember the bracket, so that we can add it when we return to this level.
//			std::cerr << "is index after entering: " << cur.is_index << std::endl;
			accus.push_back(cur);
			if(is_link_(prev_token)) cur.is_index=true;
			else                     cur.is_index=false;
			cur.erase();
			cur.bracket=0;
			}
		else if((ind=is_closing_bracket_(c))) {
//			std::cerr << "closing bracket encountered " << ind << " " << (char)cur.bracket << " " << (char)current_bracket_() << std::endl;
			if(ind==1 && cur.accu.size()==0 && cur.parts.size()==0)
				cur.parts.push_back(cur.accu); // empty lists count
//			std::cerr << "on accu: " << accus.back().is_index << std::endl;
			unwind_(sizeof(orders), ind);
//			std::cerr << "is index now " << cur.is_index << std::endl;
			cur.bracket=0;
//			cur.is_index=false; // TEST
			}
		else if(c==' ') {
//			std::cout << "space\n";
			if(cur.accu.size()>0) {
				cur.parts.push_back(cur.accu);
				cur.accu.erase();
				cur.head_is_generated=false;
				}
			}
		else cur.accu+=c;
		if(c=='\"')
			verbatim_=true;

	   loopdone:
		++cur_pos;
		}
	}

