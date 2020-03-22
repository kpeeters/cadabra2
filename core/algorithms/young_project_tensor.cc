
#include "IndexIterator.hh"
#include "Exceptions.hh"
#include "Cleanup.hh"
#include "properties/Indices.hh"
#include "properties/Integer.hh"
#include "algorithms/young_project_tensor.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/indexsort.hh"

using namespace cadabra;

young_project_tensor::young_project_tensor(const Kernel& k, Ex& tr, bool modmono)
	: Algorithm(k, tr), modulo_monoterm(modmono)
	{
	}

bool young_project_tensor::can_apply(iterator it)
	{
	if(*it->name=="\\prod") return false;
	
	tb=kernel.properties.get_composite<TableauBase>(it);
	if(tb) {
		if(tb->size(kernel.properties, tr, it)>0)
			return true;
		}
	return false;
	}

Algorithm::result_t young_project_tensor::apply(iterator& it)
	{
	//	std::cout << "at " << *it->name << std::endl;
	assert(tb);
	//	txtout << typeid(*tb).name() << std::endl;
	// FIXME: handle cases with multiple tabs.
	TableauBase::tab_t tab=tb->get_tab(kernel.properties, tr, it, 0);
	if(tab.number_of_rows()==0)
		return result_t::l_no_action;
	//	txtout << tab << std::endl;

	if(modulo_monoterm) {
		if(tab.number_of_rows()==1) // Nothing happends with fully symmetric tensors modulo monoterm.
			return result_t::l_no_action;
		if(tab.row_size(0)==1 && tab.selfdual_column==0) // Ditto for fully anti-symmetric tensors & modmono.
			return result_t::l_no_action;
		}

	// For non-trivial tableau shapes, apply the Young projector.
	Ex rep;
	rep.set_head(str_node("\\sum"));
	if(tab.row_size(0)>0) {
		sym.clear();
		tab.projector(sym); //, modulo_monoterm);

		//		txtout << sym.size() << std::endl;
		for(unsigned int i=0; i<sym.size(); ++i) {
			Ex repfac(it);
			for(unsigned int j=0; j<sym[i].size(); ++j) {
				index_iterator src_fd=index_iterator::begin(kernel.properties, it);
				index_iterator dst_fd=index_iterator::begin(kernel.properties, repfac.begin());
				//			txtout << sym[i][j] << " " << sym.original[j] << std::endl;
				src_fd+=sym[i][j];
				dst_fd+=sym.original[j];
				//			txtout << *src_fd->name  << std::endl;
				//			txtout << *dst_fd->name  << std::endl;
				dst_fd->name=src_fd->name;
				}
			multiply(repfac.begin()->multiplier, sym.signature(i));
			multiply(repfac.begin()->multiplier, tb->get_tab(kernel.properties, tr, it, 0).projector_normalisation());
			iterator newtensor=rep.append_child(rep.begin(), repfac.begin());
			if(modulo_monoterm) { // still necessary for column exchange
				indexsort isort(kernel, rep);
				auto res=isort.can_apply(newtensor); // to set tb
				assert(res);
				isort.apply(newtensor);
				}
			}
		collect_terms cterms(kernel, rep);
		iterator rephead=rep.begin();
		cterms.apply(rephead);
		}
	else {
		rep.append_child(rep.begin(), it);
		}

	// If there is a selfdual or anti-selfdual component, we need to add a term
	// for each generated term, which contains an epsilon. In the generated term,
	// find the positions of the indices which originally sat on the selfdual tensor,
	// then replace these with dummies which are repeated on the epsilon tensor.

	//	We should do all of this _here_, not before the Young projector, because we
	// want to avoid using indexsort.

	if(tab.selfdual_column!=0) {
		// Classify indices so we can insert dummies.
		index_map_t one, two, three, four, added_dummies;
		classify_indices_up(it, one, two);
		classify_indices(it, three, four);

		// Figure out the properties of the indices for which we want dummy partners.
		index_iterator iit=index_iterator::begin(kernel.properties, it);
		iit+=tab(0,abs(tab.selfdual_column)-1);
		const Integer *itg=kernel.properties.get<Integer>(iit, true);
		const Indices *ind=kernel.properties.get<Indices>(iit, true);
		if(itg==0)
			throw ConsistencyException("young_project_tensor: Need to know the range of the indices.");
		if(ind==0)
			throw ConsistencyException("young_project_tensor: Need to have a set of dummy indices.");

		// Step through all generated terms and give each one an epsilon partner term.
		sibling_iterator tt=rep.begin(rep.begin());
		while(tt!=rep.end(rep.begin())) {
			Ex repfac(tt);
			iterator prodit=repfac.wrap(repfac.begin(), str_node("\\prod"));
			iterator tensit=repfac.begin(prodit);
			// FIXME: take care of Euclidean signature cases.
			//			repfac.insert(repfac.begin(prodit), str_node("I"));
			iterator epsit =repfac.append_child(prodit, str_node("\\epsilon"));

			// Normalise the epsilon term appropriately.
			multiply(prodit->multiplier,
			         multiplier_t(1)/combin::factorial(to_long(*(itg->difference.begin()->multiplier)/2)));
			if(tab.selfdual_column<0)
				flip_sign(prodit->multiplier);

			// Move the free indices to the epsilon factor.
			for(unsigned int row=0; row<tab.column_size(abs(tab.selfdual_column)-1); ++row) {
				// Find index of original in newly generated term (all indices are different,
				// so this works).
				iit=index_iterator::begin(kernel.properties, tensit);
				index_iterator iit_orig=index_iterator::begin(kernel.properties, it);
				iit_orig+=tab(row, abs(tab.selfdual_column)-1);
				while(subtree_exact_equal(&kernel.properties, iit, iit_orig)==false)
					++iit;

				Ex dum=get_dummy(ind, &one, &two, &three, &four, &added_dummies);
				repfac.append_child(epsit, iterator(iit)); // move index to eps
				iterator repind=rep.replace_index(iterator(iit), dum.begin()); // replace index on tens
				added_dummies.insert(index_map_t::value_type(dum, repind));
				}
			// Now insert the new dummies in the epsilon.
			index_map_t::iterator adi=added_dummies.begin();
			while(adi!=added_dummies.end()) {
				iterator ni=repfac.append_child(epsit, adi->first.begin());
				ni->fl.parent_rel=str_node::p_sub;
				++adi;
				}
			added_dummies.clear();

			++tt;
			rep.insert_subtree(tt, repfac.begin());
			}
		}

	// Final cleanup
	it=tr.replace(it,rep.begin());

	cleanup_dispatch(kernel, tr, it);
	return result_t::l_applied;
	}
