
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
	std::vector<Ex::iterator> dummies;
	while(sib!=tr.end(it)) {  // iterate over all factors in the product
		sibling_iterator ch=tr.begin(sib);
		while(ch!=tr.end(sib)) { // iterate over all indices of this factor
			//			auto parent=tr.parent(sib);
			if(ch->fl.parent_rel==str_node::p_sub || ch->fl.parent_rel==str_node::p_super) {
				classify_add_index(ch, ind_free, ind_dummy);
				}
			++ch;
			}
		++sib;
		}
	if(ind_dummy.size()==0) return result_t::l_no_action;

	while(ind_dummy.begin()!=ind_dummy.end()) {
		bool found=false;
		index_map_t::iterator start=ind_dummy.begin(), backup;
		while(!found && start!=ind_dummy.end()) {
			iterator parent=tr.parent(start->second);
			sibling_iterator ch=tr.begin(parent), last_part;
			while(ch!=tr.end(parent)) {
				auto fnd=ind_dummy.find((Ex::iterator)ch);
				if(fnd!=ind_dummy.end()) last_part=ch;
				++ch;
				}
			if(last_part==start->second) {
				++last_part;
				if(last_part==tr.end(parent)) {
					// Dummy index with nothing to the right is preferred
					found=true;
					}
				else backup=start;
				}
			if(!found) ++start;
			}
		// As a backup, we use a dummy index with only non-dummies to the right
		if(!found) start=backup;
		bool paired=true;
		while(paired && start!=ind_dummy.end()) {
			iterator parent=tr.parent(start->second);
			sibling_iterator ch=tr.begin(parent), last_part;
			while(ch!=tr.end(parent) && ind_dummy.size()>0) {
				auto fnd2=ind_dummy.equal_range((Ex::iterator)ch);
				auto fnd1=fnd2.first;
				if(fnd1->second!=ch) ++fnd1;
				if(fnd1->second==ch) {
					dummies.insert(dummies.end(), ch);
					ind_dummy.erase(fnd1);
					last_part=ch;
					}
				++ch;
				}
			auto fnd=ind_dummy.find((Ex::iterator)last_part);
			if(fnd==ind_dummy.end()) {
				last_part->flip_parent_rel();
				fnd=ind_dummy.find((Ex::iterator)last_part);
				last_part->flip_parent_rel();
				}
			if(fnd==ind_dummy.end()) {
				// Contraction ends because we are on a vector
				// It could also be a trace if we removed the paired index more than one iteration ago
				paired=false;
				}
			else {
				start=fnd;
				index_map_t::iterator check=ind_dummy.end();
				iterator parent=tr.parent(start->second);
				sibling_iterator ch=tr.begin(parent), first_part;
				while(check==ind_dummy.end()) {
					first_part=ch;
					check=ind_dummy.find((Ex::iterator)ch);
					++ch;
					}
				if(first_part!=start->second) {
					throw NotYetImplemented("Evaluation requires transposing a matrix.");
					return result_t::l_no_action;
					}
				}
			}
		}

	std::vector<Ex::iterator>::iterator dums1=dummies.begin(), dums2;
	dums2=dums1;
	++dums2;
	while(dums1!=dummies.end() && dums2!=dummies.end()) {
		//		txtout << "analysing " << std::endl;
		//		txtout << *(dums1->second->name) << std::endl;

		bool isbrack1=false, isbrack2=false;
		bool ismatorvec1=false, ismatorvec2=false;
		// These are both to recognize traces
		bool diffparents=tr.parent(*dums1)!=tr.parent(*dums2);
		bool consecutive=*(*dums1)->name==*(*dums2)->name;
		const Matrix *mat1=kernel.properties.get<Matrix>(tr.parent(*dums1));
		if(mat1)
			ismatorvec1=true;
		else if(*(tr.parent(*dums1)->name)=="\\indexbracket") {
			ismatorvec1=true;
			isbrack1=true;
			}
		else if(tr.number_of_children(tr.parent(*dums1))==1)
			ismatorvec1=true;
		const Matrix *mat2=kernel.properties.get<Matrix>(tr.parent(*dums2));
		if(mat2)
			ismatorvec2=true;
		else if(*(tr.parent(*dums2)->name)=="\\indexbracket") {
			ismatorvec2=true;
			isbrack2=true;
			}
		else if(tr.number_of_children(tr.parent(*dums2))==1)
			ismatorvec2=true;

		if(ismatorvec1 && ismatorvec2 && diffparents && consecutive) {
			//			txtout << "gluing " << *(dums2->second->name) << std::endl;
			// create new indexbracket with product node
			iterator outerbrack=tr.insert(tr.parent(*dums1), str_node("\\indexbracket"));
			iterator brackprod=tr.append_child(outerbrack, str_node("\\prod"));
			iterator parn1=tr.parent(*dums1);
			iterator parn2=tr.parent(*dums2);

			// count how many sign changes stand between the two objects
			int sign=1;
			unsigned int hits=0;
			Ex_comparator compare(kernel.properties);
			sib=tr.begin(it);
			while(hits<2) {
				if(hits==1 && sib!=parn2) {
					// pass arguments manually as can_swap() does not check them
					bool isbrack=*(sib->name)=="\\indexbracket";
					if(isbrack && isbrack2) {
						auto es=compare.equal_subtree(tr.begin(parn2), tr.begin(sib));
						sign*=compare.can_swap(tr.begin(parn2), tr.begin(sib), es, true);
						}
					else if(isbrack && !isbrack2) {
						auto es=compare.equal_subtree(parn2, tr.begin(sib));
						sign*=compare.can_swap(parn2, tr.begin(sib), es, true);
						}
					else if(!isbrack && isbrack2) {
						auto es=compare.equal_subtree(tr.begin(parn2), sib);
						sign*=compare.can_swap(tr.begin(parn2), sib, es, true);
						}
					else {
						auto es=compare.equal_subtree(parn2, sib);
						sign*=compare.can_swap(parn2, sib, es, true);
						}
					}
				if(sib==parn1 || sib==parn2) ++hits;
				++sib;
				}
			if(sign==-1) flip_sign(brackprod->multiplier);

			// remove the dummy index from these two objects, and move
			// other (dummy or not) indices to the outer indexbracket.
			sibling_iterator ind1=tr.begin(tr.parent(*dums1));
			sibling_iterator stop1=tr.end(tr.parent(*dums1));
			if(isbrack1)
				++ind1;
			while(ind1!=stop1) {
				if(ind1!=*dums1) {
					sibling_iterator nxt=ind1;
					++nxt;
					tr.reparent(outerbrack, ind1, nxt);
					}
				++ind1;
				//				ind1=tr.erase(ind1);
				}
			tr.erase(*dums1);
			sibling_iterator ind2=tr.begin(tr.parent(*dums2));
			sibling_iterator stop2=tr.end(tr.parent(*dums2));
			if(isbrack2)
				++ind2;
			while(ind2!=stop2) {
				if(ind2!=*dums2) {
					sibling_iterator nxt=ind2;
					++nxt;
					tr.reparent(outerbrack, ind2, nxt);
					}
				++ind2;
				//				ind2=tr.erase(ind2);
				}
			tr.erase(*dums2);

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
		if(consecutive) {
			++dums1;
			++dums2;
			}
		++dums1;
		++dums2;
		}

	//std::cerr << it << std::endl;

	//	prodflatten pf(tr, tr.end());
	//	pf.apply_recursive(it, false);

	cleanup_dispatch(kernel, tr, it);

	return result_t::l_applied;
	}
