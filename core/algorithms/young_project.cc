
#include "IndexIterator.hh"
#include "algorithms/young_project.hh"

using namespace cadabra;

young_project::young_project(const Kernel& k, Ex& tr)
	: Algorithm(k,tr), remove_traces(false)
	{
	// For internal use in which one fills the young tableau structures directly.
	}

young_project::young_project(const Kernel& k, Ex& tr,
                             const std::vector<int>& shape,
                             const std::vector<int>& indices)
	: Algorithm(k,tr), remove_traces(false)
	{
	int count=0;
	for(unsigned int row=0; row<shape.size(); ++row) {
		for(int col=0; col<shape[row]; ++col) {
			tab.add_box(row, indices[count]);
			++count;
			}
		}
	}

bool young_project::can_apply(iterator it)
	{
	if(*it->name!="\\prod") {
		if(!is_single_term(it)) {
			return false;
			}
		}

	prod_wrap_single_term(it);
	if(nametab.number_of_rows()>0) { // have to convert names to numbers
		tab.copy_shape(nametab);
		pos_tab_t::iterator pi=tab.begin();
		name_tab_t::iterator ni=nametab.begin();
		while(ni!=nametab.end()) {
			index_iterator ii=index_iterator::begin(kernel.properties, it);
			unsigned int indexnum=0;
			while(ii!=index_iterator::end(kernel.properties, it)) {
				if(subtree_exact_equal(&kernel.properties, ii, *ni)) {
					*pi=indexnum;
					break;
					}
				++indexnum;
				++ii;
				}
			if(indexnum==number_of_indices(kernel.properties, it)) {
				prod_unwrap_single_term(it);
				return false; // cannot find indicated index in expression
				}
			++pi;
			++ni;
			}
		}

	prod_unwrap_single_term(it);
	return true;
	}

Ex::iterator young_project::nth_index_node(iterator it, unsigned int num)
	{
	Ex::fixed_depth_iterator indname=tr.begin_fixed(it, 2);
	indname+=num;
	return indname;
	}

Algorithm::result_t young_project::apply(iterator& it)
	{
	prod_wrap_single_term(it);
	sym.clear();

	if(asym_ranges.size()>0) {
		// Convert index locations to box numbers.
		combin::range_vector_t sublengths_scattered;
		//		txtout << "asym_ranges: ";
		for(unsigned int i=0; i<asym_ranges.size(); ++i) {
			combin::range_t newr;
			for(unsigned int j=0; j<asym_ranges[i].size(); ++j) {
				// Search asym_ranges[i][j]
				int offs=0;
				pos_tab_t::iterator tt=tab.begin();
				while(tt!=tab.end()) {
					if((*tt)==asym_ranges[i][j]) {
						newr.push_back(offs);
						//						txtout << asym_ranges[i][j] << " ";
						break;
						}
					++tt;
					++offs;
					}
				}
			sublengths_scattered.push_back(newr);
			//			txtout << std::endl;
			}
		tab.projector(sym, sublengths_scattered);
		} else tab.projector(sym);

	// FIXME: We can also compress the result by sorting all
	// locations which belong to the same asym set. This could actually
	// be done in combinatorics already.

	Ex rep;
	rep.set_head(str_node("\\sum"));
	for(unsigned int i=0; i<sym.size(); ++i) {
		// Generate the term.
		Ex repfac(it);
		for(unsigned int j=0; j<sym[i].size(); ++j) {
			index_iterator src_fd=index_iterator::begin(kernel.properties, it);
			index_iterator dst_fd=index_iterator::begin(kernel.properties, repfac.begin());
			src_fd+=sym[i][j];        // take the index at location sym[i][j]
			dst_fd+=sym.original[j];  // and store it in location sym.original[j]
			tr.replace_index(dst_fd, src_fd);
			}
		// Remove traces of antisymmetric objects. This can really
		// only be done here, since combinatorics.hh does not know
		// about index values, only about index locations. Note: we also
		// have to remove the entry in sym.original & sym.multiplicity if
		// we decide that a term vanishes.
		// IMPORTANT: if there are still permutations by value to be
		// done afterwards, do not use this!
		if(remove_traces) {
			for(unsigned int k=0; k<asym_ranges.size(); ++k) {
				for(unsigned int kk=0; kk<asym_ranges[k].size(); ++kk) {
					index_iterator it1=index_iterator::begin(kernel.properties, repfac.begin());
					it1+=asym_ranges[k][kk];
					for(unsigned int kkk=kk+1; kkk<asym_ranges[k].size(); ++kkk) {
						index_iterator it2=index_iterator::begin(kernel.properties, repfac.begin());
						it2+=asym_ranges[k][kkk];
						if(subtree_exact_equal(&kernel.properties, it1,it2)) {
							sym.set_multiplicity(i,0);
							goto traceterm;
							}
						}
					}
				}
			}

			{
			multiply(repfac.begin()->multiplier, sym.signature(i));
			multiply(repfac.begin()->multiplier, tab.projector_normalisation());
			iterator repfactop=repfac.begin();
			prod_unwrap_single_term(repfactop);
			rep.append_child(rep.begin(), repfac.begin());
			}

traceterm:
		;
		}
	it=tr.replace(it,rep.begin());

	sym.remove_multiplicity_zero();

	return result_t::l_applied;
	}
