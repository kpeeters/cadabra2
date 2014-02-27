
#include "DisplayTeX.hh"
#include "properties/LaTeXForm.hh"
#include "properties/Accent.hh"

#define nbsp   " "
//(( parent.utf8_output?(unichar(0x00a0)):" "))
#define zwnbsp ""
//(( parent.utf8_output?(unichar(0xfeff)):""))

DisplayTeX::DisplayTeX(const Properties& p, exptree& e)
	: tree(e), properties(p)
	{
	}

void DisplayTeX::output(std::ostream& str) 
	{
	exptree::iterator it=tree.begin();

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
	if(!ac) { // accents should never get additional curly brackets, {\bar}{g} does not print.
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
			str << zwnbsp << "(" << zwnbsp;
			}
		parent.get_printer(ch)->print_infix(str, ch);
//			if((*it).fl.mark && parent.highlight) str << "\033[1m"; 
		++ch;
		if(ch==tree.end(it) || current_bracket_!=str_node::b_none || current_bracket_!=(*ch).fl.bracket || current_parent_rel_!=(*ch).fl.parent_rel) {
			str << ")";
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
		if(parent.print_star && !(parent.output_format==exptree_output::out_texmacs
								  || parent.output_format==exptree_output::out_xcadabra) ) {
			if(parent.tight_star) str << "*";
			else if(parent.utf8_output)
				str << unichar(0x00a0) << "*" << unichar(0x00a0);
			else
				str << " * ";
			}
		else {
			if(!parent.tight_star) {
				if(parent.output_format==exptree_output::out_texmacs
					|| parent.output_format==exptree_output::out_xcadabra ) str << "\\, ";
				else str << " ";
				}
			} 
		}
	}

void DisplayTeX::print_opening_bracket(std::ostream& str, str_node::bracket_t br, str_node::parent_rel_t pr)
	{
	switch(br) {
		case str_node::b_none: 
			if(parent.output_format==exptree_output::out_xcadabra && pr==str_node::p_none) str << "(";  
			else                                                                           str << "{";
			break;
		case str_node::b_pointy: str << "\\<"; break;
		case str_node::b_curly:  str << "\\{"; break;
		case str_node::b_round:  str << "(";   break;
		case str_node::b_square: str << "[";   break;
		default :	return;
		}
	++(parent.bracket_level);
	}

void DisplayTeX::print_closing_bracket(std::ostream& str, str_node::bracket_t br, str_node::parent_rel_t pr)
	{
	switch(br) {
		case str_node::b_none:   
			if(parent.output_format==exptree_output::out_xcadabra && pr==str_node::p_none) str << ")";  
			else                                                                           str << "}";
			break;
		case str_node::b_pointy: str << "\\>"; break;
		case str_node::b_curly:  str << "\\}"; break;
		case str_node::b_round:  str << ")";   break;
		case str_node::b_square: str << "]";   break;
		default :	return;
		}
	--(parent.bracket_level);
	}

void DisplayTeX::print_parent_rel(std::ostream& str, str_node::parent_rel_t pr, bool first)
	{
	switch(pr) {
		case str_node::p_super:    
			if(!first && (parent.output_format==exptree_output::out_texmacs || 
							  parent.output_format==exptree_output::out_xcadabra) ) str << "\\,";
			str << "^"; break;
		case str_node::p_sub:
			if(!first && (parent.output_format==exptree_output::out_texmacs ||
							  parent.output_format==exptree_output::out_xcadabra)) str << "\\,";
			str << "_"; break;
		case str_node::p_property: str << "$"; break;
		case str_node::p_exponent: str << "**"; break;
		case str_node::p_none: break;
		}
	// Prevent line break after this character.
	str << zwnbsp;
	}
