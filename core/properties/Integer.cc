
#include "properties/Integer.hh"

std::string Integer::name() const
	{
	return "Integer";
	}

bool Integer::parse(exptree& tr, exptree::iterator pat, exptree::iterator prop, keyval_t&)
	{
	if(tr.number_of_children(prop)==0)
		return true;

	exptree::iterator seq=tr.child(prop,0);
	if(tr.number_of_children(prop)>1 || *(seq->name)!="\\sequence") {
//		txtout << name() << ": only one argument (range) accepted." << std::endl;
		return false;
		}
	
	if(tr.number_of_children(seq)!=2) {
//		txtout << name() << ": sequence needs first and last element." << std::endl;
		return false;
		}

	from=exptree(tr.child(seq,0));
	to  =exptree(tr.child(seq,1));
//	tr.subtree(from, tr.child(seq,0), tr.child(seq,1));
//	tr.subtree(to,   tr.child(seq,1), tr.end(seq));
//	from=to_long(*(tr.child(seq,0)->multiplier));
//	to  =to_long(*(tr.child(seq,1)->multiplier));

	exptree::iterator sm=difference.set_head(str_node("\\sum"));
	difference.append_child(sm,to.begin())->fl.bracket=str_node::b_round;
	exptree::iterator term2=difference.append_child(sm,from.begin());
	flip_sign(term2->multiplier);
	term2->fl.bracket=str_node::b_round;
	difference.append_child(sm,str_node("1"))->fl.bracket=str_node::b_round;

	exptree::sibling_iterator sib=difference.begin(sm);
	while(sib!=difference.end(sm)) {
		if(*sib->name=="\\sum") {
			difference.flatten(sib);
			sib=difference.erase(sib);
			}
		else ++sib;
		}

// V2: is this handled by the core?
//	collect_terms ct(difference, difference.end());
//	ct.apply(sm);

	return true;
	}

void Integer::display(std::ostream& str) const
	{
	str << "Integer";
	if(from.begin()!=from.end()) {
		str << "(" << *(from.begin()->multiplier) << ".." 
			 << *(to.begin()->multiplier) << ")";
		}
	}
