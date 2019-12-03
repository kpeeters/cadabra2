
#include "Cleanup.hh"
#include "algorithms/sort_product.hh"
#include "properties/Trace.hh"

using namespace cadabra;

// #define DEBUG 1

sort_product::sort_product(const Kernel&k, Ex& tr)
	: Algorithm(k, tr), cleanup(true)
	{
	//	if(has_argument("IgnoreNumbers")) {
	//		txtout << "ignoring numbers" << std::endl;
	//		ignore_numbers_=true;
	//		}
	}

void sort_product::dont_cleanup()
	{
	cleanup=false;
	}

bool sort_product::can_apply(iterator st)
	{
	if(*st->name=="\\prod" || *st->name=="\\inner" || *st->name=="\\wedge") {
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
	compare.set_value_matches_index(true);

	//	std::cerr << "sorting\n" << Ex(st) << std::endl;
	//	std::cout << "entering sort" << std::endl;
	//	tr.print_recursive_treeform(std::cout, st);

	unsigned int num=tr.number_of_children(st);
	for(unsigned int i=1; i<num; ++i) {
		one=tr.begin(st);
		two=one;
		++two;
		//		for(unsigned int j=i+1; j<=num; ++j) { // this loops too many times, no?
		while(two!=tr.end(st)) {
			compare.clear();
			auto es = compare.equal_subtree(one, two);
			if(compare.should_swap(one, es)) {
#ifdef DEBUG
				std::cerr << "should swap " << *(one->name) << " with " << *(two->name) << std::endl;
#endif
				int canswap=compare.can_swap(one, two, es);
#ifdef DEBUG
				std::cerr << "can swap? " << *(one->name) << " with " << *(two->name) << std::endl;
#endif
				if(canswap!=0) {
					// std::cerr << "swapping " << Ex(one) << " with " << Ex(two) << std::endl;
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

	if(ret==result_t::l_no_action) {
		bool found=false;
		bool ascend=true;
		iterator parn=st;
		// It is fine to cycle when we have a sum of a sum inside a trace
		while(ascend && !tr.is_head(parn)) {
			parn=tr.parent(parn);
			const Trace *trace=kernel.properties.get<Trace>(parn);
			if(trace) {
				ascend=false;
				found=true;
				}
			else if(*parn->name=="\\indexbracket") {
				ascend=false;
				if(number_of_indices(parn)==2) {
					index_iterator first=begin_index(parn);
					index_iterator last=first;
					++last;
					if(*first->name==*last->name) found=true;
					}
				}
			else if(*parn->name!="\\sum") {
				ascend=false;
				}
			}
		if(found) {
			// Construct all cyclic orderings
			std::vector<std::vector<Ex::sibling_iterator>> candidates;
			one=tr.begin(st);
			while(one!=tr.end(st)) {
				two=one;
				++two;
				if(two==tr.end(st)) two=tr.begin(st);
				std::vector<Ex::sibling_iterator> candidate(1, one);
				while(two!=one) {
					candidate.push_back(two);
					++two;
					if(two==tr.end(st)) two=tr.begin(st);
					}
				candidates.push_back(candidate);
				++one;
				}
			// Narrow them down by comparing first digit, then second digit, ...
			unsigned int digit=1;
			while(candidates.size()>1 && digit<=num) {
				std::vector<std::vector<Ex::sibling_iterator>>::iterator candidate=candidates.begin();
				one=candidate->at(digit-1);
				++candidate;
				while(candidate!=candidates.end()) {
					two=candidate->at(digit-1);
					compare.clear();
					auto es=compare.equal_subtree(one, two);
					if(es==Ex_comparator::match_t::no_match_greater || es==Ex_comparator::match_t::match_index_greater) {
						candidate=candidates.erase(candidates.begin(), candidate);
						one=candidate->at(digit-1);
						++candidate;
						}
					else if(es==Ex_comparator::match_t::no_match_less || es==Ex_comparator::match_t::match_index_less) {
						candidate=candidates.erase(candidate);
						}
					else ++candidate;
					}
				++digit;
				}
			// Use the first ordering but keep track of signs this time
			Ex::sibling_iterator front=candidates.at(0).at(0);
			if(front!=tr.begin(st)) {
				while(tr.begin(st)!=front) {
					one=tr.begin(st);
					two=one;
					++two;
					while(two!=tr.end(st)) {
						compare.clear();
						auto es=compare.equal_subtree(one, two);
						int sign=compare.can_swap_components(one, two, es);
						if(sign==-1) flip_sign(st->multiplier);
						tr.swap(one);
						++two;
						++two;
						}
					}
				ret=result_t::l_applied;
				}
			}
		}

	if(cleanup)
		cleanup_dispatch(kernel, tr, st);

	return ret;
	}
