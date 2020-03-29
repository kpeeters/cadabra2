
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
	// This bubble sort is of course a disaster, but it'll have to do for now.

	result_t ret=result_t::l_no_action;
	sibling_iterator one, two;
	unsigned int num=tr.number_of_children(st);
	for(unsigned int i=1; i<num; ++i) {
		one=tr.begin(st);
		two=one;
		++two;
		for(unsigned int j=i+1; j<=num; ++j) { // this loops too many times, no?
			int es=subtree_compare(&kernel.properties, one, two, -2, true, 0, true);
			if(should_swap(one, es)) {
				tr.swap(one);
				std::swap(one,two);  // put the iterators back in order
				ret=result_t::l_applied;
				}
			++one;
			++two;
			}
		}

	return ret;
	}

bool sort_sum::should_swap(iterator obj, int subtree_comparison) const
	{
	sibling_iterator one=obj, two=obj;
	++two;

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

