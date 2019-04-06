
#include "algorithms/zoom.hh"
#include "algorithms/substitute.hh"
#include "Cleanup.hh"

using namespace cadabra;

zoom::zoom(const Kernel& k, Ex& e, Ex& rules_)
	: Algorithm(k, e), rules(rules_)
	{
	// Create a proper substitution rule out of the pattern (otherwise
	// substitute will not swallow it).
	auto wrap = rules.wrap(rules.begin(), str_node("\\arrow"));
	rules.append_child(wrap, str_node("dummy"));
	}

bool zoom::can_apply(iterator it)
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

			return true;
			}
		}
	if(*it->name=="\\ldots")
		return true;
	
	return false;
	}

Algorithm::result_t zoom::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	if(*it->name=="\\ldots") {
		// Simply wrap in another ldots node.
		sibling_iterator sib=it, nxt=it;
		++nxt;
		auto ldots = tr.insert(sib, str_node("\\ldots"));
		tr.reparent(ldots, sib, nxt);
		return res;
		}
	
	// Wrap all things which we want to remove from view in an
	// \ldots node.

	substitute subs(kernel, tr, rules);
	sibling_iterator sib=tr.begin(it);
	bool hiding=false;
	sibling_iterator current_ldots=tr.end(it);
	while(sib!=tr.end(it)) {
		if(subs.can_apply(sib)==false) {
			// Hide this term/factor.
			res=result_t::l_applied;
			auto nxt=sib;
			++nxt;
			if(!hiding) {
				current_ldots=tr.insert(sib, str_node("\\ldots"));
				hiding=true;
				}
			else {
				if(*current_ldots->name!="\\sum") // wrap single term in new sum node.
					current_ldots=tr.wrap(tr.begin(current_ldots), str_node("\\sum"));
				}
			tr.reparent(current_ldots, sib, nxt);
			sib=nxt;
			}
		else {
			// Keep this term/factor visible.
			hiding=false;
			++sib;
			}
		}

	cleanup_dispatch(kernel, tr, it);

	return res;
	}
