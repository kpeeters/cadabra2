
#include "Algorithm.hh"
#include "DisplayTerminal.hh"
#include "properties/Accent.hh"

DisplayTerminal::DisplayTerminal(const Kernel& k, const Ex& e)
	: DisplayBase(k, e)
	{
	symmap = {
		{"\\cos", "cos"},
		{"\\sin", "sin"},
		{"\\tan", "tan"},
		{"\\int", "Integral" },
		{"\\sum", "Sum" }
		};
	}

bool DisplayTerminal::needs_brackets(Ex::iterator it)
	{
	// FIXME: may need looking at properties
	// FIXME: write as individual parent/current tests
	if(tree.is_valid(tree.parent(it))==false) return false;

	std::string parent=*tree.parent(it)->name;
	std::string child =*it->name;

	if(parent=="\\partial" && child=="\\sum") return true;

	if(*tree.parent(it)->name=="\\prod" || *tree.parent(it)->name=="\\frac" || *tree.parent(it)->name=="\\pow") {
		if(*tree.parent(it)->name!="\\frac" && *it->name=="\\sum") return true;
//		if(*tree.parent(it)->name=="\\pow" && (*it->multiplier<0 || (*it->multiplier!=1 && *it->name!="1")) ) return true;
		}
	else if(it->fl.parent_rel==str_node::p_none) { // function argument
		if(*it->name=="\\sum") return false;
		}
	else {
		if(*it->name=="\\sum")  return true;
		if(*it->name=="\\prod") return true;
		}
	return false;
	}

//void DisplayTerminal::dispatch(std::ostream& str, Ex::iterator it) 
//	{
//	if(*it->name=="\\expression") {
//		dispatch(str, tree.begin(it));
//		return;
//		}
//
//	// print multiplier and object name
//	if(*it->multiplier!=1)
//		print_multiplier(str, it);
//	
//	if(*it->name=="1") {
//		if(*it->multiplier==1 || (*it->multiplier==-1)) // this would print nothing altogether.
//			str << "1";
//		return;
//		}
//	
//	bool needs_extra_brackets=false;
//	const Accent *ac=kernel.properties.get<Accent>(it);
//	if(!ac) { // accents should never get additional curly brackets, {\bar}{g} does not print.
//		Ex::sibling_iterator sib=tree.begin(it);
//		while(sib!=tree.end(it)) {
//			if(sib->is_index()) 
//				needs_extra_brackets=true;
//			++sib;
//			}
//		}
//	
//	if(needs_extra_brackets) str << "{"; // to prevent double sup/sub script errors
//	auto rn = symmap.find(*it->name);
//	if(rn!=symmap.end())
//		str << rn->second;
//	else
//		str << *it->name;
//	if(needs_extra_brackets) str << "}";
//
//	print_children(str, it);
//	}

void DisplayTerminal::print_children(std::ostream& str, Ex::iterator it, int skip) 
	{
	str_node::bracket_t    previous_bracket_   =str_node::b_invalid;
	str_node::parent_rel_t previous_parent_rel_=str_node::p_none;

	int number_of_nonindex_children=0;
	int number_of_index_children=0;
	Ex::sibling_iterator ch=tree.begin(it);
	while(ch!=tree.end(it)) {
		if(ch->is_index()==false) {
			++number_of_nonindex_children;
			if(*ch->name=="\\prod")
				++number_of_nonindex_children;
			}
		else ++number_of_index_children;
		++ch;
		}
	
	ch=tree.begin(it);
	ch+=skip;
	unsigned int chnum=0;
	while(ch!=tree.end(it)) {
		str_node::bracket_t    current_bracket_   =(*ch).fl.bracket;
		str_node::parent_rel_t current_parent_rel_=(*ch).fl.parent_rel;
		const Accent *is_accent=kernel.properties.get<Accent>(it);
		
		if(current_bracket_!=str_node::b_none || previous_bracket_!=current_bracket_ || previous_parent_rel_!=current_parent_rel_) {
			print_parent_rel(str, current_parent_rel_, ch==tree.begin(it));
			if(is_accent==0) 
				print_opening_bracket(str, (number_of_nonindex_children>1 /* &&number_of_index_children>0 */ &&
													 current_parent_rel_!=str_node::p_sub && 
													 current_parent_rel_!=str_node::p_super ? str_node::b_round:current_bracket_), 
											 current_parent_rel_);
			else str << "{";
			}
		
		// print this child depending on its name or meaning
		dispatch(str, ch);
		
		++ch;
		if(ch==tree.end(it) || current_bracket_!=str_node::b_none || current_bracket_!=(*ch).fl.bracket || current_parent_rel_!=(*ch).fl.parent_rel) {
			if(is_accent==0) 
				print_closing_bracket(str,  (number_of_nonindex_children>1 /* &&number_of_index_children>0 */ && 
													  current_parent_rel_!=str_node::p_sub && 
													  current_parent_rel_!=str_node::p_super ? str_node::b_round:current_bracket_), 
											 current_parent_rel_);
			else str  << "}";
			}
		else str << " ";
		
		previous_bracket_=current_bracket_;
		previous_parent_rel_=current_parent_rel_;
		++chnum;
		}
	}

void DisplayTerminal::print_multiplier(std::ostream& str, Ex::iterator it)
	{
	bool turned_one=false;
	mpz_class denom=it->multiplier->get_den();

	if(*it->multiplier<0) {
		if(*tree.parent(it)->name=="\\sum") { // sum takes care of minus sign
			if(*it->multiplier!=-1) {
				if(denom!=1) {
					str << "\\frac{" << -(it->multiplier->get_num()) << "}{" 
						 << it->multiplier->get_den() << "}";
					}
				else {
					str << -(*it->multiplier);
					}
				}
			else                    turned_one=true;
			}
		else	{
			if(denom!=1) {
				str << "(\\frac{" << it->multiplier->get_num() << "}{" 
					 << it->multiplier->get_den() << "})";
				}
			else if(*it->multiplier==-1) {
				str << "-";
				turned_one=true;
				}
			else {
				str << "(" << *it->multiplier << ")";
				}
			}
		}
	else {
		if(denom!=1) {
			str << "\\frac{" << it->multiplier->get_num() << "}{" 
				 << it->multiplier->get_den() << "}";
			}
		else
			str << *it->multiplier;
		}

	if(!turned_one && !(*it->name=="1"))
		str << "*";
	}

void DisplayTerminal::print_opening_bracket(std::ostream& str, str_node::bracket_t br, str_node::parent_rel_t pr)
	{
	switch(br) {
		case str_node::b_none: str << "(";     break;
		case str_node::b_pointy: str << "\\<"; break;
		case str_node::b_curly:  str << "\\{"; break;
		case str_node::b_round:  str << "(";   break;
		case str_node::b_square: str << "[";   break;
		default :	return;
		}
	++(bracket_level);
	}

void DisplayTerminal::print_closing_bracket(std::ostream& str, str_node::bracket_t br, str_node::parent_rel_t pr)
	{
	switch(br) {
		case str_node::b_none:   str << ")";   break;
		case str_node::b_pointy: str << "\\>"; break;
		case str_node::b_curly:  str << "\\}"; break;
		case str_node::b_round:  str << ")";   break;
		case str_node::b_square: str << "]";   break;
		default :	return;
		}
	--(bracket_level);
	}

void DisplayTerminal::print_parent_rel(std::ostream& str, str_node::parent_rel_t pr, bool first)
	{
	switch(pr) {
		case str_node::p_super:    
			str << "^"; break;
		case str_node::p_sub:
			str << "_"; break;
		case str_node::p_property: str << "$"; break;
		case str_node::p_exponent: str << "**"; break;
		case str_node::p_none: break;
		case str_node::p_components: break;
		}
	}

void DisplayTerminal::dispatch(std::ostream& str, Ex::iterator it) 
	{
	if(*it->name=="\\prod")            print_productlike(str, it, "*");
	else if(*it->name=="\\sum")        print_sumlike(str, it);
	else if(*it->name=="\\frac")       print_fraclike(str, it);
	else if(*it->name=="\\comma")      print_commalike(str, it);
	else if(*it->name=="\\arrow")      print_arrowlike(str, it);
	else if(*it->name=="\\pow")        print_powlike(str, it);
	else if(*it->name=="\\int")        print_intlike(str, it);
	else if(*it->name=="\\sum")        print_intlike(str, it);
	else if(*it->name=="\\equals")     print_equalitylike(str, it);
	else if(*it->name=="\\components") print_components(str, it);
	else                               print_other(str, it);
	}

void DisplayTerminal::print_commalike(std::ostream& str, Ex::iterator it) 
	{
	Ex::sibling_iterator sib=tree.begin(it);
	bool first=true;
	print_opening_bracket(str, (*it).fl.bracket, str_node::p_none);	
	while(sib!=tree.end(it)) {
		if(first)
			first=false;
		else
			str << ", ";
		dispatch(str, sib);
		++sib;
		}
	print_closing_bracket(str, (*it).fl.bracket, str_node::p_none);	
	}

void DisplayTerminal::print_arrowlike(std::ostream& str, Ex::iterator it) 
	{
	Ex::sibling_iterator sib=tree.begin(it);
	dispatch(str, sib);
	str << " \\rightarrow ";
	++sib;
	dispatch(str, sib);
	}

void DisplayTerminal::print_fraclike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator num=tree.begin(it), den=num;
	++den;

	bool close_bracket=false;
	if(*it->multiplier!=1) {
		print_multiplier(str, it);
		str << "(";
		close_bracket=true;
		}
	str << "(";

	dispatch(str, num);

	str << ")/(";
	
	dispatch(str, den);

	str << ")";
	if(close_bracket)
		str << ")";
	}

void DisplayTerminal::print_productlike(std::ostream& str, Ex::iterator it, const std::string& inbetween)
	{
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
				print_opening_bracket(str, current_bracket_, str_node::p_none);
				beginning_of_group=true;
				}
			}
		dispatch(str, ch);
		++ch;
		if(ch==tree.end(it)) {
			if(current_bracket_!=str_node::b_none) 
				print_closing_bracket(str, current_bracket_, str_node::p_none);
			}

		if(ch!=tree.end(it)) {
			str << inbetween;
			}
		previous_bracket_=current_bracket_;
		}

//	if(close_bracket) str << ")";
	}

void DisplayTerminal::print_sumlike(std::ostream& str, Ex::iterator it) 
	{
	bool close_bracket=false;
	if(*it->multiplier!=1) 
		print_multiplier(str, it);

	Ex::iterator par=tree.parent(it);
	if(tree.number_of_children(par) - Algorithm::number_of_direct_indices(par)>1) { 
      // for a single argument, the parent already takes care of the brackets
		if(*it->multiplier!=1 || (tree.is_valid(par) && *par->name!="\\expression")) {
			// test whether we need extra brackets
			close_bracket=!children_have_brackets(it);
			if(close_bracket)
				str << "(";
			}
		}

	unsigned int steps=0;

	str_node::bracket_t previous_bracket_=str_node::b_invalid;
	Ex::sibling_iterator ch=tree.begin(it);
	bool beginning_of_group=true;
	while(ch!=tree.end(it)) {
		if(++steps==20) {
			steps=0;
			}
		str_node::bracket_t current_bracket_=(*ch).fl.bracket;
		if(previous_bracket_!=current_bracket_)
			if(current_bracket_!=str_node::b_none) {
				if(ch!=tree.begin(it)) {
					str << "+";
					}
				print_opening_bracket(str, current_bracket_, str_node::p_none);
				beginning_of_group=true;
				}
		if(beginning_of_group) {
			beginning_of_group=false;
			if(*ch->multiplier<0) {
				str << "-";
				}
			}
		else {
			if(*ch->multiplier<0) {
				str << "-";
				}
			else {
				str << "+";
				}
			}
		if(*ch->name=="1" && (*ch->multiplier==1 || *ch->multiplier==-1)) 
			str << "1"; // special case numerical constant
		else 
			dispatch(str, ch);
		++ch;
		if(ch==tree.end(it)) {
			if(current_bracket_!=str_node::b_none)
				print_closing_bracket(str, current_bracket_, str_node::p_none);
			}

		previous_bracket_=current_bracket_;
		}

	if(close_bracket) str << ")";
	str << std::flush;
	}

void DisplayTerminal::print_powlike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator sib=tree.begin(it);
	if(*it->multiplier!=1)
		print_multiplier(str, it);
	dispatch(str, sib);
	str << "**(";
	++sib;
	dispatch(str, sib);
	str << ")";
	}

void DisplayTerminal::print_intlike(std::ostream& str, Ex::iterator it)
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

void DisplayTerminal::print_equalitylike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator sib=tree.begin(it);
	dispatch(str, sib);
	str << " = ";
	++sib;
	dispatch(str, sib);
	}

void DisplayTerminal::print_components(std::ostream& str, Ex::iterator it)
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

bool DisplayTerminal::children_have_brackets(Ex::iterator ch) const
	{
	Ex::sibling_iterator chlds=tree.begin(ch);
	str_node::bracket_t childbr=chlds->fl.bracket;
	if(childbr==str_node::b_none || childbr==str_node::b_no)
		return false;
	else return true;
	}

void DisplayTerminal::print_other(std::ostream& str, Ex::iterator it) 
	{
	if(needs_brackets(it))
		str << "(";

	// print multiplier and object name
	if(*it->multiplier!=1)
		print_multiplier(str, it);
	
	if(*it->name=="1") {
		if(*it->multiplier==1 || (*it->multiplier==-1)) // this would print nothing altogether.
			str << "1";
		return;
		}
	
	bool needs_extra_brackets=false;
	const Accent *ac=kernel.properties.get<Accent>(it);
	if(!ac) { // accents should never get additional curly brackets, {\bar}{g} does not print.
		Ex::sibling_iterator sib=tree.begin(it);
		while(sib!=tree.end(it)) {
			if(sib->is_index()) 
				needs_extra_brackets=true;
			++sib;
			}
		}
	
	if(needs_extra_brackets) str << "{"; // to prevent double sup/sub script errors
	str << *it->name;
	if(needs_extra_brackets) str << "}";

	print_children(str, it);


	if(needs_brackets(it))
		str << ")";
	}
