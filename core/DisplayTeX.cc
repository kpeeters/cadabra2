
#include "Symbols.hh"
#include "DisplayTeX.hh"
#include "Algorithm.hh"
#include "properties/LaTeXForm.hh"
#include "properties/Derivative.hh"
#include "properties/Accent.hh"

#define nbsp   " "
//(( parent.utf8_output?(unichar(0x00a0)):" "))
#define zwnbsp ""
//(( parent.utf8_output?(unichar(0xfeff)):""))

DisplayTeX::DisplayTeX(const Kernel& k, const Ex& e)
	: DisplayBase(k, e)
	{
	symmap = {
		{"\\hat", "\\widehat"},
		{"\\tilde", "\\widetilde"}
		};

	curly_bracket_operators = {
		"\\sqrt"
	};
	}

bool DisplayTeX::needs_brackets(Ex::iterator it)
	{
	// FIXME: may need looking at properties
	// FIXME: write as individual parent/current tests
	if(tree.is_valid(tree.parent(it))==false) return false;

	std::string parent=*tree.parent(it)->name;
	std::string child =*it->name;

	if(parent=="\\partial" && child=="\\sum") return false; // Always handled by the functional argument. Was: true;

	if(parent=="\\int" && child=="\\sum") return true;

	if(parent=="\\indexbracket" && child=="\\prod") return false;

	if(parent=="\\pow" && (child=="\\prod" || child=="\\sum")) return  true;

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

bool DisplayTeX::reads_as_operator(Ex::iterator obj, Ex::iterator arg) const
	{
	const Derivative *der=kernel.properties.get<Derivative>(obj);
	if(der) {
		// FIXME: this needs fine-tuning; there are more cases where
		// no brackets are needed.
      if((*arg->name).size()==1 || cadabra::symbols::greek.find(*arg->name)!=cadabra::symbols::greek.end()) return true;
		}
	auto it=curly_bracket_operators.find(*obj->name);
	if(it!=curly_bracket_operators.end()) return true;

	return false;
	}

void DisplayTeX::print_other(std::ostream& str, Ex::iterator it) 
	{
	if(needs_brackets(it))
		str << "\\left(";

	// print multiplier and object name
	if(*it->multiplier!=1)
		print_multiplier(str, it);
	
	if(*it->name=="1") {
		if(*it->multiplier==1 || (*it->multiplier==-1)) // this would print nothing altogether.
			str << "1";
		return;
		}
	
	const LaTeXForm *lf=kernel.properties.get<LaTeXForm>(it);
	bool needs_extra_brackets=false;
	const Accent *ac=kernel.properties.get<Accent>(it);
	if(!ac && extra_brackets_for_symbols) { // accents should never get additional curly brackets, {\bar}{g} does not print.
		Ex::sibling_iterator sib=tree.begin(it);
		while(sib!=tree.end(it)) {
			if(sib->is_index()) 
				needs_extra_brackets=true;
			++sib;
			}
		}
	
	if(needs_extra_brackets) str << "{"; // to prevent double sup/sub script errors
	if(lf) str << lf->latex_form();
	else   str << texify(*it->name);
	if(needs_extra_brackets) str << "}";
//	else str << *it->name;

	print_children(str, it);


	if(needs_brackets(it))
		str << "\\right)";
	}

std::string DisplayTeX::texify(std::string str) const
	{
	auto rn = symmap.find(str);
	if(rn!=symmap.end())
		str = rn->second;

	std::string res;
   for(unsigned int i=0; i<str.size(); ++i) {
		 if(str[i]=='#') res+="\\#";
		 else res+=str[i];
      }
   return res;
	}

void DisplayTeX::print_children(std::ostream& str, Ex::iterator it, int skip) 
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
		
		bool function_bracket_needed=true;
		if(current_bracket_==str_node::b_none) {
			if(previous_bracket_==str_node::b_none)
				str << ", ";
			function_bracket_needed=!reads_as_operator(it, ch);
			}

		if(current_bracket_!=str_node::b_none || previous_bracket_!=current_bracket_ || previous_parent_rel_!=current_parent_rel_) {
			print_parent_rel(str, current_parent_rel_, ch==tree.begin(it));

			if(is_accent==0 && function_bracket_needed) 
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
			if(is_accent==0 && function_bracket_needed) 
				print_closing_bracket(str,  (number_of_nonindex_children>1 /* &&number_of_index_children>0 */ && 
													  current_parent_rel_!=str_node::p_sub && 
													  current_parent_rel_!=str_node::p_super ? str_node::b_round:current_bracket_), 
											 current_parent_rel_);
			else str  << "}";
			}
		else str << nbsp;
		
		previous_bracket_=current_bracket_;
		previous_parent_rel_=current_parent_rel_;
		++chnum;
		}
	}

void DisplayTeX::print_multiplier(std::ostream& str, Ex::iterator it, int mult)
	{
	mpz_class denom=it->multiplier->get_den();

	if(denom!=1) {
		if(mult*it->multiplier->get_num()<0) {
			str << " - ";
			mult *= -1;
			}
		str << "\\frac{" << mult * it->multiplier->get_num() << "}{" << it->multiplier->get_den() << "}";
		}
	else if(mult * (*it->multiplier)==-1) {
		str << "-";
		}
	else {
		str << mult * (*it->multiplier);
		}
	}

void DisplayTeX::print_opening_bracket(std::ostream& str, str_node::bracket_t br, str_node::parent_rel_t pr)
	{
	switch(br) {
		case str_node::b_none:
			if(pr==str_node::p_none)     str << "\\left(";
			else                         str << "{";
			break;
		case str_node::b_pointy: str << "\\<"; break;
		case str_node::b_curly:  str << "\\left\\{"; break;
		case str_node::b_round:  str << "\\left(";   break;
		case str_node::b_square: str << "\\left[";   break;
		default :	return;
		}
	++(bracket_level);
	}

void DisplayTeX::print_closing_bracket(std::ostream& str, str_node::bracket_t br, str_node::parent_rel_t pr)
	{
	switch(br) {
		case str_node::b_none:   
			if(pr==str_node::p_none)     str << "\\right)";
			else                         str << "}";
			break;
		case str_node::b_pointy: str << "\\>"; break;
		case str_node::b_curly:  str << "\\right\\}"; break;
		case str_node::b_round:  str << "\\right)";   break;
		case str_node::b_square: str << "\\right]";   break;
		default :	return;
		}
	--(bracket_level);
	}

void DisplayTeX::print_parent_rel(std::ostream& str, str_node::parent_rel_t pr, bool first)
	{
	switch(pr) {
		case str_node::p_super:    
			if(!first && latex_spacing) str << "\\,";
			str << "^"; break;
		case str_node::p_sub:
			if(!first && latex_spacing) str << "\\,";
			str << "_"; break;
		case str_node::p_property: str << "$"; break;
		case str_node::p_exponent: str << "**"; break;
		case str_node::p_none: break;
		case str_node::p_components: break;
		}
	// Prevent line break after this character.
	str << zwnbsp;
	}

void DisplayTeX::dispatch(std::ostream& str, Ex::iterator it) 
	{
	if(*it->name=="\\prod")                print_productlike(str, it, " ");
	else if(*it->name=="\\sum")            print_sumlike(str, it);
	else if(*it->name=="\\frac")           print_fraclike(str, it);
	else if(*it->name=="\\comma")          print_commalike(str, it);
	else if(*it->name=="\\arrow")          print_arrowlike(str, it);
	else if(*it->name=="\\pow")            print_powlike(str, it);
	else if(*it->name=="\\int")            print_intlike(str, it);
	else if(*it->name=="\\equals")         print_equalitylike(str, it);
	else if(*it->name=="\\commutator")     print_commutator(str, it, true);
	else if(*it->name=="\\anticommutator") print_commutator(str, it, false);
	else if(*it->name=="\\components")     print_components(str, it);
	else if(*it->name=="\\conditional")    print_conditional(str, it);
	else if(*it->name=="\\greater" || *it->name=="\\less")  print_relation(str, it);
	else if(*it->name=="\\indexbracket")   print_indexbracket(str, it);
	else                                   print_other(str, it);
	}

void DisplayTeX::print_commalike(std::ostream& str, Ex::iterator it) 
	{
	Ex::sibling_iterator sib=tree.begin(it);
	bool first=true;
	str << "\\left\\{";
	while(sib!=tree.end(it)) {
		if(first)
			first=false;
		else
			str << ", \\linebreak[0] ";
		dispatch(str, sib);
		++sib;
		}
	str << "\\right\\}";
	}

void DisplayTeX::print_arrowlike(std::ostream& str, Ex::iterator it) 
	{
	Ex::sibling_iterator sib=tree.begin(it);
	dispatch(str, sib);
	str << " \\rightarrow ";
	++sib;
	dispatch(str, sib);
	}

void DisplayTeX::print_fraclike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator num=tree.begin(it), den=num;
	++den;

	int mult=1;
	if(*it->multiplier<0) {
		str << " - ";
		mult=-1;
		}
	str << "\\frac{";
	if(mult * (*it->multiplier)!=1) {
		print_multiplier(str, it, mult);
		}

	dispatch(str, num);
	str << "}{";
	dispatch(str, den);
	str << "}";
	}

void DisplayTeX::print_productlike(std::ostream& str, Ex::iterator it, const std::string& inbetween)
	{
	if(*it->multiplier!=1) {
		print_multiplier(str, it);
		}

	if(needs_brackets(it)) 
		str << "\\left(";

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
			 if(print_star) {
				if(tight_star) str << inbetween;
				else str << " " << inbetween << " ";
				}
			else {
				str << " ";
				}
			}
		previous_bracket_=current_bracket_;
		}

	if(needs_brackets(it)) 
		str << "\\right)";

	}

void DisplayTeX::print_sumlike(std::ostream& str, Ex::iterator it) 
	{
	assert(*it->multiplier==1);

	if(needs_brackets(it)) 
		str << "\\left(";

	unsigned int steps=0;

	Ex::sibling_iterator ch=tree.begin(it);
	while(ch!=tree.end(it)) {
//		if(ch!=tree.begin(it))
//			str << "%\n"; // prevent LaTeX overflow.
		if(++steps==20) {
			steps=0;
			str << "%\n"; // prevent LaTeX overflow.
			}
		if(*ch->multiplier>=0 && ch!=tree.begin(it))
			str << "+"; 

		dispatch(str, ch);
		++ch;
		}

	if(needs_brackets(it)) 
		str << "\\right)";
	str << std::flush;
	}

void DisplayTeX::print_powlike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator sib=tree.begin(it);
	assert(sib!=tree.end(it));

	if(*it->multiplier!=1)
		print_multiplier(str, it);
	dispatch(str, sib);
	++sib;

	if(sib!=tree.end(it)) {
		str << "^{";
		dispatch(str, sib);
		str << "}";
		}
	}

void DisplayTeX::print_intlike(std::ostream& str, Ex::iterator it)
	{
	if(*it->multiplier!=1)
		print_multiplier(str, it);
	str << *it->name << "{}"; // FIXME: add limits
	Ex::sibling_iterator sib=tree.begin(it);
	dispatch(str, sib);
	++sib;
	if(tree.is_valid(sib)) {
		str << "\\, {\\rm d}";
		dispatch(str, sib);
		}
	}

void DisplayTeX::print_equalitylike(std::ostream& str, Ex::iterator it)
	{
	Ex::sibling_iterator sib=tree.begin(it);
	dispatch(str, sib);
	str << " = ";
	++sib;
	dispatch(str, sib);
	}

void DisplayTeX::print_commutator(std::ostream& str, Ex::iterator it, bool comm)
	{
	if(comm) str << "{}\\left[";
	else     str << "{}\\left\\{";
	auto sib=tree.begin(it);
	bool first=true;
	while(sib!=tree.end(it)) {
		if(!first) str << ", ";
		else       first=false;
		dispatch(str, sib);
		++sib;
		}
	if(comm) str << "\\right]{}";
	else     str << "\\right\\}{}";
	}

void DisplayTeX::print_components(std::ostream& str, Ex::iterator it)
	{
	assert(*it->multiplier==1);

	auto ind_names=tree.begin(it);
	auto ind_values=tree.end(it);
	--ind_values;

	str << "\\left\\{\\begin{aligned}";
	auto sib=tree.begin(ind_values);
	while(sib!=tree.end(ind_values)) {
		Ex::sibling_iterator c=tree.begin(sib);
		auto iv = tree.begin(c);
		auto in = ind_names;
		str << "\\square";
		while(iv!=tree.end(c)) {
			if(in->fl.parent_rel==str_node::p_sub)   str << "{}_{";
			if(in->fl.parent_rel==str_node::p_super) str << "{}^{";
			dispatch(str, iv);
			str << "}";
			++in;
			++iv;
			}
		str << "= & ";
		++c;
		dispatch(str, c);
		str << "\\\\[-.5ex]\n";
		++sib;
		}
	str << "\\end{aligned}\\right.\n";
	}

void DisplayTeX::print_conditional(std::ostream& str, Ex::iterator it)
	{
	auto sib=tree.begin(it);
	dispatch(str, sib);
	str << "\\quad\\text{with}\\quad{}";
	++sib;
	dispatch(str, sib);
	}

void DisplayTeX::print_relation(std::ostream& str, Ex::iterator it)
	{
	auto sib=tree.begin(it);
	dispatch(str, sib);
	if(*it->name=="\\greater") str << " > ";
	if(*it->name=="\\less")    str << " < ";
	++sib;
	dispatch(str, sib);
	}

void DisplayTeX::print_indexbracket(std::ostream& str, Ex::iterator it)
	{
	auto sib=tree.begin(it);
	str << "\\left(";
	dispatch(str, sib);
	str << "\\right)";
	print_children(str, it, 1);
	}

bool DisplayTeX::children_have_brackets(Ex::iterator ch) const
	{
	Ex::sibling_iterator chlds=tree.begin(ch);
	str_node::bracket_t childbr=chlds->fl.bracket;
	if(childbr==str_node::b_none || childbr==str_node::b_no)
		return false;
	else return true;
	}


// Thanks to Behdad Esfahbod
int k_unichar_to_utf8(kunichar c, char *buf) 
	{
	buf[0]=(c) < 0x00000080 ?   (c)                  :
      (c) < 0x00000800 ?  ((c) >>  6)         | 0xC0 :
      (c) < 0x00010000 ?  ((c) >> 12)         | 0xE0 :
      (c) < 0x00200000 ?  ((c) >> 18)         | 0xF0 :
      (c) < 0x04000000 ?  ((c) >> 24)         | 0xF8 :
		((c) >> 30)         | 0xFC;

	buf[1]=(c) < 0x00000080 ?    0 /* null-terminator */     : 
      (c) < 0x00000800 ?  ((c)        & 0x3F) | 0x80 :         
      (c) < 0x00010000 ? (((c) >>  6) & 0x3F) | 0x80 :         
      (c) < 0x00200000 ? (((c) >> 12) & 0x3F) | 0x80 :         
      (c) < 0x04000000 ? (((c) >> 18) & 0x3F) | 0x80 :         
                            (((c) >> 24) & 0x3F) | 0x80;

	buf[2]=(c) < 0x00000800 ?    0 /* null-terminator */     : 
      (c) < 0x00010000 ?  ((c)        & 0x3F) | 0x80 :         
      (c) < 0x00200000 ? (((c) >>  6) & 0x3F) | 0x80 :         
      (c) < 0x04000000 ? (((c) >> 12) & 0x3F) | 0x80 :         
		(((c) >> 18) & 0x3F) | 0x80;

	buf[3]=(c) < 0x00010000 ?    0 /* null-terminator */     : 
      (c) < 0x00200000 ?  ((c)        & 0x3F) | 0x80 :         
      (c) < 0x04000000 ? (((c) >>  6) & 0x3F) | 0x80 :         
		(((c) >> 12) & 0x3F) | 0x80;

	buf[4]=(c) < 0x00200000 ?    0 /* null-terminator */     : 
      (c) < 0x04000000 ?  ((c)        & 0x3F) | 0x80 :         
		(((c) >>  6) & 0x3F) | 0x80;

	buf[5]=(c) < 0x04000000 ?    0 /* null-terminator */     : 
                             ((c)        & 0x3F) | 0x80;

	buf[6]=0;
	return 6;
	}

const char *unichar(kunichar c)
	{
	static char buffer[7];
	int pos=k_unichar_to_utf8(c, buffer);
	buffer[pos]=0;
	return buffer;
	}
