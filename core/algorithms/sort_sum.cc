
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
	/*
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
	*/
	result_t ret=result_t::l_no_action;

	std::vector<sibling_iterator> sibs;
	sibling_iterator sib;
	unsigned int num=tr.number_of_children(st);
	sib = tr.begin(st);
	// Add all the sibling iterators
	for (unsigned int i=0; i < num; i++) {
		sibs.push_back(sib);
		++sib;
		}
	// sort them
	std::stable_sort(sibs.begin(), sibs.end(), 
		[this](const sibling_iterator& sib1, const sibling_iterator& sib2) {
			int es=subtree_compare(&kernel.properties, sib1, sib2, -2, true, 0, true);
			return !should_swap(sib1, sib2, es);
			});

	// check if anything actually moved. any better way?
	sib=tr.begin(st);
	for (unsigned int i=0; i<num; i++) {
		if (sib != sibs[i]) {
			ret=result_t::l_applied;
			break;
			}
		++sib;
		}

	// rebuild the tree
	st.node->first_child = sibs[0].node;
	st.node->last_child = sibs[num-1].node;
	for (unsigned int i = 0; i < num-1; i++) {
		sibs[i+1].node->prev_sibling = sibs[i].node;
		sibs[i].node->next_sibling = sibs[i+1].node;
		}
	sibs[0].node->prev_sibling = 0;
	sibs[num-1].node->next_sibling = 0;

	return ret;

	/* alternative option using tree_nodes directly (about the same runtime)

	// just put the references of the nodes in the vector
	// maybe that's a bit faster?
	typedef tree_node_<str_node> tree_node;
	unsigned int num=tr.number_of_children(st);
	std::vector<tree_node *> nodes(num);

	// Add all the sibling iterators
	sibling_iterator sib = tr.begin(st);
	for (unsigned int i=0; i < num; i++) {
		nodes[i] = sib.node;
		++sib;
		}
	// sort them
	std::stable_sort(nodes.begin(), nodes.end(), 
		[this](tree_node *a, tree_node *b) {
			sibling_iterator sib1 = sibling_iterator(a);
			sibling_iterator sib2 = sibling_iterator(b);
			int es=subtree_compare(&kernel.properties, sib1, sib2, -2, true, 0, true);
			return !should_swap(sib1, sib2, es);
			});
	
	// check if anything actually sorted
	sib=tr.begin(st);
	for (unsigned int i=0; i<num; i++) {
		if (sib.node != nodes[i]) {
			ret=result_t::l_applied;
			break;
			}
		++sib;
		}

	// rebuild the tree
	st.node->first_child = nodes[0];
	st.node->last_child = nodes[num-1];
	for (unsigned int i = 0; i < num-1; i++) {
		nodes[i+1]->prev_sibling = nodes[i];
		nodes[i]->next_sibling = nodes[i+1];
		}
	nodes[0]->prev_sibling = 0;
	nodes[num-1]->next_sibling = 0;

	*/

	}


/* no longer necessary

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
*/

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


