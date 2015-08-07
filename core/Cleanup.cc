
#include "Cleanup.hh"
#include "Algorithm.hh"
#include "algorithms/prodcollectnum.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/flatten_sum.hh"
#include "properties/Derivative.hh"

void cleanup_dispatch(Kernel& kernel, Ex& tr, Ex::iterator& it)
	{
//	std::cout << "dispatch at " << *it->name << "\n";
	if(it->is_zero())  {
		::zero(it->multiplier);
		tr.erase_children(it);
		it->name=name_set.insert("1").first;
		}
	else if(*it->name=="\\prod")       cleanup_productlike(kernel, tr, it);
	else if(*it->name=="\\sum")        cleanup_sumlike(kernel, tr, it);
	else if(*it->name=="\\expression") cleanup_expressionlike(kernel, tr, it);

	const Derivative *der = kernel.properties.get<Derivative>(it);
	if(der) cleanup_derivative(kernel, tr, it);
	}

void cleanup_productlike(Kernel& k, Ex&tr, Ex::iterator& it)
	{
	assert(*it->name=="\\prod");

	if(tr.number_of_children(it)==1)
		if(tr.begin(it)->is_range_wildcard())
			return;

	// Remove children which are 1
   // Collect all multipliers
	prodcollectnum pc(k, tr);
	pc.apply(it);

	// FIXME: why does prodcollectnum not find the 0?
	// FIXME: do not call algorithms here to avoid recursion
	if(it->is_zero()) {
		::zero(it->multiplier);
		tr.erase_children(it);
		it->name=name_set.insert("1").first;
		}
	}

void cleanup_sumlike(Kernel& k, Ex&tr, Ex::iterator& it)
	{
	assert(*it->name=="\\sum");

	if(tr.number_of_children(it)==1)
		if(tr.begin(it)->is_range_wildcard())
			return;

	// Flatten sums which are supposed to be flat.
	// FIXME: this does too much and we should not be using Algorithm
	// objects here anyway.
	flatten_sum fs(k,tr);
	fs.apply(it);

	// Remove children which are 0
	Ex::sibling_iterator sib=tr.begin(it);
	while(sib!=tr.end(it)) {
		if(sib->is_zero())
			sib=tr.erase(sib);
		else
			++sib;
		}

   // Collect all equal terms.
	collect_terms ct(k, tr);
	ct.apply(it);
	}

void cleanup_expressionlike(Kernel& k, Ex&tr, Ex::iterator& it)
	{
	assert(*it->name=="\\expression");

	// Can only have one child which contains actual expression data;
	// all others are meta-data like \asymimplicit. If this child has
	// zero multiplier, simplify.

	Ex::sibling_iterator sib=tr.begin(it);
	if(sib==tr.end(it)) return;
	if(sib->is_zero()) {
		// FIXME: duplicate of node_zero in Algorithm.
		tr.erase_children(sib);
		sib->name=name_set.insert("1").first;
		}
	}

void cleanup_derivative(Kernel& k, Ex& tr, Ex::iterator& it)
	{
	// Nested derivatives with the same name should be flattened, but
	// only if both the outer derivative and the inner derivative have
	// an index (otherwise D(D(A)) becomes D(A) which is wrong).

	// Find first non-index child.

	Ex::sibling_iterator sib=tr.begin(it);
	if(sib==tr.end(it)) return;

	while(sib->is_index()) {
		++sib;
		if(sib==tr.end(it))
			throw ConsistencyException("Encountered Derivative object without argument on which to act.");
		// FIXME: the above is not correct when a derivative is declared without argument,
		// like in \Omega::Derivative; A::Depends(\Omega).
		}

	// FIXME: this ignores that derivatives can have functional child
	// nodes which are interpreted as 'object wrt. with derivative should be taken'.

	if(it->name == sib->name) {
		if(Algorithm::number_of_direct_indices(it)>0 && Algorithm::number_of_direct_indices(sib)>0) {
			multiply(it->multiplier, *sib->multiplier);
			tr.flatten(sib);
			tr.erase(sib);
			}
		}
	}

void cleanup_dispatch_deep(Kernel& k, Ex& tr, dispatcher_t dispatch)
	{
	// Cleanup the entire tree starting from the deepest nodes and
	// working upwards. 

	// This duplicates work of Algorithm::apply, but we want to have an
	// independent cleanup unit which does not rely on things we may
	// want to change in Algorithm::apply in the future, and we do not
	// want to make recursive calls into that function either. And it is
	// simple enough anyway.

	Ex::post_order_iterator it=tr.begin();
	it.descend_all();
	Ex::post_order_iterator last=tr.begin();
	while(it!=last) {
		Ex::post_order_iterator next=it;
		++next;
		Ex::iterator tmp=it;
		dispatch(k, tr, tmp);
		it=next;
		}
	}

