
#include "algorithms/replace_match.hh"

replace_match::replace_match(exptree& tr, iterator it)
	: algorithm(tr, it)
	{
	if(*args_begin()->name!="\\arrow") 
		throw constructor_error();
	}

bool replace_match::can_apply(iterator it) 
	{
	if(*it->name=="\\sum" || *it->name=="\\comma") return true;
	return false;
	}

algorithm::result_t replace_match::apply(iterator& it)
	{
	substitute subs(tr, this_command);

	sibling_iterator sib=tr.begin(it);
//	int i=0;
	bool replaced=false;
	while(sib!=tr.end(it)) {
		if(subs.can_apply(sib)) {
			sib=tr.erase(sib);
			if(!replaced) {
				replaced=true;
				iterator lhs=tr.begin(args_begin());
				iterator rhs=lhs;
				rhs.skip_children();
				++rhs;
				
				tr.insert_subtree(sib, rhs);
				expression_modified=true;
				}
			}
		else ++sib;
		}
	if(expression_modified)
		cleanup_sums_products(tr, it);

	if(expression_modified) return l_applied;
	else return l_no_action;
	}

