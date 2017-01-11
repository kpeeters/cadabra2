
#include "Cleanup.hh"
#include "Exceptions.hh"
#include "algorithms/expand.hh"
#include "properties/Matrix.hh"

using namespace cadabra;

expand::expand(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool expand::can_apply(iterator it)
	{
	if(*it->name=="\\indexbracket") 
		if(*tr.begin(it)->name=="\\prod") {
			// If we have only one external index, determine whether the first
			// or the last object should be the one with only one index. We
			// do this by checking for the 'Matrix' property on the first and
			// last element.
			one_index=(tr.number_of_children(it)==2);
			sibling_iterator prod=tr.begin(it);
			sibling_iterator sib=tr.begin(prod);
			mx_first=tr.end();
			mx_last=tr.end();
			ii_first=tr.end();
			ii_last=tr.end();

			unsigned int index_open=0;

			while(sib!=tr.end(prod)) {
				const ImplicitIndex *impi=kernel.properties.get_composite<ImplicitIndex>(sib);
				if(impi) {
					const Matrix *mat=kernel.properties.get_composite<Matrix>(sib);
					if(mat) { 
						if(index_open==0) {
							mx_first=sib;
							index_open=2;
							}
						mx_last=sib;
						}
					else {
						if(index_open==0) {
							ii_first=sib;
							mx_first=tr.end();
							index_open=1;
							}
						else {
							ii_last=sib;
							mx_last=tr.end();
							--index_open;
							}
						}
					}
				++sib;
				}
			
			if(index_open+1==tr.number_of_children(it)) return true;
			}
	return false;
	}

Algorithm::result_t expand::apply(iterator& it)
	{
	sibling_iterator prod=tr.begin(it); // the first child of the indexbracket is the product

	// Figure out the type of the indices to be inserted.
	sibling_iterator origind=prod;
	++origind;
	const Indices *dums=kernel.properties.get<Indices>(origind, true);		
	if(!dums)
		throw ConsistencyException("No information about the index types known.");

	// Scan through the factors, adding indexbrackets around any
	// objects which already carry indices, and adding new
	// dummies when necessary.

	sibling_iterator sib=tr.begin(prod);
	Ex dum;
	while(sib!=tr.end(prod)) {
		const ImplicitIndex *impi=kernel.properties.get<ImplicitIndex>(sib);
		sib->fl.bracket=str_node::b_none;
		if(impi) {
			const Matrix *mat=kernel.properties.get_composite<Matrix>(sib);
			sibling_iterator origobj=sib;

			if(tr.number_of_children(sib)>0)
				sib=tr.wrap(sib, str_node("\\indexbracket"));
			if(dum.size()>0) {
				iterator tmpit=tr.append_child((iterator)(sib), dum.begin());
				tmpit->fl.bracket=str_node::b_none;
				tmpit->fl.parent_rel=str_node::p_sub;
				}

			if(mat) { // two-index object
				if(origobj==mx_first) { // put in an open index
					tr.append_child(sib, origind);
					origind=tr.erase(origind);
					}
				if(origobj==mx_last) {
					tr.append_child(sib, origind);
					origind=tr.erase(origind);
					}
				else {
					dum=get_dummy(dums, sib);
					iterator tmpit=tr.append_child((iterator)(sib), dum.begin());
					tmpit->fl.bracket=str_node::b_none;
					tmpit->fl.parent_rel=str_node::p_sub;
					}
				}
			else { // one-index object
				if(origobj==ii_first) {
					dum=get_dummy(dums, sib);
					iterator tmpit=tr.append_child((iterator)(sib), dum.begin());
					tmpit->fl.bracket=str_node::b_none;
					tmpit->fl.parent_rel=str_node::p_sub;
					}
				else dum.clear(); 
				}
			++sib;
			}
		else ++sib;
		}

	it->name=name_set.insert("\\prod").first;
	cleanup_dispatch(kernel, tr, it);

	return result_t::l_applied;
	}

