
#include "algorithms/sort_product.hh"

sort_product::sort_product(Kernel&k, exptree& tr)
	: Algorithm(k, tr), ignore_numbers_(false)
	{
//	if(has_argument("IgnoreNumbers")) {
//		txtout << "ignoring numbers" << std::endl;
//		ignore_numbers_=true;
//		}
	}

bool sort_product::can_apply(iterator st) 
	{
	if(*st->name=="\\prod" || *st->name=="\\dot") return true;
	else return false;
	}

Algorithm::result_t sort_product::apply(iterator& st) 
	{
	// This could have been done using STL's sort, but then you have to worry
	// about using stable_sort, and then the tree.sort() doesn't do that,
	// and anyhow you would perhaps want exceptions. Let's just use a bubble
	// sort since how many times do we have more than 100 items in a product?

	exptree::sibling_iterator one, two;
	exptree_comparator compare(kernel.properties);

	unsigned int num=tr.number_of_children(st);
	for(unsigned int i=1; i<num; ++i) {
		one=tr.begin(st);
		two=one; ++two;
		for(unsigned int j=i+1; j<=num; ++j) { // this loops too many times, no?
			int es=subtree_compare(&kernel.properties, one, two, -2);
//			std::cerr << "hi " << es << std::endl;
			if(compare.should_swap(one, es)) {
				int canswap=compare.can_swap(one, two, es);
				if(canswap!=0) {
//					std::cout << "swapping ";
//					tr.print_recursive_treeform(std::cout, one);
//					std::cout << " with ";
//					tr.print_recursive_treeform(std::cout, two);
					tr.swap(one);
					std::swap(one,two);  // put the iterators back in order
					if(canswap==-1) {
//						std::cout << "MINUS" << std::endl;
						flip_sign(st->multiplier);
						}
					expression_modified=true;
					}
				}
			++one;
			++two;
			}
		}

	if(expression_modified) return l_applied;
	else return l_no_action;
	}
