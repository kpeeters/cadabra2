
#include "DisplayBase.hh"

DisplayBase::DisplayBase(const Properties& p, const Ex& e)
	: tree(e), properties(p)
	{
	}

bool DisplayBase::needs_brackets(Ex::iterator it)
	{
	// FIXME: may need looking at properties

	if(*tree.parent(it)->name=="\\prod" || *tree.parent(it)->name=="\\frac" || *tree.parent(it)->name=="\\pow") {
		if(*it->name=="\\sum") return true;
		if(*tree.parent(it)->name=="\\pow" && (*it->multiplier<0 || (*it->multiplier!=1 && *it->name!="1")) ) return true;
		}
	else if(it->fl.parent_rel==str_node::p_none) {
		if(*it->name=="\\sum") return false;
		}
	else {
		if(*it->name=="\\sum")  return true;
		if(*it->name=="\\prod") return true;
		}
	return false;
	}

