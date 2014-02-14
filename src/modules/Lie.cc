unsigned int exchange::possible_singlets(exptree& tr, exptree::iterator it)
	{
	std::vector<identical_tensors_t> idts;
	collect_identical_tensors(tr, it, idts);

	if(idts.size()==0) return 1; // no indices, so this is a singlet already

	LiE::LiE_t lie;
	// Figure out the algebra from one of the indices.
	exptree::index_iterator indit=tr.begin_index(idts[0].tensors[0]);
	const numerical::Integer *iprop=properties::get<numerical::Integer>(indit, true);
	if(!iprop) 
		throw consistency_error("Need to know about the range of the " + *indit->name + " index.");

//	iprop->difference.print_recursive_treeform(txtout, iprop->difference.begin());
	unsigned int dims=to_long(*iprop->difference.begin()->multiplier);
//	std::cout << "*** " << dims << std::endl;
	if(dims%2==0) 
		lie.algebra_type=LiE::LiE_t::alg_D;
	else
		lie.algebra_type=LiE::LiE_t::alg_B;
	lie.algebra_dim=dims/2;
	
	lie.start();

	// Find the representation for each group of tensors, taking into
	// account their exchange symmetries.
	std::vector<LiE::LiE_t::reps_t> groupreps;

	for(unsigned int i=0; i<idts.size(); ++i) {
		LiE::LiE_t::reps_t single_tensor_rep;
		if(idts[i].number_of_indices>1) {
			assert(idts[i].tab); // Every tensor with 2+ indices needs a TableauSymmetry.
			TableauBase::tab_t thetab=idts[i].tab->get_tab(tr, idts[i].tensors[0], 0);
			
			std::vector<unsigned int> topleth;
			for(unsigned int rws=0; rws<thetab.number_of_rows(); ++rws)
				topleth.push_back(thetab.row_size(rws));
			lie.plethysm(topleth, single_tensor_rep, idts[i].traceless!=0, 
							 (thetab.selfdual_column==0)?0:thetab.selfdual_column/abs(thetab.selfdual_column));
			}
		else { // vector representation
			LiE::LiE_t::rep_t tmp;
			tmp.weight.resize(lie.algebra_dim,0);
			tmp.weight[0]=1;
			if(lie.algebra_type==LiE::LiE_t::alg_D && lie.algebra_dim==2)
				tmp.weight[1]=1;
//			txtout << tmp.weight[1] << std::endl;
			single_tensor_rep.push_back(tmp);
			}
		// If it's a spinor, still tensor with a spinor rep.
		if(idts[i].spino) {
			LiE::LiE_t::reps_t vectorpart=single_tensor_rep;
			LiE::LiE_t::reps_t spinorpart;
			LiE::LiE_t::rep_t spinorrep;
			spinorrep.weight.resize(lie.algebra_dim,0);
			spinorrep.weight[lie.algebra_dim-1]=1;
			spinorpart.push_back(spinorrep);
			lie.tensor(vectorpart, spinorpart, single_tensor_rep);
			if(idts[i].gammatraceless)
				lie.keep_largest_dim(single_tensor_rep);
			}
			
		if(idts[i].tensors.size()==1) { // we're done
			groupreps.push_back(single_tensor_rep);
			}
		else {
			LiE::LiE_t::reps_t multi_tensor_rep;
			int sign=idts[i].extra_sign + (idts[i].spino!=0);
			if(idts[i].comm && idts[i].comm->sign()==-1) 
				++sign;
			lie.alt_sym_tensor(idts[i].tensors.size(), single_tensor_rep, multi_tensor_rep, sign%2==0);
			groupreps.push_back(multi_tensor_rep);
			}
		}

	// Now tensor the whole lot together.
	LiE::LiE_t::reps_t result=groupreps[0], tmpstore;

	for(unsigned int i=0; i<groupreps.size()-1; ++i) {
		lie.tensor(result, groupreps[i+1], tmpstore);
		result=tmpstore;
		}
	unsigned int retval=lie.multiplicity_of_singlet(result);
	lie.stop();

	return retval;
	}
