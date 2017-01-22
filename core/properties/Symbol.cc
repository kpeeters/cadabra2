
#include "properties/Symbol.hh"

using namespace cadabra;

std::string Symbol::name() const
	{
	return "Symbol";
	}

const Symbol *Symbol::get(const Properties& properties, Ex::iterator it, bool ignore_parent_rel) 
	{
	if(*it->name=="\\sum") {
		// Check whether all siblings have the Symbol property.
		Ex::sibling_iterator sib=it.begin();
		const Symbol *s=0;
		while(sib!=it.end()) {
			s = properties.get<Symbol>(sib, ignore_parent_rel);
			if(!s)
				break;
			++sib;
			}
		return s;
		}
	else return properties.get<Symbol>(it, ignore_parent_rel);
	}
