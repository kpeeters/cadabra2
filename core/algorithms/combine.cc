
#include "Cleanup.hh"
#include "algorithms/combine.hh"
#include "properties/Matrix.hh"

using namespace cadabra;

combine::combine(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool combine::can_apply(iterator it)
	{
	if(*it->name=="\\prod") return true;
	return false;
	}

Algorithm::result_t combine::apply(iterator& it)
	{
	sibling_iterator sib=tr.begin(it);
	index_map_t ind_free, ind_dummy;
	while(sib!=tr.end(it)) {
		sibling_iterator ch=tr.begin(sib);
		while(ch!=tr.end(sib)) {
			if(ch->fl.parent_rel==str_node::p_sub || ch->fl.parent_rel==str_node::p_super) {
				classify_add_index(ch, ind_free, ind_dummy);
				}
			++ch;
			}
		++sib;
		}
	if(ind_dummy.size()==0) return result_t::l_no_action;

	index_map_t::iterator dums1=ind_dummy.begin(), dums2;
	while(dums1!=ind_dummy.end()) {
//		txtout << "analysing " << std::endl;
//		txtout << *(dums1->second->name) << std::endl;
		dums2=dums1;
		++dums2;

		bool isbrack1=false, isbrack2=false;
		bool ismatorvec1=false, ismatorvec2=false;
		const Matrix *mat1=kernel.properties.get<Matrix>(tr.parent(dums1->second));
		if(mat1)
			ismatorvec1=true;
		else if(*(tr.parent(dums1->second)->name)=="\\indexbracket") {
			ismatorvec1=true; isbrack1=true;
			}
		else if(tr.number_of_children(tr.parent(dums1->second))==1) 
			ismatorvec1=true;
		const Matrix *mat2=kernel.properties.get<Matrix>(tr.parent(dums2->second));
		if(mat2)
			ismatorvec2=true;
		else if(*(tr.parent(dums2->second)->name)=="\\indexbracket") {
			ismatorvec2=true; isbrack2=true;
			}
		else if(tr.number_of_children(tr.parent(dums2->second))==1) 
			ismatorvec2=true;

		if(ismatorvec1 && ismatorvec2) {
//			txtout << "gluing " << *(dums2->second->name) << std::endl;
			// create new indexbracket with product node
			iterator outerbrack=tr.insert(tr.parent(dums1->second), str_node("\\indexbracket"));
			iterator brackprod=tr.append_child(outerbrack, str_node("\\prod"));
			iterator parn1=tr.parent(dums1->second);
			iterator parn2=tr.parent(dums2->second);
			// remove the dummy index from these two objects, and move
			// the non-dummy indices to the outer indexbracket.
			sibling_iterator ind1=tr.begin(tr.parent(dums1->second));
			sibling_iterator stop1=tr.end(tr.parent(dums1->second));
			if(isbrack1)
				++ind1;
			while(ind1!=stop1) {
				if(ind1!=dums1->second) {
					tr.append_child(outerbrack, iterator(ind1));
					}
				++ind1;
//				ind1=tr.erase(ind1);
				}
			tr.erase(dums1->second);
			sibling_iterator ind2=tr.begin(tr.parent(dums2->second));
			sibling_iterator stop2=tr.end(tr.parent(dums2->second));
			if(isbrack2)
				++ind2;
			while(ind2!=stop2) {
				if(ind2!=dums2->second) {
					tr.append_child(outerbrack, iterator(ind2));
					}
				++ind2;
//				ind2=tr.erase(ind2);
				}
			tr.erase(dums2->second);

			// put both objects inside the indexbracket.
			if(isbrack1) {
				sibling_iterator nxt=tr.begin(parn1);
				++nxt;
//				tr.begin(parn1)->fl.bracket=str_node::b_round;
				tr.reparent(brackprod, tr.begin(parn1), nxt);
				multiply(brackprod->multiplier, *parn1->multiplier);
				tr.erase(parn1);
				}
			else {
				sibling_iterator nxt=parn1;
				++nxt;
//				parn1->fl.bracket=str_node::b_round;
				tr.reparent(brackprod,parn1,nxt);
				}
			if(isbrack2) {
				sibling_iterator nxt=tr.begin(parn2);
				++nxt;
//				tr.begin(parn2)->fl.bracket=str_node::b_round;
				tr.reparent(brackprod, tr.begin(parn2), nxt);
				multiply(brackprod->multiplier, *parn2->multiplier);
				tr.erase(parn2);
				}
			else {
				sibling_iterator nxt=parn2;
				++nxt;
//				parn2->fl.bracket=str_node::b_round;
				tr.reparent(brackprod,parn2,nxt);
				}
			}
		++dums1;
		++dums1;
		}

	std::cerr << it << std::endl;

//	prodflatten pf(tr, tr.end());
//	pf.apply_recursive(it, false);

	cleanup_dispatch(kernel, tr, it);

	return result_t::l_applied;
	}
