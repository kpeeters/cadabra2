
#include "DisplayBase.hh"

DisplayBase::DisplayBase(const Kernel& k, const Ex& e)
	: tree(e), kernel(k)
	{
	}

void DisplayBase::output(std::ostream& str) 
	{
	Ex::iterator it=tree.begin();
	if(it==tree.end()) return;

	std::cerr << Ex(it) << std::endl;

	output(str, it);
	}

void DisplayBase::output(std::ostream& str, Ex::iterator it) 
	{
	dispatch(str, it);
	}

