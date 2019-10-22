
#include "Cleanup.hh"
#include "Exceptions.hh"
#include "algorithms/expand.hh"
#include "properties/Matrix.hh"

using namespace cadabra;

expand::expand(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

index_iterator expand::nth_implicit_index(Ex::iterator eform, Ex::iterator iform, unsigned int n)
	{
	unsigned int hits=0;
	index_iterator ch1=begin_index(eform);
	while(ch1!=end_index(eform)) {
		bool found=false;
		index_iterator ch2=begin_index(iform);
		while(!found && ch2!=end_index(iform)) {
			if(*ch1->name==*ch2->name) found=true;
			++ch2;
			}
		if(!found) ++hits;
		if(hits==n) return ch1;
		++ch1;
		}
	return ch1;
	}

bool expand::can_apply(iterator it)
	{
	if(*it->name=="\\indexbracket")
		if(*tr.begin(it)->name=="\\prod") {
			// If we have only one external index, determine whether the first
			// or the last object should be the one with only one index. We
			// do this by checking for the 'Matrix' property on the first and
			// last element.
			check_pos=false;
			one_index=(tr.number_of_children(it)==2);
			sibling_iterator prod=tr.begin(it);
			sibling_iterator sib=tr.begin(prod);
			sibling_iterator ind1=prod;
			++ind1;
			sibling_iterator ind2=ind1;
			if(!one_index) ++ind2;
			const Indices *props=kernel.properties.get<Indices>(ind1, true);
			// For this to stay true, every factor below must come with an
			// index structure. If it is missing even once, we will not be
			// able to guess what the user really meant.
			if(props && props->position_type!=Indices::free) check_pos=true;

			mx_first=tr.end();
			mx_last=tr.end();
			ii_first=tr.end();
			ii_last=tr.end();

			unsigned int index_open=0;

			while(sib!=tr.end(prod)) {
				int tmp;
				auto impi=kernel.properties.get_with_pattern<ImplicitIndex>(sib, tmp);
				if(impi.first) {
					const Matrix *mat=kernel.properties.get_composite<Matrix>(sib);
					if(mat) {
						if(index_open==0) {
							mx_first=sib;
							index_open=2;
							}
						mx_last=sib;
						// If an explicit form exists, verify that it has
						// exactly two more indices than the implicit one.
						if(impi.first->explicit_form.size()>0) {
							Ex::iterator eform=impi.first->explicit_form.begin();
							Ex::iterator iform=impi.second->obj.begin();
							if(tr.number_of_children(eform)-tr.number_of_children(iform)!=2) {
								throw ConsistencyException("Matrices should have two implicit indices.");
								return false;
								}
							}
						else check_pos = false;
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
						// If an explicit form exists, verify that it has
						// exactly one more index than the implicit one.
						if(impi.first->explicit_form.size()>0) {
							Ex::iterator eform=impi.first->explicit_form.begin();
							Ex::iterator iform=impi.second->obj.begin();
							if(tr.number_of_children(eform)-tr.number_of_children(iform)!=1) {
								throw ConsistencyException("Vectors should have one implicit index.");
								return false;
								}
							}
						else check_pos=false;
						}
					}
				++sib;
				}

			if(check_pos && mx_first!=tr.end()) {
				int tmp;
				auto impi=kernel.properties.get_with_pattern<ImplicitIndex>(mx_first, tmp);
				Ex::iterator eform=impi.first->explicit_form.begin();
				Ex::iterator iform=impi.second->obj.begin();
				index_iterator ch1=nth_implicit_index(eform, iform, 1);
				if(ch1->fl.parent_rel!=ind1->fl.parent_rel) {
					throw ConsistencyException("Bracket index structure is not compatible with the factors.");
					return false;
					}
				}
			if(check_pos && mx_last!=tr.end()) {
				int tmp;
				auto impi=kernel.properties.get_with_pattern<ImplicitIndex>(mx_last, tmp);
				Ex::iterator eform=impi.first->explicit_form.begin();
				Ex::iterator iform=impi.second->obj.begin();
				index_iterator ch1=nth_implicit_index(eform, iform, 2);
				if(ch1->fl.parent_rel!=ind2->fl.parent_rel) {
					throw ConsistencyException("Bracket index structure is not compatible with the factors.");
					return false;
					}
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
		int tmp;
		auto impi=kernel.properties.get_with_pattern<ImplicitIndex>(sib, tmp);
		sib->fl.bracket=str_node::b_none;
		if(impi.first) {
			const Matrix *mat=kernel.properties.get_composite<Matrix>(sib);
			sibling_iterator origobj=sib;

			if(tr.number_of_children(sib)>0)
				sib=tr.wrap(sib, str_node("\\indexbracket"));
			if(dum.size()>0) {
				iterator tmpit=tr.append_child((iterator)(sib), dum.begin());
				tmpit->fl.bracket=str_node::b_none;
				if(check_pos) {
					Ex::iterator eform=impi.first->explicit_form.begin();
					Ex::iterator iform=impi.second->obj.begin();
					index_iterator ch=nth_implicit_index(eform, iform, 1);
					tmpit->fl.parent_rel=ch->fl.parent_rel;
					}
				else tmpit->fl.parent_rel=str_node::p_sub;
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
					if(check_pos) {
						Ex::iterator eform=impi.first->explicit_form.begin();
						Ex::iterator iform=impi.second->obj.begin();
						index_iterator ch=nth_implicit_index(eform, iform, 2);
						tmpit->fl.parent_rel=ch->fl.parent_rel;
						}
					else tmpit->fl.parent_rel=str_node::p_sub;
					}
				}
			else {   // one-index object
				if(origobj==ii_first) {
					dum=get_dummy(dums, sib);
					iterator tmpit=tr.append_child((iterator)(sib), dum.begin());
					tmpit->fl.bracket=str_node::b_none;
					if(check_pos) {
						Ex::iterator eform=impi.first->explicit_form.begin();
						Ex::iterator iform=impi.second->obj.begin();
						index_iterator ch=nth_implicit_index(eform, iform, 1);
						tmpit->fl.parent_rel=ch->fl.parent_rel;
						}
					else tmpit->fl.parent_rel=str_node::p_sub;
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

