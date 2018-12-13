
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
		if(is_single_term(sib))   return true;
		}
	return false;
	}

Algorithm::result_t untrace::apply(iterator& trloc)
	{
	const Trace *trace = kernel.properties.get<Trace>(trloc);
	
	// ensure we sit in a product
	auto par=tr.parent(trloc);
	iterator moveprod=par;
	if(tr.is_head(trloc) || *par->name!="\\prod") {
		moveprod=trloc;
		prod_wrap_single_term(moveprod);
		}

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
			tr.move_before(trloc, st);
			//multiply(it->multiplier, sign);
			}
		
		st=nxt;
		}

	// Trace of empty product should become trace of 1.
	if(tr.number_of_children(prodloc)==0)
		node_one(prodloc);

	trloc = moveprod;
	// std::cerr << trloc << std::endl;
	return result_t::l_no_action;
	}
