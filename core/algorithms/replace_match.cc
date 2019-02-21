
#include "Cleanup.hh"
#include "algorithms/replace_match.hh"
#include "algorithms/substitute.hh"

using namespace cadabra;

replace_match::replace_match(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool replace_match::can_apply(iterator) 
	{
	if(tr.history_size()>0) return true;
	return false;
	}

Algorithm::result_t replace_match::apply(iterator& it)
	{
	// Preserve the expression before popping. After this, the 'tr' is
	// the original expression from before 'take_match'.
	Ex current(tr);
	auto to_keep=tr.pop_history();
	if(to_keep.size()==0) {
		return result_t::l_applied;
		}

	// Remove the terms which we will replace, by converting
	// the 'to_keep' paths above to iterators, then removing.
	iterator sum_node = tr.parent(tr.iterator_from_path(to_keep[0], tr.begin()));
	std::vector<iterator> to_erase;
	for(const auto& p: to_keep)
		to_erase.push_back( tr.iterator_from_path(p, tr.begin()) );
	for(auto& erase: to_erase)
		tr.erase(erase);

	// If the replacement is zero, there is nothing to substitute.
	if(!current.begin()->is_zero()) {

		// We already have an iterator to the sum node in the now-current
		// expression (sum_node). We also need one to the sum node in the
		// replacement sum.
		iterator replacement_sum_node = current.iterator_from_path(tr.path_from_iterator(sum_node, tr.begin()), current.begin());
		
		// If the original sum has disappeared (because subsequent manipulations
		// made all but one terms vanish), wrap it again in a sum.
		if(*replacement_sum_node->name!="\\sum") 
			replacement_sum_node = current.wrap(replacement_sum_node, str_node("\\sum"));
		
		// If we are inside an integral, determine the \int multiplier in the original
		// and in the replacement.
		multiplier_t rescale=1;
		if(!tr.is_head(it) && *tr.parent(it)->name=="\\int") {
			multiplier_t orig_mult = *tr.parent(it)->multiplier;
			multiplier_t repl_mult = *current.parent(replacement_sum_node)->multiplier;
			rescale = repl_mult/orig_mult;
			}
		
		sibling_iterator repit=current.begin(replacement_sum_node);
		while(repit!=current.end(replacement_sum_node)) {
			multiply( tr.append_child(sum_node, iterator(repit))->multiplier, rescale);
			++repit;
			}
		}
	
	cleanup_dispatch(kernel, tr, it);
	return result_t::l_applied;
	}

