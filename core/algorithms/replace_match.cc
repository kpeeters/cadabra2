
#include "Cleanup.hh"
#include "algorithms/replace_match.hh"
#include "algorithms/substitute.hh"

replace_match::replace_match(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool replace_match::can_apply(iterator it) 
	{
	if(*it->name=="\\sum" || *it->name=="\\comma") return true;
	return false;
	}

Algorithm::result_t replace_match::apply(iterator& it)
	{
	Ex current(tr);
	Ex rules=tr.pop_history();

//	std::cerr << "rules: " << rules << std::endl;
//	std::cerr << "current: " << current << std::endl;
//	std::cerr << "old: " << tr << std::endl;
//
	it=tr.begin();
	substitute subs(kernel, tr, rules);

	auto sumnode=tr.begin(it);
	sibling_iterator sib=tr.begin(sumnode);
	bool replaced=false;
	while(sib!=tr.end(sumnode)) {
		if(subs.can_apply(sib)) {
			// std::cerr << "applying" << std::endl;
			sib=tr.erase(sib);
			if(!replaced) {
				replaced=true;
				iterator ci = tr.insert_subtree(sib, current.begin(current.begin()));
				cleanup_dispatch(kernel, tr, ci);
				}
			}
		else ++sib;
		}

	// std::cerr << tr << std::endl;

	cleanup_dispatch_deep(kernel, tr);
	it=tr.begin();
	
//	std::cerr << tr << std::endl;

	return result_t::l_applied;
	}

