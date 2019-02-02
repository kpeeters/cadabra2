
#include "explicit_indices.hh"

#include "algorithms/substitute.hh"
#include "properties/ImplicitIndex.hh"

using namespace cadabra;

explicit_indices::explicit_indices(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool explicit_indices::can_apply(iterator st)
	{
	// Work on equals nodes, or single terms (not terms in a sum) or 
	// sums, provided the latter are not lhs or rhs of an equals node.
	// All this because when we generate free indices, we need to
	// ensure that all terms and all sides of an equals node use the
	// same index names.

	if(*st->name=="\\equals") return false; // switch
	if(*st->name=="\\sum")    return true;
	if(is_termlike(st)) {
		if(tr.is_head(st)) return         true;
		if(*tr.parent(st)->name=="\\sum") return false;
		return true;
		}
	
	return false;
	}

Algorithm::result_t explicit_indices::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	// Ensure that we are always working on a sum, even
	// if there is only one term.
	if(is_termlike(it)) 
		force_node_wrap(it, "\\sum");

	// Classify all free and dummy indices already present. Any new
	// indices cannot be taken from these.
	index_map_t ind_free_sum, ind_dummy_sum;
	classify_indices(it, ind_free_sum, ind_dummy_sum);
	for(auto& k: ind_free_sum)
		std::cerr << k.first << std::endl;
	for(auto& k: ind_dummy_sum)
		std::cerr << k.first << std::endl;
	
	sibling_iterator term=tr.begin(it);
	while(term!=tr.end(it)) {
		iterator tmp=term;
		prod_wrap_single_term(tmp);
		term=tmp;

		// For each index set, keep track of the last used index in
		// building the explicit index line.
		std::map<const Indices *, Ex::iterator> index_lines;
		
		sibling_iterator factor=tr.begin(term);
		while(factor!=tr.end(term)) {
			int tmp;
			auto ii = kernel.properties.get_with_pattern<ImplicitIndex>(factor, tmp);
			if(ii.first) {
				// Determine indices on this factor. Use a copy because we
				// need this object later.
				Ex orig(factor);
				index_map_t factor_ind_free, factor_ind_dummy;
				classify_indices(orig.begin(), factor_ind_free, factor_ind_dummy);

				// Substitute explcit replacement and rename the indices
				// already present on the implicit factor.
				Ex replacement("\\arrow");
				replacement.append_child(replacement.begin(), ii.second->obj.begin());
				replacement.append_child(replacement.begin(), ii.first->explicit_form.begin());
				substitute subs(kernel, tr, replacement);
				iterator factor_tmp=factor;
				if(subs.can_apply(factor_tmp)) 
					subs.apply(factor_tmp);
				else
					throw InternalError("Internal inconsistency encountered, aborting.");
				factor=factor_tmp;

				// Determine indices on the replacement.
				index_map_t repl_ind_free, repl_ind_dummy;
				classify_indices(factor, repl_ind_free, repl_ind_dummy);

				// Which indices have been added?
				index_map_t ind_same;
				IndexClassifier ic(kernel);
				ic.determine_intersection(factor_ind_free, repl_ind_free, ind_same, true);

				// Go through indices in order of appearance, determine if they have
				// been added, and rename. Note: indices do not appear in order in the
				// index maps above.
				auto iit=index_iterator::begin(kernel.properties, factor);
				while(iit!=index_iterator::end(kernel.properties, factor)) {
					auto search=repl_ind_free.begin();
					bool found=false;
					while(search!=repl_ind_free.end()) {
						if(search->second == (iterator)iit) {
							found=true;
							break;
							}
						++search;
						}
					if(found) {
						// This index was added.
						// Get a new free index.

						auto ip = kernel.properties.get<Indices>(search->second);
						if(!ip)
							throw InternalError("Do not have Indices property for all implicit indices.");

						std::cerr << "getting dummy index" << std::endl;
						auto di = ic.get_dummy(ip, &ind_free_sum, &ind_dummy_sum);
						std::cerr << di << std::endl;
						tr.replace(search->second, di.begin());
						}

					++iit;
					}
				}
			++factor;
			}

		tmp=term;
		prod_unwrap_single_term(tmp);
		++term;
		}

	return res;
	}
