
#include "algorithms/take_match.hh"
#include "algorithms/substitute.hh"
#include "Cleanup.hh"

using namespace cadabra;

take_match::take_match(const Kernel& k, Ex& e, Ex& rules_)
	: Algorithm(k, e), rules(rules_)
	{
	}

bool take_match::can_apply(iterator it) 
	{
	// Apply only on terms in a top-level sum or elements of a top-level list,
	// or on terms inside an integral.
	// The second condition can be relaxed in the future to cover other
	// operators which distribute over sums; for now let's be conservative.
	// Note that any changes here need corresponding changes in replace_match.

	std::cerr << *it->name << std::endl;
	if(tr.is_head(it) && (*it->name=="\\sum" || *it->name=="\\comma")) return true; // sums at top level
	if(!tr.is_head(it)) {
		if(*it->name=="\\sum" && *tr.parent(it)->name=="\\int") return true; // sums as arguments of integrals
		if(*it->name=="\\sum" && *tr.parent(it)->name=="\\equals") return true; // sum as lhs or rhs of equation
		}
	return false;
	}

Algorithm::result_t take_match::apply(iterator& it)
	{
	std::cerr << "applying at " << it << std::endl;
	// Push a copy of the expression onto the history stack.

//	auto wrap = rules.wrap(rules.begin(), str_node("\\arrow"));
//	rules.append_child(wrap, str_node("dummy"));

	auto to_keep = tr.push_history();

	substitute subs(kernel, tr, rules);
	sibling_iterator sib=tr.begin(it);
	std::vector<sibling_iterator> to_erase;
	while(sib!=tr.end(it)) {
		if(subs.can_apply(sib)==false)
			to_erase.push_back(sib);
		else
			to_keep.push_back(tr.path_from_iterator(sib));
		++sib;
		}
	std::cerr << "to erase " << to_erase.size() << " terms" << std::endl;
	for(auto& s: to_erase)
		tr.erase(s);

	cleanup_dispatch(kernel, tr, it);
	
	return result_t::l_applied;
	}
