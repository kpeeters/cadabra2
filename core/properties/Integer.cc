
#include "properties/Integer.hh"

std::string Integer::name() const
	{
	return "Integer";
	}

bool Integer::parse(keyval_t& keyvals)
	{
	keyval_t::iterator kv=keyvals.find("range");
	if(kv!=keyvals.end()) {
		from=exptree(exptree::child(kv->second,0));
		to  =exptree(exptree::child(kv->second,1));
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
