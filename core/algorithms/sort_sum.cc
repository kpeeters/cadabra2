
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


	if (algochoice == 1) {
		// Modified bubble sort to keep track of last sort location
		// Best case behaviour is now O(n)
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
		unsigned int num=tr.number_of_children(st);
		bool result = insertionSort(tr.begin(st), num);
		if (result) {
			ret=result_t::l_applied;
			} 

		return ret;
		}



	if (algochoice == 3) {
		// timSort (simple version)
		bool result = timSort(st);
		if (result) {
			ret=result_t::l_applied;
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


bool sort_sum::insertionSort(sibling_iterator step, unsigned int num)
	{
	bool result = false;			// boolean to record if a swap happened
	sibling_iterator one, two;

	// step tracks the beginning of the unsorted list
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
				result = true;
				}
			else {
				// 'two' is in place so [0,i] is now sorted
				j=1;
				}
			--two;
			}
		}
	return result;
	}

bool sort_sum::timSort(iterator& st)
	{
	// Simple implementation of timSort, stolen from https://www.geeksforgeeks.org/timsort/

	// This version is unoptimized (e.g. no gallopping),
	// so it is just mergeSort, with insertionSort on the "base" case of minRun length lists.
	// This reduces to insertionSort on sums of fewer than 32 terms.

	unsigned int num=tr.number_of_children(st);

	bool result = false;
	sibling_iterator sub_start;
	unsigned int minRun = calcMinRun(num);
	unsigned int l, r, m;

	// Break original list into sublists of nearly equal size minRun
    // minRun is computed to lie in (16, 32)
	// 
    for (unsigned int i = 0; i < num; i += minRun) {
		sub_start = tr.begin(st);
		sub_start += i;
        result = insertionSort(sub_start,  std::min(minRun, num-i)) || result;
		// Note: swapping order of above || seems to lead to stupid compiler "optimization"
		// where it skips calling insertionSort if result = true
		}

	// Merge successive sublists
	unsigned int size = minRun;
	while (size < num) {
		for (l = 0; l < num; l += 2*size) {
			// m = std::min(num-1, l + size - 1);
			// above min is not logically necessary because then m >= r
			m = l+size-1;
			r = std::min(l+2*size-1, num-1);
			if (m < r) {
				result = merge(tr.begin(st), l, m, r) || result;
				}
			}
		size = 2 * size;
		}

	return result;
	}

bool sort_sum::merge(sibling_iterator start1, unsigned int l, unsigned int m, unsigned int r)
	{
	// merge acts on a subset of an "array" beginning at start1
	// the "subarrays" indexed by (l, ..., m) and (m+1, ..., r) are presumed sorted
	// returns a sorted "subarray" indexed by (l, ..., r)
	bool result = false;
	sibling_iterator mid, start2, temp;
	
	start1 += l;

	mid = start1;
	mid += m-l;

	start2 = mid;
	start2 += 1;

	// start1 lies at index l
	// mid lies at index m
	// start2 lies at index m+1
	
	// if mid < start2, the range is already fully sorted
	int es=subtree_compare(&kernel.properties, mid, start2, -2, true, 0, true);
	if(!should_swap(mid, es)) {
		return result;
		}

    while (l <= m && m+1 <= r) {
		// Within the loop, the subarray can be thought of as:
		// (final sorted piece) (left remainder) (right remainder)
		// l tracks start1, at beginning of left remainder
		// m tracks mid, at end of left remainder
		// m+1 tracks start2, at beginning of right remainder

		// If start1 is smaller than start2, 
		// final sorted piece absorbs the first element of left remainder
		es=subtree_compare(&kernel.properties, start1, start2, -2, true, 0, true);
        if (!should_swap(start1, start2, es)) {
			// advance the index and the element
			++l;
            ++start1;
			}
		// otherwise, move start2 to lie before start1
        else {
			// grab the next element past start2
			temp = start2;
			++temp;
			// put start2 before start1
			tr.move_before(start1, start2);
            ++l;
            ++m;
			// don't bother updating mid b/c we don't need it anymore
            start2 = temp;
			result = true;
			}
		}
	return result;
	}

unsigned int sort_sum::calcMinRun(unsigned int n)
	{
	/* Compute a good value for the minimum run length; natural runs shorter
	 * than this are boosted artificially via binary insertion.
	 *
	 * If n < RUN, return n (it's too small to bother with fancy stuff).
	 * Else if n is an exact power of 2, return RUN/2.
	 * Else return an int k, RUN/2 <= k <= RUN, such that n/k is close to, but
	 * strictly less than, an exact power of 2.
	 * 
	 * Python's timSort uses RUN = 64. Set it to 32 here.
	 * 
	 */
	const unsigned int RUN = 32;
	unsigned int r = 0;
	while (n >= RUN) {
		r |= (n & 1); 
		n >>= 1; 
        } 
	return n + r;
	}

