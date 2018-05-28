
#include "algorithms/decompose.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/rename_dummies.hh"
#include "algorithms/young_project_product.hh"
#include "properties/TableauBase.hh"
#include "Linear.hh"

using namespace cadabra;

decompose::decompose(const Kernel& k, Ex& tr, Ex& b)
   : Algorithm(k, tr), basis(b)
	{
	}

bool decompose::can_apply(iterator it) 
	{
	if(*it->name!="\\prod") return false;
	return true;
	}

void decompose::add_element_to_basis(Ex& projterm, Ex::iterator projtermit) 
	{
	// Add a new column for the new term in the basis
	for(unsigned int ii=0; ii<coefficient_matrix.size(); ++ii)
		coefficient_matrix[ii].push_back(0);

	if(*projtermit->name=="\\sum") {
		sibling_iterator moreit=projterm.begin(projtermit);
		while(moreit!=projterm.end(projtermit)) {
			multiplier_t remember_mult=*moreit->multiplier;
			one(moreit->multiplier);
			bool thistermfound=false;
			for(unsigned int ypi=0; ypi<terms_from_yp.size(); ++ypi) {
				if(projterm.equal_subtree(terms_from_yp[ypi].begin(), (iterator)moreit)) {
					coefficient_matrix[ypi].back()=remember_mult;
					thistermfound=true;
					break;
					}
				}
			if(!thistermfound) { // new monomial, so add a new row to the coefficient matrix
				Ex tmp(moreit);
//				tmp.print_recursive_treeform(txtout, tmp.begin());
				terms_from_yp.push_back(tmp);
				std::vector<multiplier_t> crow(coefficient_matrix.size()>0?
														 coefficient_matrix[0].size():1,0);
				crow.back()=remember_mult;
				coefficient_matrix.push_back(crow);
//				txtout << "added new monomial" << std::endl;
				}
			++moreit;
			}
		}
	else {
		multiplier_t remember_mult=*projtermit->multiplier;
		one(projtermit->multiplier);
		bool thistermfound=false;
		for(unsigned int ypi=0; ypi<terms_from_yp.size(); ++ypi) {
			if(projterm.equal_subtree(terms_from_yp[ypi].begin(), projtermit)) {
				coefficient_matrix[ypi].back()=remember_mult;
				thistermfound=true;
				break;
				}
			}
		if(!thistermfound) { // new monomial, so add a new row to the coefficient matrix
			Ex tmp(projtermit);
			terms_from_yp.push_back(tmp);
			std::vector<multiplier_t> crow(coefficient_matrix.size()>0?
													 coefficient_matrix[0].size():1,0);
			crow.back()=remember_mult;
			coefficient_matrix.push_back(crow);
			}
		}
	}

Algorithm::result_t decompose::apply(iterator& it)
	{
	bool ypproject=true;

#ifdef DEBUG
	std::cerr << "Projecting on basis " << basis.begin() << std::endl;
#endif
	
	iterator basisit=basis.begin();
	if(! (*basisit->name=="\\comma")) {
		sibling_iterator fr=basis.begin();
		sibling_iterator nd=fr;
		++nd;
		// basis should be a list; write it as such even if there's only one element.
		basisit->fl.bracket=str_node::b_none;
		basisit=tr.wrap(basisit, str_node("\\comma"));
		}

	Ex projbasis;
	projbasis.set_head(str_node("\\expression"));
	terms_from_yp.clear();
	coefficient_matrix.clear();

	// Some overlap with code in all_contractions.
	bool nontrivial_symmetries_present=false;
	sibling_iterator factorit=tr.begin(it);
	while(factorit!=tr.end(it)) { // Do this always, even if ypproject==false, since we need it for rhs.
		if(tr.number_of_children(factorit)>1) {
			const TableauBase *tb = kernel.properties.get<TableauBase>(factorit);
			if(tb)
				if(!tb->is_simple_symmetry(kernel.properties, tr, it)) {
					nontrivial_symmetries_present=true;
					break;
					}
			}
		++factorit;
		}

	// Setup the coefficient matrix.
	if(nontrivial_symmetries_present && ypproject) {
		//debugout << "Going to project the basis." << std::endl;
		// Need to make a Young-projected basis.
		sibling_iterator sib=tr.begin(basisit);
		while(sib!=tr.end(basisit)) {
			// debugout << "Next term in the basis." << std::endl;
			Ex projterm;
			projterm.set_head(str_node("\\expression"));
			projterm.append_child(projterm.begin(), (iterator)(sib));

#ifdef OLDVERSION
			young_project_tensor ypt(projterm, projterm.end());
			ypt.modulo_monoterm=true;
			iterator projtermit=projterm.begin(projterm.begin());
			ypt.apply_generic(projtermit, true, false, 0);

			distribute dbt(kernel, projterm);
			canonicalise can(kernel, projterm);
//			can.method=canonicalise::xperm;
			rename_dummies ren(kernel, projterm);
			collect_terms ct(kernel, projterm);
			
			dbt.apply_generic(projtermit, false);// FIXME: URGENT: should check consistency
			ren.apply_recursive(projtermit, false);  // by far the slowest step
			if(*projtermit->name=="\\sum")
				ct.apply(projtermit);
			can.apply_recursive(projtermit, false);
			ren.apply_recursive(projtermit, false);
			if(*projtermit->name=="\\sum")
				ct.apply(projtermit);
#else
			iterator projtermit=projterm.begin(projterm.begin());
			young_project_product ypp(kernel, projterm);
//			sumflatten sf(kernel, projterm);
			collect_terms ct(kernel, projterm);
			rename_dummies ren(kernel, projterm, "", "");

			ypp.apply_generic(projtermit, true, false, 0);
//			sf.apply_recursive(projtermit, false);
			ren.apply_generic(projtermit, true, false, 0);  // by far the slowest step
			if(*projtermit->name=="\\sum")
				ct.apply(projtermit);
			sibling_iterator sib2=tr.begin(projtermit);
			while(sib2!=tr.end(projtermit)) {
				 sib2->fl.bracket=str_node::b_none;
				 ++sib2;
				 }
#endif
			// After young projection, we may get identically zero.
			if(projtermit->is_zero()) {
//				txtout << "An element of the basis is identically zero after Young projection." << std::endl;
				return result_t::l_error;
				}
			add_element_to_basis(projterm, projtermit);
			++sib;
			}
		// debugout << "Young-projected basis constructed." << std::endl;
		}
	else {
		// Copy the basis straight into the terms_from_yp.
		assert(*basisit->name=="\\comma");
		sibling_iterator sib=tr.begin(basisit);
		while(sib!=tr.end(basisit)) {
			Ex projterm(sib);
			iterator projtermit=projterm.begin();
			sibling_iterator sib2=tr.begin(projtermit);
			while(sib2!=tr.end(projtermit)) {
				 sib2->fl.bracket=str_node::b_none;
				 ++sib2;
				 }
			add_element_to_basis(projterm, projtermit);
			++sib;
			}
		// debugout << "Kept old young-projected basis." << std::endl;
		}

	// Young project the rhs.
	Ex rhstree;
	rhstree.set_head(str_node("\\expression"));
	rhstree.append_child(rhstree.begin(), it);
	iterator rhsit=rhstree.begin(rhstree.begin());
	if(nontrivial_symmetries_present) {
#ifdef OLDVERSION
		young_project_tensor ypt(rhstree, rhstree.end());
		ypt.modulo_monoterm=true;
		ypt.apply_generic(rhsit, true, false, 0);
		if(*rhsit->name=="\\prod") {
			distribute dbt(rhstree, rhstree.end());
			canonicalise can(rhstree, rhstree.end());
			rename_dummies ren(rhstree, rhstree.end());
			collect_terms ct(rhstree, rhstree.end());

			dbt.apply(rhsit);
			ren.apply_generic(rhsit, true, false, 0);
			if(*rhsit->name=="\\sum")
				ct.apply(rhsit);
			can.apply_generic(rhsit, true, false, 0);
			ren.apply_generic(rhsit, true, false, 0);
			if(*rhsit->name=="\\sum")
				ct.apply(rhsit);
			}
#else
		young_project_product ypp(kernel, rhstree);
//		sumflatten sf(rhstree, rhstree.end());
		collect_terms ct(kernel, rhstree);
		rename_dummies ren(kernel, rhstree, "", "");

		// debugout << "young project rhs." << std::endl;
		ypp.apply_generic(rhsit, true, false, 0);
		// debugout << "sumflatten." << std::endl;
//		sf.apply_recursive(rhsit, false);
		// debugout << "rename." << std::endl;
		ren.apply_generic(rhsit, true, false, 0);  // by far the slowest step
		// debugout << "collect terms." << std::endl;
		ct.apply_generic(rhsit, true, false, 0);
		// debugout << "rhs projection done." << std::endl;

		sibling_iterator sib2=rhstree.begin(rhsit);
		while(sib2!=rhstree.end(rhsit)) {
			 sib2->fl.bracket=str_node::b_none;
			 ++sib2;
			 }
#endif		
		}	
	// debugout << "Young-projected rhs constructed" << std::endl;
	// rhstree.print_recursive_treeform(debugout, rhstree.begin());

	std::vector<multiplier_t> rhs(terms_from_yp.size(),0);
	if(*rhsit->name=="\\sum") {
		// iterate over all terms
		sibling_iterator rhssumit=rhstree.begin(rhsit);
		while(rhssumit!=rhstree.end(rhsit)) {
			bool found_in_basis=false;
			multiplier_t rhsmult=*rhssumit->multiplier;
			one(rhssumit->multiplier);
			for(unsigned int i=0; i<terms_from_yp.size(); ++i) {
				if(tr.equal_subtree(terms_from_yp[i].begin(), (iterator)(rhssumit))) {
					rhs[i]=rhsmult;
					found_in_basis=true;
					break;
					}
				}
			if(!found_in_basis) {
//				txtout << "rhs contains a term not present in the basis" << std::endl;
				return result_t::l_error;
				}
			++rhssumit;
			}
		}
	else {
		// only one term in the rhs
		 if(rhsit->is_zero()==false) {
			  bool found_in_basis=false;
			  multiplier_t rhsmult=*rhsit->multiplier;
			  one(rhsit->multiplier);
			  for(unsigned int i=0; i<terms_from_yp.size(); ++i) {
					if(tr.equal_subtree(terms_from_yp[i].begin(), rhsit)) {
						 rhs[i]=rhsmult;
						 found_in_basis=true;
						 break;
						 }
					}
			  if(!found_in_basis) {
//					txtout << "rhs contains a term not present in the basis" << std::endl;
					return result_t::l_error;
					}
			  }
		 }

	// debugout << "linear problem constructed" << std::endl;
	// for(unsigned int i=0; i<coefficient_matrix.size(); ++i) {
	// 	for(unsigned int j=0; j<coefficient_matrix[i].size(); ++j)
	// 		debugout << coefficient_matrix[i][j] << " ";
	// 	debugout << " " << rhs[i] << std::endl;
	// 	}
	// Now decompose 

	if(rhsit->is_zero()) {
		 // debugout << "rhs is identically zero" << std::endl;
		 Ex res;
		 res.set_head(str_node("\\comma"));
		 for(unsigned int i=0; i<coefficient_matrix[0].size(); ++i) 
			  res.append_child(res.begin(), str_node("1"))->multiplier=rat_set.insert(0).first;
		 tr.replace(it, res.begin());
		 }
	else {
		 // debugout << "doing gaussian elimination" << std::endl;
		 if(linear::gaussian_elimination_inplace(coefficient_matrix, rhs)) {
			 // for(unsigned int i=0; i<coefficient_matrix.size(); ++i) {
			 // 		for(unsigned int j=0; j<coefficient_matrix[i].size(); ++j)
			 // 			 debugout << coefficient_matrix[i][j] << " ";
			 // 		debugout << " = " << rhs[i] << std::endl;
			 // 		}
			  
			  Ex res;
			  res.set_head(str_node("\\comma"));
			  for(unsigned int i=0; i<coefficient_matrix[0].size(); ++i) 
					res.append_child(res.begin(), str_node("1"))->multiplier=rat_set.insert(rhs[i]).first;
			  it=tr.replace(it, res.begin());
			  }
		 else {
//			  txtout << "decomposing impossible" << std::endl;
//		tr.print_recursive_treeform(txtout, it);
			  return result_t::l_error;
			  }
		 }
	

	return result_t::l_applied;
	}


