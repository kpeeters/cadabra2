
#include "properties/SortOrder.hh"
#include "algorithms/sort_sum.hh"

using namespace cadabra;

sort_sum::sort_sum(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool sort_sum::can_apply(iterator st)
	{
	if(*st->name=="\\sum") return true;
	else return false;
	}

Algorithm::result_t sort_sum::apply(iterator& st)
	{
	bool changed = tr.subtree_sort(st.begin(), st.end(), 
		[this](const sibling_iterator& sib1, const sibling_iterator& sib2) {
            /* The less function for stable_sort requires less(a,a) = false.
             * Simplest fix is to swap order of arguments below.
             */
			int es=subtree_compare(&kernel.properties, sib2, sib1, -2, true, 0, true);
			return should_swap(sib2, sib1, es);
			});

	// check if anything actually moved.
    if (changed) return result_t::l_applied;
	else return result_t::l_no_action;
	}


bool sort_sum::should_swap(iterator obj1, iterator obj2, int subtree_comparison) const
	{
	sibling_iterator one=obj1, two=obj2;

	// Find a SortOrder property which contains both one and two.
	int num1, num2;
	const SortOrder *so1=kernel.properties.get<SortOrder>(one,num1);
	const SortOrder *so2=kernel.properties.get<SortOrder>(two,num2);

	if(so1==0 || so2==0) { // No sort order known
		if(subtree_comparison<0) return true;
		return false;
		}
	else if(abs(subtree_comparison)<=1) {   // Identical up to index names
		if(subtree_comparison==-1) return true;
		return false;
		}
	else {
		if(so1==so2) {
			if(num1>num2) return true;
			return false;
			}
		}

	return false;
	}