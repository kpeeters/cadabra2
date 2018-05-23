
#include "algorithms/take_match.hh"
#include "algorithms/substitute.hh"
#include "Cleanup.hh"

using namespace cadabra;

take_match::take_match(const Kernel& k, Ex& e, Ex& rules_)
	: Algorithm(k, e), rules(rules_)
	{
	// Create a proper substitution rule out of the pattern (otherwise
	// substitute will not swallow it).
	auto wrap = rules.wrap(rules.begin(), str_node("\\arrow"));
	rules.append_child(wrap, str_node("dummy"));
	}

bool take_match::can_apply(iterator it) 
	{
	// Apply only on terms in a top-level sum (NO LONGER: or elements of a top-level list),
	// or on terms inside an integral.
	// The second condition can be relaxed in the future to cover other
	// operators which distribute over sums; for now let's be conservative.
	// Note that any changes here need corresponding changes in replace_match.

	if(*it->name=="\\sum") {
		if(tr.is_head(it) ||
		   *tr.parent(it)->name=="\\int" ||
		   *tr.parent(it)->name=="\\equals") {

			to_keep.clear();
			to_erase.clear();
			
			substitute subs(kernel, tr, rules);
			sibling_iterator sib=tr.begin(it);
			while(sib!=tr.end(it)) {
				if(subs.can_apply(sib)==false)
					to_erase.push_back(sib);
				else
					to_keep.push_back(tr.path_from_iterator(sib, tr.begin()));
				++sib;
				}
			
			// If there is no match whatsoever we cannot apply here.
			if(to_keep.size()==0) return false;
			return true;
			}
		}
	return false;
	}

Algorithm::result_t take_match::apply(iterator& it)
	{
	// Push a copy of the expression onto the history stack. The
	// 'to_keep' is a reference to the paths vector on the associated
	// stack.
	auto itpath = tr.path_from_iterator(it, tr.begin());
	tr.push_history(to_keep);
	
	// Now erase what we do not want anymore, the cleanup
	for(auto& s: to_erase)
		tr.erase(s);
	cleanup_dispatch(kernel, tr, it);
	
	return result_t::l_applied;
	}
