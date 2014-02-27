
#include "DisplayTeX.hh"
#include "Algorithm.hh"
#include "properties/LaTeXForm.hh"
#include "properties/Accent.hh"

#define nbsp   " "
//(( parent.utf8_output?(unichar(0x00a0)):" "))
#define zwnbsp ""
//(( parent.utf8_output?(unichar(0xfeff)):""))

DisplayTeX::DisplayTeX(const Properties& p, const exptree& e)
	: tree(e), properties(p)
	{
	}

void DisplayTeX::output(std::ostream& str) 
	{
	exptree::iterator it=tree.begin();

	output(str, it);
	}

void DisplayTeX::output(std::ostream& str, exptree::iterator it) 
	{
	if(*it->name=="\\expression") {
		dispatch(str, tree.begin(it));
		return;
		}

	// print multiplier and object name
	if(*it->multiplier!=1)
		print_multiplier(str, it);
	
	if(*it->name=="1") {
		if(*it->multiplier==1 || (*it->multiplier==-1)) // this would print nothing altogether.
			str << "1";
		return;
		}
	
	const LaTeXForm *lf=properties.get<LaTeXForm>(it);
	bool needs_extra_brackets=false;
	const Accent *ac=properties.get<Accent>(it);
	if(!ac && extra_brackets_for_symbols) { // accents should never get additional curly brackets, {\bar}{g} does not print.
		exptree::sibling_iterator sib=tree.begin(it);
		while(sib!=tree.end(it)) {
			if(sib->is_index()) 
				needs_extra_brackets=true;
			++sib;
			}
		}
	
	if(needs_extra_brackets) str << "{"; // to prevent double sup/sub script errors
	if(lf) str << lf->latex;
	else   str << texify(*it->name);
	if(needs_extra_brackets) str << "}";
//	else str << *it->name;

	print_children(str, it);
	}

std::string DisplayTeX::texify(const std::string& str) const
	{
	std::string res;
   for(unsigned int i=0; i<str.size(); ++i) {
		 if(str[i]=='#') res+="\\#";
		 else res+=str[i];
      }
   return res;
	}

void DisplayTeX::print_children(std::ostream& str, exptree::iterator it, int skip) 
	{
	previous_bracket_   =str_node::b_invalid;
	previous_parent_rel_=str_node::p_none;

	int number_of_nonindex_children=0;
	int number_of_index_children=0;
	exptree::sibling_iterator ch=tree.begin(it);
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
		current_bracket_   =(*ch).fl.bracket;
		current_parent_rel_=(*ch).fl.parent_rel;
		const Accent *is_accent=properties.get<Accent>(it);
		
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
		else str << nbsp;
		
		previous_bracket_=current_bracket_;
		previous_parent_rel_=current_parent_rel_;
		++chnum;
		}
	}

void DisplayTeX::print_multiplier(std::ostream& str, exptree::iterator it)
	{
	bool turned_one=false;
	mpz_class denom=it->multiplier->get_den();

	if(*it->multiplier<0) {
		if(*tree.parent(it)->name=="\\sum") { // sum takes care of minus sign
			if(*it->multiplier!=-1) {
				str << "\\frac{" << -(it->multiplier->get_num()) << "}{" 
					 << it->multiplier->get_den() << "}";
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

	if(!turned_one && !(*it->name=="1")) {
		if(print_star) {
			if(tight_star) str << "*";
// 			else if(utf8_output)
//				str << unichar(0x00a0) << "*" << unichar(0x00a0);
			else
				str << " * ";
			}
		else {
			if(!tight_star) {
				if(latex_spacing) str << "\\, ";
				else              str << " ";
				}
			} 
		}
	}

void DisplayTeX::print_opening_bracket(std::ostream& str, str_node::bracket_t br, str_node::parent_rel_t pr)
	{
	switch(br) {
		case str_node::b_none: 
			str << "{";
//			if(parent.output_format==exptree_output::out_xcadabra && pr==str_node::p_none) str << "(";  
//			else                                                                           str << "{";
			break;
		case str_node::b_pointy: str << "\\<"; break;
		case str_node::b_curly:  str << "\\{"; break;
		case str_node::b_round:  str << "(";   break;
		case str_node::b_square: str << "[";   break;
		default :	return;
		}
	++(bracket_level);
	}

void DisplayTeX::print_closing_bracket(std::ostream& str, str_node::bracket_t br, str_node::parent_rel_t pr)
	{
	switch(br) {
		case str_node::b_none:   
			str << "}";
//			if(parent.output_format==exptree_output::out_xcadabra && pr==str_node::p_none) str << ")";  
//			else                                                                           str << "}";
			break;
		case str_node::b_pointy: str << "\\>"; break;
		case str_node::b_curly:  str << "\\}"; break;
		case str_node::b_round:  str << ")";   break;
		case str_node::b_square: str << "]";   break;
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
		}
	// Prevent line break after this character.
	str << zwnbsp;
	}

void DisplayTeX::dispatch(std::ostream& str, exptree::iterator it) 
	{
	if(*it->name=="\\prod")     print_productlike(str, it, " ");
	else if(*it->name=="\\sum") print_sumlike(str, it);
	else
		output(str, it);
	}

void DisplayTeX::print_productlike(std::ostream& str, exptree::iterator it, const std::string& inbetween)
	{
//	bool close_bracket=false;
	if(*it->multiplier!=1) {
		print_multiplier(str, it);
		exptree::sibling_iterator st=tree.begin(it);
//		while(st!=tr.end(it)) {
//			if(*st->name=="\\sum") {
//				str << "(";
//				close_bracket=true;
//				break;
//				}
//			++st;
//			}
		}

	// To print \prod{\sum{a}{b}}{\sum{c}{d}} correctly:
	// If there is any sum as child, and if the sum children do not
	// all have the same bracket type (different from b_none or b_no),
	// then print brackets.
	
	str_node::bracket_t previous_bracket_=str_node::b_invalid;
	bool beginning_of_group=true;
	exptree::sibling_iterator ch=tree.begin(it);
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
//				else if(utf8_output) {
//					str << unichar(0x00a0) << inbetween << unichar(0x00a0);
//					}
				else str << " " << inbetween << " ";
				}
			else {
//				 if(output_format==exptree_output::out_texmacs) str << "\\,";
//				 else 
				str << " ";
				 }
			}
		previous_bracket_=current_bracket_;
		}

//	if(close_bracket) str << ")";
	}

void DisplayTeX::print_sumlike(std::ostream& str, exptree::iterator it) 
	{
	bool close_bracket=false;
	if(*it->multiplier!=1) 
		print_multiplier(str, it);

	exptree::iterator par=tree.parent(it);
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
	exptree::sibling_iterator ch=tree.begin(it);
	bool beginning_of_group=true;
	bool mathematica_postponed_endl=false;
	while(ch!=tree.end(it)) {
		if(++steps==20) {
			if(latex_linefeeds)
				str << "%\n" << std::flush; // prevent LaTeX overflow
			steps=0;
			}
		str_node::bracket_t current_bracket_=(*ch).fl.bracket;
		if(previous_bracket_!=current_bracket_)
			if(current_bracket_!=str_node::b_none) {
				if(ch!=tree.begin(it)) {
					if(tight_plus) str << "+";
					else if(utf8_output) str << " +" << unichar(0x00a0);
					else                        str << " + ";
					}
				print_opening_bracket(str, current_bracket_, str_node::p_none);
				beginning_of_group=true;
				}
		if(beginning_of_group) {
			beginning_of_group=false;
			if(*ch->multiplier<0) {
				if(tight_plus) str << "-";
				else if(utf8_output) str << " -" << unichar(0x00a0);
				else                        str << " - ";
					
				}
			}
		else {
			if(*ch->multiplier<0) {
				if(tight_plus)       str << "-";
				else if(utf8_output) str << " -" << unichar(0x00a0);
				else                        str << " - ";
				}
			else {
				if(tight_plus) str << "+";
				else if(utf8_output) str << " +" << unichar(0x00a0);
				else                        str << " + ";
				}
			}
		if(mathematica_postponed_endl) {
			str << std::endl;
			mathematica_postponed_endl=false;
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

bool DisplayTeX::children_have_brackets(exptree::iterator ch) const
	{
	exptree::sibling_iterator chlds=tree.begin(ch);
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
