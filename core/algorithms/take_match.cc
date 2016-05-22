
#include "algorithms/take_match.hh"
#include "algorithms/substitute.hh"
#include "Cleanup.hh"

take_match::take_match(const Kernel& k, Ex& e, Ex& rules_)
	: Algorithm(k, e), rules(rules_)
	{
	}

bool take_match::can_apply(iterator it) 
	{
	if(*it->name=="\\sum" || *it->name=="\\comma") return true;
	return false;
	}

Algorithm::result_t take_match::apply(iterator& it)
	{
	// Push a copy of the expression onto the history stack.

	auto wrap = rules.wrap(rules.begin(), str_node("\\arrow"));
	rules.append_child(wrap, str_node("dummy"));

	tr.push_history(rules);

	substitute subs(kernel, tr, rules);

	sibling_iterator sib=tr.begin(it);
//	int i=0;
	while(sib!=tr.end(it)) {
		if(subs.can_apply(sib)==false) {
			sib=tr.erase(sib);
			}
		else {
			++sib;
			}
		}
	cleanup_dispatch(kernel, tr, it);
	
	return result_t::l_applied;
	}
