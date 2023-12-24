
#include "properties/SortOrder.hh"
#include "algorithms/sort_sum.hh"

using namespace cadabra;

sort_sum::sort_sum(const Kernel& k, Ex& e, int algochoice)
	: Algorithm(k, e), algochoice(algochoice)
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
	
	if (algochoice == 0) {
		// Original implementation
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
		} 

	// Modified bubble sort to keep track of last sort location
	// Best case behaviour is now O(n)
	// Worst case performance probably slightly worse because of additional overhead

	if (algochoice == 1) {
		// upper_limit denotes where the sorted section of the list begins
		sibling_iterator one, two;
		unsigned int upper_limit = tr.number_of_children(st);
		unsigned int swap_pos = 0;
		while (upper_limit > 1) 
			{
			one=tr.begin(st);
			two=one;
			++two;

			// swap_pos will eventually denote the larger index of the two items swapped
			swap_pos = 0;
			// j == indexed position of `one`
			for (unsigned int j=0;  j < upper_limit-1 ; j++) {
				int es=subtree_compare(&kernel.properties, one, two, -2, true, 0, true);
				if(should_swap(one, es)) {
					tr.swap(one);
					std::swap(one,two);  // put the iterators back in order
					ret=result_t::l_applied;
					// store the position of `two` in swap_pos
					swap_pos = j+1;
					}
				++one;
				++two;
				}
			// upper_limit is set to the position of `two` in the last swap
			upper_limit = swap_pos;
			// done if swap_pos is either 0 (no swap) or 1 (swapped first two elements)
			}
		}

	if (algochoice == 2) {
		// insertion sort 

		// This is a slight modification of insertion sort, since instead of copying elements over
		// each other, we will "bubble" them into place. This generally involves fewer comparisons
		// than any version of bubble sort.

		sibling_iterator one, two, step;

		unsigned int num=tr.number_of_children(st);

		// step tracks the beginning of the unsorted list
		step = tr.begin(st);
		// first element is added to the sorted list, now of length 1
		++step;

		// continue growing the sorted list by bubbling the next element down to where it goes
		// i is tracking the "index" of 'step'
		for (unsigned int i = 1; i < num; i++) {
			two = step;
			++step;
			// the sorted/unsorted interface is now
			//  (sorted)   two    step
			//    [0,i-1]   i      i+1
			// 'two' will now be sorted into it
			
			// j tracks 'two' as we "bubble" it down into the ordered list until it finds its place
			for (unsigned int j=i; j>=1; j--) {
				one = two;
				--one;

				// compare 'one' and 'two'
				int es=subtree_compare(&kernel.properties, one, two, -2, true, 0, true);
				if(should_swap(one, es)) {
					tr.swap(one);
					std::swap(one,two);  // put the iterators back in order
					ret=result_t::l_applied;
					}
				else {
					// 'two' is in place so [0,i] is now sorted
					j=1;
					}
				--two;
				}
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

