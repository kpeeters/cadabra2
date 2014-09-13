
#include "algorithms/keep_terms.hh"

keep_terms::keep_terms(Kernel& k, exptree& tr)
	: Algorithm(k, tr)
	{
	}

bool keep_terms::can_apply(iterator it)
	{
	if(*it->name!="\\sum") return false;
	if(number_of_args()!=1 && number_of_args()!=2) return false;
	return true;
	}

Algorithm::result_t keep_terms::apply(iterator& it)
	{
	sibling_iterator argit=args_begin();
	unsigned long firstnode=to_long(*argit->multiplier);
	long lastnode=-2;
	if(number_of_args()==2) {
		++argit;
		lastnode=to_long(*argit->multiplier);
		}
	sibling_iterator cut1=tr.begin(it);
	assert(firstnode<tr.number_of_children(it));

	assert(firstnode>=0);
	while(firstnode>0) {
		expression_modified=true;
		cut1=tr.erase(cut1);
		--firstnode;
		--lastnode;
		}
	++lastnode;
	if(lastnode>0) {
		while(lastnode>0) {
			if(cut1==tr.end()) 
				break;
			++cut1;
			--lastnode;
			}
		while(cut1!=tr.end(it)) {
			expression_modified=true;
			cut1=tr.erase(cut1);
			}
		}
	
	cleanup_sums_products(tr,it);
	return l_applied;
	}

