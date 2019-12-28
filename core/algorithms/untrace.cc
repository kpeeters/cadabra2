
#include "untrace.hh"
#include "Cleanup.hh"

#include "properties/ImplicitIndex.hh"
#include "properties/Trace.hh"

using namespace cadabra;

untrace::untrace(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool untrace::can_apply(iterator st)
	{
	const Trace *trace = kernel.properties.get<Trace>(st);
	if(trace) {
		auto sib=tr.begin(st);
		if(sib==tr.end(st))      return false;
		if(*sib->name=="\\prod") return true;
		if(is_single_term(sib))  return true;
		}
	return false;
	}

Algorithm::result_t untrace::apply(iterator& trloc)
	{
	auto res=result_t::l_no_action;
	const Trace *trace = kernel.properties.get<Trace>(trloc);

	// We cannot touch any nodes above `trloc`, so
	// we will wrap the trace node in a product and
	// then let cleanup take care of unwrapping
	// that product.
	iterator moveprod=trloc;
	force_node_wrap(moveprod, "\\prod");

	// ensure the argument is a product
	iterator prodloc=tr.begin(trloc);
	prod_wrap_single_term(prodloc);

	// scan arguments of trace
	sibling_iterator st=tr.begin(prodloc);
	while(st!=tr.end(prodloc)) {
		sibling_iterator nxt=st;
		++nxt;

		bool move_out=true;

		auto imp = kernel.properties.get<ImplicitIndex>(st);
		if(imp) {
			// std::cerr << st << " has implicit index" << std::endl;
			if(imp->explicit_form.size()==0) move_out=false;
			else {
				iterator eb=imp->explicit_form.begin();
				index_iterator ii=index_iterator::begin(kernel.properties, eb);
				while(ii!=index_iterator::end(kernel.properties, eb)) {
					auto ind=kernel.properties.get<Indices>(ii, true);
					if(ind) {
						// std::cerr << "seen set name " << ind->set_name << std::endl;
						if(ind->set_name==trace->index_set_name) {
							move_out=false;
							break;
							}
						}
					else {
						// no Indices property known, better be safe.
						move_out=false;
						break;
						}
					++ii;
					}
				}
			}

		if(move_out) {
			res=result_t::l_applied;
			int sign=1;
			sibling_iterator st2=tr.begin(prodloc);
			Ex_comparator compare(kernel.properties);
			while(st2!=st) {
				auto es=compare.equal_subtree(st, st2);
				sign*=compare.can_swap_components(st, st2, es);
				++st2;
				}
			tr.move_before(trloc, st);
			multiply(trloc->multiplier, sign);
			}

		st=nxt;
		}

	// Trace of empty product should become trace of 1.
	if(tr.number_of_children(prodloc)==0)
		node_one(prodloc);

	trloc = moveprod;

//	std::cerr << trloc << std::endl;
	cleanup_dispatch(kernel, tr, trloc);
	
	return res;
	}
