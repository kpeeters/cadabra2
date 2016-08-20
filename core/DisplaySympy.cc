
#include "Algorithm.hh"
#include "Functional.hh"
#include "DisplaySympy.hh"
#include "properties/Depends.hh"
#include "properties/Accent.hh"

DisplaySympy::DisplaySympy(const Kernel& kernel, const Ex& e)
	: DisplayBase(kernel, e)
	{
	symmap = {
		{"\\cos", "cos"},
		{"\\sin", "sin"},
		{"\\tan", "tan"},
		{"\\int", "Integral" },
		{"\\matrix", "Matrix" },
		{"\\sum", "Sum" },
		{"\\theta", "theta"},
		{"\\Theta", "Theta"},
		{"\\Phi", "Phi"},
		{"\\phi", "phi"},
		{"\\Sigma", "Sigma"},
		{"\\partial", "Derivative"},
		{"\\dot", "dot"},
		{"\\ddot", "ddot"}
		};
	}

//TODO: complete this list (take from Sympy)

bool DisplaySympy::needs_brackets(Ex::iterator it)
	{
	// FIXME: may need looking at properties
	// FIXME: write as individual parent/current tests
	if(tree.is_valid(tree.parent(it))==false) return false;

	if(*tree.parent(it)->name=="\\prod" || *tree.parent(it)->name=="\\frac" || *tree.parent(it)->name=="\\pow") {
		if(*it->name=="\\sum") return true;
		if(*tree.parent(it)->name=="\\pow" && (*it->multiplier<0 || (*it->multiplier!=1 && *it->name!="1")) ) return true;
		}
	else if(it->fl.parent_rel==str_node::p_none) {
		if(*it->name=="\\sum") return false;
		}
	else {
		if(*it->name=="\\sum")  return true;
		if(*it->name=="\\prod") return true;
		}
	return false;
	}


void DisplaySympy::print_other(std::ostream& str, Ex::iterator it)
	{
	if(needs_brackets(it))
		str << "(";

	// print multiplier and object name
	if(*it->multiplier!=1)
		print_multiplier(str, it);
	
	if(*it->name=="1") {
		if(*it->multiplier==1 || (*it->multiplier==-1)) // this would print nothing altogether.
			str << "1";

		if(needs_brackets(it))
			str << ")";
		return;
		}
	
//	const Accent *ac=properties.get<Accent>(it);
//	if(!ac) { // accents should never get additional curly brackets, {\bar}{g} does not print.
//		Ex::sibling_iterator sib=tree.begin(it);
//		while(sib!=tree.end(it)) {
//			if(sib->is_index()) 
//				needs_extra_brackets=true;
//			++sib;
//			}
//		}
	
	auto rn = symmap.find(*it->name);
	if(rn!=symmap.end())
		str << rn->second;
	else
		str << *it->name;

	print_children(str, it);

	if(needs_brackets(it))
		str << ")";
	}

void DisplaySympy::print_children(std::ostream& str, Ex::iterator it, int skip) 
	{
	// Sympy has no notion of children with different parent relations; it's all 
	// functions of functions kind of stuff. What we will do is print upper and
	// lower indices as 'UP(..)' and 'DN(..)' type arguments, and then convert
	// them back later.

	// We need to know if the symbol has implicit dependence on other symbols,
	// as this needs to be made explicit for sympy.

	const Depends *dep=kernel.properties.get<Depends>(it);

	Ex::sibling_iterator ch=tree.begin(it);
	if(ch!=tree.end(it) || dep!=0) {
		str << "(";
		bool first=true;
		while(ch!=tree.end(it)) {
			if(first) first=false;
			else      str << ", ";
			if(ch->fl.parent_rel==str_node::p_super) 
				str << "UP(";
			if(ch->fl.parent_rel==str_node::p_sub) 
				str << "DN(";

			dispatch(str, ch);

			if(ch->fl.parent_rel==str_node::p_super || ch->fl.parent_rel==str_node::p_sub) 
				str << ")";
			++ch;
			}
		if(dep) {
			if(!first) str << ", ";
			Ex deplist=dep->dependencies(kernel, it);
			DisplaySympy ds(kernel, deplist);
			ds.output(str);
			}
		str << ")";
		}
	}

void DisplaySympy::print_multiplier(std::ostream& str, Ex::iterator it)
	{
	bool suppress_star=false;
	mpz_class denom=it->multiplier->get_den();

	if(denom!=1) {
		if(false && it->multiplier->get_num()<0)
			str << "(" << it->multiplier->get_num() << ")";
		else 
			str << it->multiplier->get_num();
		str << "/" << it->multiplier->get_den();
		}
	else if(*it->multiplier==-1) {
		str << "-";
		suppress_star=true;
		}
	else {
		str << *it->multiplier;
		}
	
	if(!suppress_star && !(*it->name=="1"))
		str << "*";
	}

void DisplaySympy::print_opening_bracket(std::ostream& str, str_node::bracket_t br)
	{
	switch(br) {
		case str_node::b_none:   str << ")";   break;
		case str_node::b_pointy: str << "\\<"; break;
		case str_node::b_curly:  str << "\\{"; break;
		case str_node::b_round:  str << "(";   break;
		case str_node::b_square: str << "[";   break;
		default :	return;
		}
	}

void DisplaySympy::print_closing_bracket(std::ostream& str, str_node::bracket_t br)
	{
	switch(br) {
		case str_node::b_none:   str << ")";   break;
		case str_node::b_pointy: str << "\\>"; break;
		case str_node::b_curly:  str << "\\}"; break;
		case str_node::b_round:  str << ")";   break;
		case str_node::b_square: str << "]";   break;
		default :	return;
		}
	}

void DisplaySympy::print_parent_rel(std::ostream& str, str_node::parent_rel_t pr, bool first)
	{
	switch(pr) {
		case str_node::p_super:    str << "^"; break;
		case str_node::p_sub:      str << "_"; break;
		case str_node::p_property: str << "$"; break;
		case str_node::p_exponent: str << "**"; break;
		case str_node::p_none: break;
		case str_node::p_components: break;
		}
	}

void DisplaySympy::dispatch(std::ostream& str, Ex::iterator it) 
	{
	// The node names below should only be reserved node names; all others
	// should be looked up using properties. FIXME
	if(*it->name=="\\prod")        print_productlike(str, it, "*");
	else if(*it->name=="\\sum")    print_sumlike(str, it);
	else if(*it->name=="\\frac")   print_fraclike(str, it);
	else if(*it->name=="\\comma")  print_commalike(str, it);
	else if(*it->name=="\\arrow")  print_arrowlike(str, it);
	else if(*it->name=="\\pow")    print_powlike(str, it);
	else if(*it->name=="\\int")    print_intlike(str, it);
	else if(*it->name=="\\sum")    print_intlike(str, it);
	else if(*it->name=="\\equals") print_equalitylike(str, it);
	else if(*it->name=="\\components") print_components(str, it);
	else if(*it->name=="\\partial") print_partial(str, it);
	else if(*it->name=="\\matrix") print_matrix(str, it);
	else                           print_other(str, it);
	}

void DisplaySympy::print_commalike(std::ostream& str, Ex::iterator it) 
	{
	Ex::sibling_iterator sib=tree.begin(it);
	bool first=true;
	while(sib!=tree.end(it)) {
		if(first)
			first=false;
		else
			str << ", ";
		dispatch(str, sib);
		++sib;
		}
	//print_closing_bracket(str, (*it).fl.bracket, str_node::p_none);	
	}

void DisplaySympy::print_arrowlike(std::ostream& str, Ex::iterator it) 
	{
	Ex::sibling_iterator sib=tree.begin(it);
	str << "rule(";
	dispatch(str, sib);
	str << ", ";
	++sib;
	dispatch(str, sib);
	str << ")";
	}

void DisplaySympy::print_fraclike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator num=tree.begin(it), den=num;
	++den;

	if(*it->multiplier!=1) {
		print_multiplier(str, it);
		}
	dispatch(str, num);

	str << "/(";
	
	dispatch(str, den);

	str << ")";
	}

void DisplaySympy::print_productlike(std::ostream& str, Ex::iterator it, const std::string& inbetween)
	{
	if(needs_brackets(it)) 
		str << "(";

	if(*it->multiplier!=1) {
		print_multiplier(str, it);
		Ex::sibling_iterator st=tree.begin(it);
		}

	// To print \prod{\sum{a}{b}}{\sum{c}{d}} correctly:
	// If there is any sum as child, and if the sum children do not
	// all have the same bracket type (different from b_none or b_no),
	// then print brackets.
	
	str_node::bracket_t previous_bracket_=str_node::b_invalid;
	bool beginning_of_group=true;
	Ex::sibling_iterator ch=tree.begin(it);
	while(ch!=tree.end(it)) {
		str_node::bracket_t current_bracket_=(*ch).fl.bracket;
		if(previous_bracket_!=current_bracket_) {
			if(current_bracket_!=str_node::b_none) {
				print_opening_bracket(str, current_bracket_);
				beginning_of_group=true;
				}
			}
		dispatch(str, ch);
		++ch;
		if(ch==tree.end(it)) {
			if(current_bracket_!=str_node::b_none) 
				print_closing_bracket(str, current_bracket_);
			}

		if(ch!=tree.end(it)) {
			str << inbetween;
			}
		previous_bracket_=current_bracket_;
		}

	if(needs_brackets(it)) 
		str << ")";
//	if(close_bracket) str << ")";
	}

void DisplaySympy::print_sumlike(std::ostream& str, Ex::iterator it) 
	{
	assert(*it->multiplier==1);

	if(needs_brackets(it)) 
		str << "(";

	unsigned int steps=0;

	Ex::sibling_iterator ch=tree.begin(it);
	while(ch!=tree.end(it)) {
		if(++steps==20) {
			steps=0;
			}
		if(*ch->multiplier>=0 && ch!=tree.begin(it))
			str << "+"; 

		dispatch(str, ch);
		++ch;
		}

	if(needs_brackets(it)) 
		str << ")";
	str << std::flush;
	}

void DisplaySympy::print_powlike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator sib=tree.begin(it);
	if(*it->multiplier!=1)
		print_multiplier(str, it);
	dispatch(str, sib);
	str << "**";
	++sib;
	dispatch(str, sib);
	}

void DisplaySympy::print_intlike(std::ostream& str, Ex::iterator it)
	{
	if(*it->multiplier!=1)
		print_multiplier(str, it);
	str << symmap[*it->name] << "(";
	Ex::sibling_iterator sib=tree.begin(it);
	dispatch(str, sib);
	++sib;
	if(tree.is_valid(sib)) {	
		str << ", ";
		dispatch(str, sib);
		}
	str << ")";
	}

void DisplaySympy::print_equalitylike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator sib=tree.begin(it);
	dispatch(str, sib);
	str << " = ";
	++sib;
	dispatch(str, sib);
	}

void DisplaySympy::print_components(std::ostream& str, Ex::iterator it)
	{
	str << *it->name;
	auto sib=tree.begin(it);
	auto end=tree.end(it);
	--end;
	while(sib!=end) {
		dispatch(str, sib);
		++sib;
		}
	str << "\n";
	sib=tree.begin(end);
	while(sib!=tree.end(end)) {
		str << "    ";
		dispatch(str, sib);
		str << "\n";
		++sib;
		}
	}

void DisplaySympy::print_partial(std::ostream& str, Ex::iterator it)
	{
	if(*it->multiplier!=1)
		print_multiplier(str, it);

	str << "diff(";
	Ex::sibling_iterator sib=tree.begin(it);
	while(sib!=tree.end(it)) {
		if(sib->fl.parent_rel==str_node::p_none) {
			dispatch(str, sib);
			break;
			}
		++sib;
		}
	sib=tree.begin(it);
	while(sib!=tree.end(it)) {
		if(sib->fl.parent_rel!=str_node::p_none) {
			str << ", ";
			dispatch(str, sib);
			}
		++sib;
		}
	str << ")";
	}

void DisplaySympy::print_matrix(std::ostream& str, Ex::iterator it)
	{
	str << "Matrix([";
	auto comma=tree.begin(it);
	Ex::sibling_iterator row_it = tree.begin(comma);
	while(row_it!=tree.end(comma)) {
		if(row_it!=tree.begin(comma)) str << ", ";
		Ex::sibling_iterator col_it = tree.begin(row_it);
		str << "[";
		while(col_it!=tree.end(row_it)) {
			if(col_it!=tree.begin(row_it)) str << ", ";
			dispatch(str, col_it);
			++col_it;
			}
		str << "]";
		++row_it;
		}
	str << "])";
	}

bool DisplaySympy::children_have_brackets(Ex::iterator ch) const
	{
	Ex::sibling_iterator chlds=tree.begin(ch);
	str_node::bracket_t childbr=chlds->fl.bracket;
	if(childbr==str_node::b_none || childbr==str_node::b_no)
		return false;
	else return true;
	}

void DisplaySympy::import(Ex& ex)
	{
	cadabra::do_subtree(ex, ex.begin(), [&](Ex::iterator it) -> Ex::iterator {
			for(auto& m: symmap) {
				if(m.second==*it->name) {
					it->name=name_set.insert(m.first).first;
					break;
					}
				}
			return it;
			});
	}
