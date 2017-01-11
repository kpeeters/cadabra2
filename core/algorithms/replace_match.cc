
#include "Cleanup.hh"
#include "algorithms/replace_match.hh"
#include "algorithms/substitute.hh"

using namespace cadabra;

replace_match::replace_match(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool replace_match::can_apply(iterator it) 
	{
	if(tr.history_size()>0) return true;
	return false;
	}

Algorithm::result_t replace_match::apply(iterator& it)
	{
	Ex current(tr); // preserve the expression before popping
	Ex rules=tr.pop_history();

	// std::cerr << "rules: " << rules << std::endl;
	// std::cerr << "current: " << current << std::endl;
	// std::cerr << "old: " << tr << std::endl;

	it=tr.begin();
	while(it!=tr.end()) {
		if(*it->name=="\\sum")
			break;
		++it;
		}
	substitute subs(kernel, tr, rules);

	auto sumnode=it;
	sibling_iterator sib=tr.begin(sumnode);
	bool replaced=false;
	while(sib!=tr.end(sumnode)) {
		if(subs.can_apply(sib)) {
			// std::cerr << "applying" << std::endl;
			sib=tr.erase(sib);
			if(!replaced) {
				// Replace the first term that matches with 'current'.
				replaced=true;
				iterator ci = tr.insert_subtree(sib, current.begin());
				cleanup_dispatch(kernel, tr, ci);
				}
			}
		else ++sib;
		}

	// std::cerr << tr << std::endl;

	cleanup_dispatch(kernel, tr, it);
	
//	std::cerr << tr << std::endl;

	return result_t::l_applied;
	}

