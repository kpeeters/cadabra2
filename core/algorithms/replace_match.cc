
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
	auto to_keep=tr.pop_history();

// FIXME: re-do the replacement logic.
	
//	substitute subs(kernel, tr, Ex());
//
//	auto sumnode=it;
//	sibling_iterator sib=tr.begin(sumnode);
//	bool replaced=false;
//	while(sib!=tr.end(sumnode)) {
//		if(subs.can_apply(sib)) {
//			// std::cerr << "applying" << std::endl;
//			if(!replaced) {
//				// Replace the first term that matches with 'current'.
//				replaced=true;
//				iterator ci=tr.end();
//				if(acted_at_head) {
//					ci = tr.insert_subtree(sib, current.begin());
//					}
//				else {
//					// FIXME: make this more robust.
//					auto findsum=current.begin();
//					if(findsum!=current.end()) { // ensure the replacement is not zero
//						if(*findsum->name=="\\int") {
//							findsum=tr.begin(findsum);
//							while(findsum->fl.parent_rel!=str_node::parent_rel_t::p_none) 
//								++findsum;
//							}
////						std::cerr << "replacement tree " << 
//						ci = tr.insert_subtree(sib, findsum);
//						multiply(ci->multiplier, *current.begin()->multiplier/intmult);
//						}
//					}
//				if(ci!=tr.end())
//					cleanup_dispatch(kernel, tr, ci);
//				}
//			sib=tr.erase(sib);
//			}
//		else ++sib;
//		}

	// std::cerr << tr << std::endl;

	cleanup_dispatch(kernel, tr, it);
	
//	std::cerr << tr << std::endl;

	return result_t::l_applied;
	}

