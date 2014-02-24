
#include "properties/Symbol.hh"

std::string Symbol::name() const
	{
	return "Symbol";
	}

const Symbol *Symbol::get(exptree::iterator it, bool ignore_parent_rel) 
	{
	if(*it->name=="\\sum") {
		// Check whether all siblings have the Symbol property.
		exptree::sibling_iterator sib=it.begin();
		const Symbol *s=0;
		while(sib!=it.end()) {
			s = Properties::get<Symbol>(sib, ignore_parent_rel);
			if(!s)
				break;
			++sib;
			}
		return s;
		}
	else return Properties::get<Symbol>(it, ignore_parent_rel);
	}
