
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

	// FIXME: we should store the point at which take_match acted in the history,
	// because it may not have been the first point where can_apply returned true.
	it=tr.begin();
	bool acted_at_head=true;
	while(it!=tr.end()) {
		if(tr.is_head(it) && (*it->name=="\\sum" || *it->name=="\\comma")) break;
		if(*it->name=="\\sum" && *tr.parent(it)->name=="\\int") {
			acted_at_head=false;
			break;
			}
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
				iterator ci;
				if(acted_at_head) 
					ci = tr.insert_subtree(sib, current.begin());
				else {
					// FIXME: make this more robust.
					auto findsum=current.begin(current.begin());
					while(findsum->fl.parent_rel!=str_node::parent_rel_t::p_none)
						++findsum;
					ci = tr.insert_subtree(sib, findsum);
					}
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

