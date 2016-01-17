
#include "DisplayBase.hh"

DisplayBase::DisplayBase(const Properties& p, const Ex& e)
	: tree(e), properties(p)
	{
	}

void DisplayBase::output(std::ostream& str) 
	{
	Ex::iterator it=tree.begin();

	output(str, it);
	}

void DisplayBase::output(std::ostream& str, Ex::iterator it) 
	{
	if(*it->name=="\\expression") {
		dispatch(str, tree.begin(it));
		return;
		}

	dispatch(str, it);
	}

