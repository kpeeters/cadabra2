
#include "algorithms/sort_product.hh"

using namespace cadabra;

sort_product::sort_product(const Kernel&k, Ex& tr)
	: Algorithm(k, tr), ignore_numbers_(false)
	{
//	if(has_argument("IgnoreNumbers")) {
//		txtout << "ignoring numbers" << std::endl;
//		ignore_numbers_=true;
//		}
	}

bool sort_product::can_apply(iterator st) 
	{
	if(*st->name=="\\prod" || *st->name=="\\dot" || *st->name=="\\wedge") {
		// ensure that there are no factors with object or name wildcards, as we
		// cannot know what they would match to (in general, sort_product does
		// not make much sense acting on patterns).

		sibling_iterator sib=tr.begin(st);
		while(sib!=tr.end(st)) {
			if(sib->is_name_wildcard() || sib->is_object_wildcard())
				return false;
			++sib;
			}
		return true;
		}
	else return false;
	}

Algorithm::result_t sort_product::apply(iterator& st) 
	{
	// This could have been done using STL's sort, but then you have to worry
	// about using stable_sort, and then the tree.sort() doesn't do that,
	// and anyhow you would perhaps want exceptions. Let's just use a bubble
	// sort since how many times do we have more than 100 items in a product?

	result_t ret=result_t::l_no_action;

	Ex::sibling_iterator one, two;
	Ex_comparator compare(kernel.properties);

//	std::cerr << "sorting\n" << Ex(st) << std::endl;
//	std::cout << "entering sort" << std::endl;
//	tr.print_recursive_treeform(std::cout, st);

	unsigned int num=tr.number_of_children(st);
	for(unsigned int i=1; i<num; ++i) {
		one=tr.begin(st);
		two=one; ++two;
//		for(unsigned int j=i+1; j<=num; ++j) { // this loops too many times, no?
		while(two!=tr.end(st)) {
			compare.clear();
			auto es = compare.equal_subtree(one, two);
			if(compare.should_swap(one, es)) {
//				std::cerr << "should swap " << *(one->name) << " with " << *(two->name) << std::endl;
				int canswap=compare.can_swap(one, two, es);
//				std::cerr << "can swap? " << *(one->name) << " with " << *(two->name) << std::endl;
				if(canswap!=0) {
//					std::cerr << "swapping " << Ex(one) << " with " << Ex(two) << std::endl;
					tr.swap(one);
					std::swap(one,two);  // put the iterators back in order
					if(canswap==-1) {
//						std::cout << "MINUS" << std::endl;
						flip_sign(st->multiplier);
						}
					ret=result_t::l_applied;
					}
				}
			++one;
			++two;
			}
		}

	return ret;
	}
