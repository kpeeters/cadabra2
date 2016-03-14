
#include "Compare.hh"
#include "Storage.hh"

int main()
	{
	Ex a1("A"), a2("A");

	a1.begin()->fl.parent_rel=str_node::p_sub;
	a2.begin()->fl.parent_rel=str_node::p_super;

	tree_exact_less_for_indexmap_obj compare;

	std::cerr << compare(a2, a1) << std::endl;
	assert(compare(a1, a2)!=compare(a2,a1)); 
	}
