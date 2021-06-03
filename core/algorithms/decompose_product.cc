
#include "Cleanup.hh"
#include "IndexIterator.hh"
#include "Exceptions.hh"
#include "algorithms/decompose_product.hh"
#include "properties/Integer.hh"

using namespace cadabra;

decompose_product::decompose_product(const Kernel& k, Ex&tr)
	: Algorithm(k, tr), t1(0), t2(0)
{
}

const Indices *decompose_product::indices_equivalent(iterator it) const
{
	index_iterator ii=index_iterator::begin(kernel.properties, it);
	const Indices *ret=0, *tmp=0;
	while(ii!=index_iterator::end(kernel.properties, it)) {
		tmp=kernel.properties.get<Indices>(ii, true);
		if(tmp==0) return 0;
		if(ret==0) ret=tmp;
		else if(ret!=tmp) return 0;
		++ii;
	}
	return ret;
}

bool decompose_product::can_apply(iterator it)
{
	// Act on products. Find the first object which either has a
	// TableauSymmetry or has one vector index only. Then find the next
	// indexed object in the product and return true if this is a
	// one-indexed or TableauSymmetry object, and if the index types
	// of all indices match.

	if(*it->name=="\\prod") {
		sibling_iterator fc=tr.begin(it);
		while(fc!=tr.end(it)) {
			t1=kernel.properties.get<TableauBase>(fc);
			if(t1 || number_of_indices(kernel.properties, fc)==1) {
				f1=fc;
				ind1=indices_equivalent(fc);
				if(ind1) {
					++fc;
					if(fc!=tr.end(it)) {
						t2=kernel.properties.get<TableauBase>(fc);
						if(t2 || number_of_indices(kernel.properties, fc)==1) {
							f2=fc;
							ind2=indices_equivalent(fc);
							if(ind2 && ind1==ind2) {
								// Strip off the parent rel because Integer properties are
								// declared as {m,n,p}::Integer, not {_m, _n, _p}::Integer.
								Ex index(index_iterator::begin(kernel.properties, fc));
								index.begin()->fl.parent_rel=str_node::p_none;
								const Integer *itg=
								   kernel.properties.get<Integer>( index.begin(), true );
								if(itg) {
									dim=to_long(*itg->difference.begin()->multiplier);
									if(dim>0)
										return true;
								}
							}
						}
					}
				}
			}
			++fc;
		}
	}
	return false;
}

void decompose_product::fill_asym_ranges(TableauBase::tab_t& tab, int offset,
      combin::range_vector_t& ranges)
{
	// FIXME: we could also look at all other factors, and see if the index
	// _name_ in the slot is contracted to the index name in an antisymmetric
	// slot range. But that is more tricky, because index names move, whereas
	// slots stay.

	for(unsigned int i=0; i<tab.row_size(0); ++i) {
		TableauBase::tab_t::in_column_iterator ci=tab.begin_column(i);
		combin::range_t tmprange;
		while(ci!=tab.end_column(i)) {
			tmprange.push_back((*ci)+offset);
			++ci;
		}
		if(tmprange.size()>=2)
			ranges.push_back(tmprange);
	}
}

Algorithm::result_t decompose_product::apply(iterator& it)
{
	// Create the tensor product Young tableaux.
	sibtab_t  m1,m2;
	sibtabs_t prod;
	numtabs_t numprod;

	unsigned int ioffset1=0, ioffset2=0;

	if(t1) {
		if(t1->size(kernel.properties, tr, f1)>1)
			throw ConsistencyException("decompose_product: cannot handle multiple tableau tensors");
		t1tab=t1->get_tab(kernel.properties, tr, f1, 0);
		for(unsigned int r=0; r<t1tab.number_of_rows(); ++r)
			for(unsigned int c=0; c<t1tab.row_size(r); ++c) {
				index_iterator tmpii=index_iterator::begin(kernel.properties, f1);
				tmpii+=t1tab(r,c);
				m1.add_box(r, tmpii);
			}
	}
	else m1.add_box(0, index_iterator::begin(kernel.properties, f1));

	if(t2) {
		if(t2->size(kernel.properties, tr, f2)>1)
			throw ConsistencyException("decompose_product: cannot handle multiple tableau tensors");
		t2tab=t2->get_tab(kernel.properties, tr, f2, 0);
		for(unsigned int r=0; r<t2tab.number_of_rows(); ++r)
			for(unsigned int c=0; c<t2tab.row_size(r); ++c) {
				index_iterator tmpii=index_iterator::begin(kernel.properties, f2);
				tmpii+=t2tab(r,c);
				m2.add_box(r, tmpii);
			}
	}
	else m2.add_box(0, index_iterator::begin(kernel.properties, f2));

	// Determine the position of the first index of the two
	// factors relative to the product (not to the tensors themselves).
	index_iterator srch=index_iterator::begin(kernel.properties, it);
	while(srch!=index_iterator::end(kernel.properties, it)) {
		if(iterator(srch)==iterator(index_iterator::begin(kernel.properties, f1)))
			break;
		++ioffset1;
		++srch;
	}
	srch=index_iterator::begin(kernel.properties, it);
	while(srch!=index_iterator::end(kernel.properties, it)) {
		if(iterator(srch)==iterator(index_iterator::begin(kernel.properties, f2)))
			break;
		++ioffset2;
		++srch;
	}

	// Determine slot ranges which are anti-symmetric.

	asym_ranges.clear();
	if(t1) fill_asym_ranges(t1tab, ioffset1, asym_ranges);
	if(t2) fill_asym_ranges(t2tab, ioffset2, asym_ranges);

	// Make the tensor product tableaux.

	yngtab::LR_tensor(m1, m2, dim, prod.get_back_insert_iterator(), true);

	//std::cerr << "dim=" << dim << ", size=" << prod.storage.size() << std::endl;

	// The tableaux in 'prod' contain in their boxes iterators to
	// the indices in the original expression. We convert these to
	// numerical positions so they can be applied to copies of the
	// expression as well.

	sibtabs_t::tableau_container_t::iterator tt=prod.storage.begin();
	while(tt!=prod.storage.end()) {
		numtab_t tmptab;
		tmptab.copy_shape(*tt);
		sibtab_t::iterator si=tt->begin();
		numtab_t::iterator ni=tmptab.begin();
		while(si!=tt->end()) {
			index_iterator fnd=index_iterator::begin(kernel.properties, it);
			unsigned int inum=0;
			while(fnd!=index_iterator::end(kernel.properties, it)) {
				if(iterator(fnd) == (*si)) {
					*ni=inum;
					break;
				}
				++inum;
				++fnd;
			}
			assert(inum!=number_of_indices(kernel.properties, it));
			++ni;
			++si;
		}
		numprod.storage.push_back(tmptab);
		++tt;
	}

	// Now create a Young projector sum of terms with the indices
	// distributed according to the tensor product tableaux.

	// 	std::cout << numprod << std::endl;

	Ex rep;
	rep.set_head(str_node("\\tmp"));  // not \sum to prevent auto flattening

	numtabs_t::tableau_container_t::iterator ntt=numprod.storage.begin();

	while(ntt!=numprod.storage.end()) {
		// TESTINGONLY
		///		++ntt; ++ntt; ++ntt;

		//		txtout << "another tableau" << std::endl;
		young_project yp(kernel, tr);
		yp.tab=(*ntt);

		//		if(getenv("SMART"))
		yp.asym_ranges=asym_ranges;

		// The asym ranges contain ranges of index locations. What we need
		// to convert this to is box numbers. This is a value->location
		// conversion in combinatorics.hh language. This will be done
		// inside the youngtab.hh routines.

		// Apply the product projector.
		iterator rr=rep.append_child(rep.begin(), it);
		auto res=yp.can_apply(rr);
		assert(res);
		yp.apply(rr);

		// We cannot use any algorithms which re-order indices, as the
		// order in yp.sym must match the order in the expression. Also,
		// we cannot remove terms without removing the corresponding entries
		// in yp.sym. So for the time being we have decided to put this
		// simplification in young_project.

		// Now apply the symmetries of the original tableaux (if any).
		// For each of the permutations in the product projector,
		// we need to figure out where the indices went which sat on
		// tensor 1 and 2. This information is stored in the symmetriser
		// of young_project. These indices then have to be projected using
		// the tensor projectors.


		// TESTINGONLY
		//		txtout << "one ..." << std::flush;
		if(t1) project_onto_initial_symmetries(rep, rr, yp, t1, f1, ioffset1, t1tab, false);
		//		txtout << "done" << std::endl;
		//		txtout << "two ..." << std::flush;
		if(t2) project_onto_initial_symmetries(rep, rr, yp, t2, f2, ioffset2, t2tab, true);
		//		txtout << "done" << std::endl;

		//    TESTINGONLY
		///		break;

		++ntt;
	}

	rep.begin()->name=name_set.insert("\\sum").first;

	it=tr.replace(it, rep.begin());

	// flatten sums
	//	txtout << "flattening... " << std::flush;
	//	sumflatten sf(tr, tr.end());
	//	sf.apply_recursive(it, false);
	//	txtout << "done" << std::endl;

	//	tr.print_recursive_treeform(std::cout, it);

	cleanup_dispatch(kernel, tr, it);

	return result_t::l_applied;
}


void decompose_product::project_onto_initial_symmetries(Ex& rep, iterator rr, young_project& yp,
      const TableauBase *, iterator ff,
      int ioffset, const TableauBase::tab_t& thetab,
      bool remove_traces)
{
	// Sample: S_{m n} D_{p}{ A_{n q} } with S symmetric and A antisymmetric.
	// The tensor product contains one tableau which leads to a symmetriser
	// with as first entry 0 3 2 1 4. This means that the 'm' and 'n' index
	// names are associated, in the original, to box 0 and 3 respectively.
	// Now one of the terms in this symmetriser reads 2 4 0 1 3. In order
	// to apply the individual tensor projectors, we read off that the
	// 'm' and 'n' indices have now been moved to slot 2 and 1 respectively.
	// So we create a [1 1] tableau with numbers 2 and 1 in the boxes,
	// and apply this tensor projector to the full product tensor.

	unsigned int termnum=0;

	// Run through all terms in this tableau of the tensor product.
	sibling_iterator term=rep.begin(rr);
	while(term!=rep.end(rr)) {
		// Setup the tableau for initial-tensor projection.
		young_project ypinitial(kernel, tr);
		ypinitial.tab.copy_shape(thetab);
		numtab_t::iterator tabit=ypinitial.tab.begin();
		numtab_t::const_iterator origtabit=thetab.begin();

		sibling_iterator nxt=term;
		++nxt;
		index_iterator ii=index_iterator::begin(kernel.properties, ff);
		while(ii!=index_iterator::end(kernel.properties, ff)) {
			unsigned int ipos=ioffset + (*origtabit);
			assert(termnum<yp.sym.size());

			// Find ipos in the first entry of yp.sym
			// and store the new position in the tableau.
			for(unsigned int i=0; i<yp.sym[termnum].size(); ++i) {
				if(yp.sym[termnum][i]==ipos) {
					*tabit=yp.sym[0][i];
					//					txtout << ipos << " has moved to " << yp.sym[0][i] << std::endl;
					break;
				}
			}
			++tabit;
			++origtabit;
			++ii;
		}

		// Now we can finally project.
		yp.remove_traces=remove_traces;

		if(*term->name=="\\sum") { // apply to all terms in the sum
			// THIS IS NOT CORRECT?! If we turn on asym_ranges  here
			// the result breaks.
			//			if(getenv("SMART"))
			//				ypinitial.asym_ranges=asym_ranges;
			sibling_iterator trmit=tr.begin(term);
			while(trmit!=tr.end(term)) {
				iterator tmp=trmit;
				sibling_iterator nxt2=trmit;
				++nxt2;

				// Now apply the projector.
				auto res=ypinitial.can_apply(tmp);
				assert(res);
				ypinitial.apply(tmp);
				trmit=nxt2;
			}
		}
		else {   // just a single term
			//			if(getenv("SMART"))
			ypinitial.asym_ranges=asym_ranges;
			iterator tmp=term;
			[[maybe_unused]] auto res=ypinitial.can_apply(tmp);
			assert(res);
			ypinitial.apply(tmp);
		}

		++termnum;
		term=nxt;
	}
}
