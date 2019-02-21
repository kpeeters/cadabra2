
#include "Algorithm.hh"
#include "DisplayTerminal.hh"
#include "properties/Accent.hh"

using namespace cadabra;

DisplayTerminal::DisplayTerminal(const Kernel& k, const Ex& e, bool uuc)
	: DisplayBase(k, e), use_unicode(uuc)
	{
	symmap = {
		{"\\cos", "cos"},
		{"\\sin", "sin"},
		{"\\tan", "tan"},
		{"\\int", "∫" },
		{"\\sum", "∑" }
	};

	greekmap = {
		{"\\alpha",   "α" },
		{"\\beta",    "β" },  // beta seems to be reserved
		{"\\gamma",   "γ" }, // gamma seems to be reserved 
		{"\\delta",   "δ" },
		{"\\epsilon", "ε" },
		{"\\zeta",    "ζ" },
		{"\\eta",     "η" },
		{"\\theta",   "θ" },
		{"\\iota",    "ι" },
		{"\\kappa",   "κ" },
		{"\\lambda",  "λ" }, // lambda is reserved
		{"\\mu",      "μ" },
		{"\\nu",      "ν" },
		{"\\xi",      "ξ" },
		{"\\omicron", "ο" },
		{"\\pi",      "π" },
		{"\\rho",     "ρ" },
		{"\\sigma",   "σ" },
		{"\\tau",     "τ" },
		{"\\upsilon", "υ" },
		{"\\phi",     "φ" },
		{"\\chi",     "χ" },
		{"\\psi",     "ψ" },
		{"\\omega",   "ω" },

		{"\\Alpha",   "Α" },
		{"\\Beta",    "Β" },
		{"\\Gamma",   "Γ" },
		{"\\Delta",   "Δ" },
		{"\\Epsilon", "Ε" },
		{"\\Zeta",    "Ζ" },
		{"\\Eta",     "Η" },
		{"\\Theta",   "ϴ" },
		{"\\Iota",    "Ι" },
		{"\\Kappa",   "Κ" },
		{"\\Lambda",  "Λ" },
		{"\\Mu",      "Μ" },
		{"\\Nu",      "Ν" },
		{"\\Xi",      "Ξ" },
		{"\\Omicron", "Ο" },
		{"\\Pi",      "Π" },
		{"\\Rho",     "Ρ" },
		{"\\Sigma",   "Σ" },
		{"\\Tau",     "Τ" },
		{"\\Upsilon", "Υ" },
		{"\\Phi",     "Φ" },
		{"\\Chi",     "Χ" },
		{"\\Psi",     "Ψ" },
		{"\\Omega",   "Ω" },

	};
	}

// Logic: each node should check whether it needs to have brackets printed around
// all of itself. The function below will determine that by inspecting the parent
// node. Nodes should NOT print brackets for children.

bool DisplayTerminal::needs_brackets(Ex::iterator it)
	{
	// FIXME: may need looking at properties
	// FIXME: write as individual parent/current tests
	if(tree.is_valid(tree.parent(it))==false) return false;

//	Ex::iterator parent_it = tree.parent(it);
	Ex::iterator child_it  = it;
	int child_num = tree.index(it);

	std::string parent=*tree.parent(it)->name;
	std::string child =*it->name;

	if(parent=="\\partial" && child=="\\sum") return true;

	if(parent=="\\frac" && ( child=="\\sum" || child=="\\prod" || (*child_it->multiplier!=1 && child_num>0) )) return true;

	if(parent=="\\pow" && ( !it->is_integer() || child=="\\prod" || child=="\\sum" || child=="\\pow")  ) return true;

	// negative and fractional exponents need brackets.
	if(parent=="\\pow" && (*it->multiplier < 0 || !it->is_integer()) ) return true;

//	if(parent=="\\pow" && child_num>0 && *child_it->multiplier!=1 ) return true;

	if(parent=="\\prod" && child=="\\sum") return true;

//	if(it->fl.parent_rel==str_node::p_none && (child=="\\sum" || child=="\\prod") ) return true;

	return false;
	}

//void DisplayTerminal::dispatch(std::ostream& str, Ex::iterator it) 
//	{
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
	str_node::parent_rel_t previous_parent_rel_=str_node::p_invalid;

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
		
		if(current_bracket_==str_node::b_none) {
			if(previous_bracket_==str_node::b_none && current_parent_rel_==previous_parent_rel_ && current_parent_rel_==str_node::p_none)
				str << ", ";
			}
		
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

void DisplayTerminal::print_multiplier(std::ostream& str, Ex::iterator it, int mult)
	{
	mpz_class denom=it->multiplier->get_den();

	if(denom!=1) {
		if(mult*it->multiplier->get_num()<0) {
			str << " - ";
			mult *= -1;
			}
		str << " " << mult * it->multiplier->get_num() << "/" << it->multiplier->get_den() << " ";
		}
	else if(mult * (*it->multiplier)==-1) {
		str << "-";
		}
	else {
		str << mult * (*it->multiplier);
		}

/*	bool turned_one=false;
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
	str << "*"; */
	}

void DisplayTerminal::print_opening_bracket(std::ostream& str, str_node::bracket_t br, str_node::parent_rel_t pr)
	{
	switch(br) {
		case str_node::b_none:
			if(pr==str_node::p_none)     str << "(";
			else                         str << "{";
			break;
		case str_node::b_pointy: str << "<"; break;
		case str_node::b_curly:  str << "{"; break;
		case str_node::b_round:  str << "(";   break;
		case str_node::b_square: str << "[";   break;
		default :	return;
		}
	++(bracket_level);
	}

void DisplayTerminal::print_closing_bracket(std::ostream& str, str_node::bracket_t br, str_node::parent_rel_t pr)
	{
	switch(br) {
		case str_node::b_none:   
			if(pr==str_node::p_none)     str << ")";
			else                         str << "}";
			break;
		case str_node::b_pointy: str << ">"; break;
		case str_node::b_curly:  str << "}"; break;
		case str_node::b_round:  str << ")";   break;
		case str_node::b_square: str << "]";   break;
		default :	return;
		}
	--(bracket_level);
	}

void DisplayTerminal::print_parent_rel(std::ostream& str, str_node::parent_rel_t pr, bool)
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
		case str_node::p_invalid: break;
		}
	}

void DisplayTerminal::dispatch(std::ostream& str, Ex::iterator it) 
	{
	if(*it->name=="\\prod")            print_productlike(str, it, " ");
	else if(*it->name=="\\sum")        print_sumlike(str, it);
	else if(*it->name=="\\frac")       print_fraclike(str, it);
	else if(*it->name=="\\comma")      print_commalike(str, it);
	else if(*it->name=="\\commutator") print_commutator(str, it, true);
	else if(*it->name=="\\anticommutator") print_commutator(str, it, false);
	else if(*it->name=="\\arrow")      print_arrowlike(str, it);
	else if(*it->name=="\\pow")        print_powlike(str, it);
	else if(*it->name=="\\wedge")      print_productlike(str, it, " ^ ");
	else if(*it->name=="\\int")        print_intlike(str, it);
	else if(*it->name=="\\sum")        print_intlike(str, it);
	else if(*it->name=="\\equals")     print_equalitylike(str, it);
	else if(*it->name=="\\components") print_components(str, it);
	else if(*it->name=="\\ldots")      print_dots(str, it);
	else                               print_other(str, it);
	}

void DisplayTerminal::print_commalike(std::ostream& str, Ex::iterator it) 
	{
	Ex::sibling_iterator sib=tree.begin(it);
	bool first=true;
	str << "{";
	while(sib!=tree.end(it)) {
		if(first)
			first=false;
		else
			str << ", ";
		dispatch(str, sib);
		++sib;
		}
	str << "}";
	}

void DisplayTerminal::print_arrowlike(std::ostream& str, Ex::iterator it) 
	{
	Ex::sibling_iterator sib=tree.begin(it);
	dispatch(str, sib);
	str << " → ";
	++sib;
	dispatch(str, sib);
	}

void DisplayTerminal::print_fraclike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator num=tree.begin(it), den=num;
	++den;

	if(*it->multiplier!=1) 
		print_multiplier(str, it);

//	if(needs_brackets(num))
//		str << "(";

	if(num->is_rational()==false || (*it->multiplier)==1)
		dispatch(str, num);

//	if(needs_brackets(num))
//		str << ")";

	str << "/";

//	if(needs_brackets(den))
//		str << "(";
	
	dispatch(str, den);

//	if(needs_brackets(den))
//		str << ")";
	}

void DisplayTerminal::print_productlike(std::ostream& str, Ex::iterator it, const std::string& inbetween)
	{
	if(needs_brackets(it)) 
		str << "(";

	if(*it->multiplier!=1) {
		print_multiplier(str, it);
//		Ex::sibling_iterator st=tree.begin(it);
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

	if(needs_brackets(it)) 
		str << ")";
	}

void DisplayTerminal::print_sumlike(std::ostream& str, Ex::iterator it) 
	{
	assert(*it->multiplier==1);

	if(needs_brackets(it)) 
		str << "(";

	Ex::sibling_iterator ch=tree.begin(it);
	while(ch!=tree.end(it)) {
		if(*ch->multiplier>=0 && ch!=tree.begin(it))
			str << " + "; 

		dispatch(str, ch);
		++ch;
		}

	if(needs_brackets(it)) 
		str << ")";
	str << std::flush;
	}

void DisplayTerminal::print_powlike(std::ostream& str, Ex::iterator it)
	{
	if(needs_brackets(it))
		str << "(";

	Ex::sibling_iterator sib=tree.begin(it);
	if(*it->multiplier!=1)
		print_multiplier(str, it);
	dispatch(str, sib);
//	if(needs_brackets(sib))
//		str << ")";
	str << "**";
	++sib;
//	if(needs_brackets(sib))
//		str << "(";
	dispatch(str, sib);

	if(needs_brackets(it))
		str << ")";
	}

void DisplayTerminal::print_intlike(std::ostream& str, Ex::iterator it)
	{
	if(*it->multiplier!=1)
		print_multiplier(str, it);
	if(!use_unicode || getenv("CADABRA_NO_UNICODE")!=0)
		str << *it->name << "{";
	else
		str << symmap[*it->name] << "(";
	Ex::sibling_iterator sib=tree.begin(it);
	dispatch(str, sib);
	++sib;
	if(tree.is_valid(sib)) {
		str << "}{";
		dispatch(str, sib);
		}
	str << "}";
	}

void DisplayTerminal::print_equalitylike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator sib=tree.begin(it);
	dispatch(str, sib);
	str << " = ";
	++sib;
	if(sib==tree.end(it)) 
		throw ConsistencyException("Found equals node with only one child node.");
	dispatch(str, sib);
	}

void DisplayTerminal::print_commutator(std::ostream& str, Ex::iterator it, bool comm)
	{
	if(comm) str << "[";
	else     str << "{";
	auto sib=tree.begin(it);
	bool first=true;
	while(sib!=tree.end(it)) {
		if(!first) str << ", ";
		else       first=false;
		dispatch(str, sib);
		++sib;
		}
	if(comm) str << "]";
	else     str << "}";
	}

void DisplayTerminal::print_components(std::ostream& str, Ex::iterator it)
	{
	if( ! (use_unicode && getenv("CADABRA_NO_UNICODE")==0) ) {
		print_other(str, it);
		return;
		}

	str << R"(□)";
	auto sib=tree.begin(it);
	auto end=tree.end(it);
	--end;
	if(sib!=end) {
		bool needs_close=false;
		str_node::parent_rel_t prel=str_node::parent_rel_t::p_none;
		while(sib!=end) {
			if(sib->fl.parent_rel!=prel) {
				if(needs_close) 
					str << "}";
				needs_close=true;
				prel=sib->fl.parent_rel;
				if(prel==str_node::p_sub)   str << "_{";
				if(prel==str_node::p_super) str << "^{";
				}
			dispatch(str, sib);
			++sib;
			}
		if(needs_close)
			str << "}";
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

void DisplayTerminal::print_dots(std::ostream& str, Ex::iterator )
   {
   str << " ... ";
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
		if(needs_brackets(it))
			str << ")";
		return;
		}
	
	bool needs_extra_brackets=false;
//	const Accent *ac=kernel.properties.get<Accent>(it);
//	if(!ac) { // accents should never get additional curly brackets, {\bar}{g} does not print.
//		Ex::sibling_iterator sib=tree.begin(it);
//		while(sib!=tree.end(it)) {
//			if(sib->is_index()) 
//				needs_extra_brackets=true;
//			++sib;
//			}
//		}
	
	if(needs_extra_brackets) str << "{"; // to prevent double sup/sub script errors
	std::string sbit=*it->name;
	if(use_unicode && getenv("CADABRA_NO_UNICODE")==0) {
		auto rn1 = symmap.find(sbit);
		if(rn1!=symmap.end())
			sbit = rn1->second;
		auto rn = greekmap.find(sbit);
		if(rn!=greekmap.end())
			sbit = rn->second;
		}
	str << sbit;
		
	if(needs_extra_brackets) str << "}";

	print_children(str, it);


	if(needs_brackets(it))
		str << ")";
	}
