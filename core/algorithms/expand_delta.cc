
#include "algorithms/expand_delta.hh"
#include "properties/KroneckerDelta.hh"
#include "properties/TableauBase.hh"
#include "Cleanup.hh"

using namespace cadabra;

#define ASYM 1

expand_delta::expand_delta(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool expand_delta::can_apply(iterator st)
	{
	const KroneckerDelta *kr=kernel.properties.get<KroneckerDelta>(st);
	if(kr)
		if(tr.number_of_children(st)>2)
			return true;
	return false;
	}

Algorithm::result_t expand_delta::apply(iterator& st)
	{
	// std::cerr << "applying " << Ex(st) << std::endl;

	Ex rep("\\sum");
	iterator sum=rep.begin();
	combin::combinations<Ex> ci;

#ifdef ASYM
	// Determine implicit anti-symmetrisation by looking for anti-symmetric
	// tensors in the product (if any). Store these sets in `implicit_sets`.
	std::vector<std::vector<Ex>> implicit_sets;
	
	if(*(tr.parent(st)->name)=="\\prod") {
		sibling_iterator oth=tr.begin(tr.parent(st));
		while(oth!=tr.end(tr.parent(st))) {
			if(oth!=st) {
				const TableauBase *tb=kernel.properties.get<TableauBase>(oth);
				if(tb) {
					// std::cerr << "object " << oth << " has symmetry" << std::endl;
					for(unsigned int i=0; i<tb->size(kernel.properties, tr, oth); ++i) {
						// std::cerr << "object " << oth << " tab " << i << std::endl;						
						TableauBase::tab_t tmptab(tb->get_tab(kernel.properties, tr, oth, i));
						for(unsigned int asset=0; asset<tmptab.row_size(0); ++asset) {
							if(tmptab.column_size(asset)>1) {
								// std::cerr << "   asym size " << tmptab.column_size(asset) << std::endl;
								std::vector<Ex> iset;
								for(unsigned int asel=0; asel<tmptab.column_size(asset); ++asel) {
									int indexnum=tmptab(asel,asset);
									auto oth2=begin_index(oth);
									oth2+=indexnum;
									iset.push_back(*oth2);
									}
								if(iset.size()>1)
									implicit_sets.push_back(iset);
								}
							}
						}
					}
				}
			++oth;
			}
		}
#endif
	// std::cerr << implicit_sets.size() << std::endl;
	// for(const auto& implicit_set: implicit_sets) {
	// 	for(const auto& implicit_index: implicit_set) {
	// 		std::cerr << *implicit_index.begin()->name << "  ";
	// 		}
	// 	std::cerr << std::endl;
	// 	}


	// Determine the overlap factors. For each index set
	// which is to be kept implicitly anti-symmetrised, we need
	// to determine how many of those indices actually sit on
	// the Kronecker delta. E.g. \delta^a_m^b_n A_{m n p} will
	// have an implicit set [m,n,p], but there are only two
	// indices overlapping, so the reduction in number of terms
	// is 2, not 6.
	multiplier_t divfactor1=1, divfactor2=1;

#ifdef ASYM
	Ex_comparator comp(kernel.properties);
	for(const auto& implicit_set: implicit_sets) {
		unsigned int overlap1=0, overlap2=0;
		for(const auto& implicit_index: implicit_set) {
			sibling_iterator ind=tr.begin(st);
			for(unsigned int pairnum=0; pairnum<tr.number_of_children(st); pairnum+=2) {
				// std::cerr << "does " << ind << " match " << implicit_index.begin() << std::endl;
				comp.clear();
				auto m1=comp.equal_subtree(ind, implicit_index.begin(), Ex_comparator::useprops_t::never, true);
				// std::cerr << static_cast<int>(m1) << std::endl;
				if(m1<=Ex_comparator::match_t::subtree_match) ++overlap1;
				++ind;
				comp.clear();				
				auto m2=comp.equal_subtree(ind, implicit_index.begin(), Ex_comparator::useprops_t::never, true);
				if(m2<=Ex_comparator::match_t::subtree_match) ++overlap2;
				++ind;
				}
			}
		divfactor1*=combin::fact(overlap1);
		divfactor2*=combin::fact(overlap2);
		}
#endif

	// std::cerr << divfactor1 << ", " << divfactor2 << std::endl;
	
	bool permute_second_set=(divfactor2>divfactor1);

	// Construct the original permutation.
	sibling_iterator ind=tr.begin(st);
	for(unsigned int pairnum=0; pairnum<tr.number_of_children(st); pairnum+=2) {
		if(permute_second_set)
			++ind;
		if(std::find(ci.original.begin(), ci.original.end(), *ind)!=ci.original.end()) {
			zero(st->multiplier);
			cleanup_dispatch(kernel, tr, st);
			return result_t::l_applied;
			}
		ci.original.push_back(*ind);
		++ind;
		if(!permute_second_set)
			++ind;
		ci.sublengths.push_back(1);
		}

#ifdef ASYM
	// Now construct the output asym ranges. 
	for(const auto& implicit_set: implicit_sets) {
		combin::range_t asymrange1;
		for(const auto& implicit_index: implicit_set) {
			for(unsigned int i1=0; i1<ci.original.size(); ++i1) {
				comp.clear();				
				auto m = comp.equal_subtree(ci.original[i1].begin(), implicit_index.begin(), Ex_comparator::useprops_t::never, true);
				if(m<=Ex_comparator::match_t::subtree_match) {
					// std::cerr << "add to asymrange: " << i1 << std::endl;
					asymrange1.push_back(i1);
					break;
					}
				}
			}
		ci.input_asym.push_back(asymrange1);
		}
#endif
	
	// Now generate all the permutations of the indices.
	ci.permute();
	// std::cerr << "number of permutations: " << ci.size() << std::endl;

	// For each permutation, add a product of deltas with two indices.
	for(unsigned int permnum=0; permnum<ci.size(); ++permnum) {
// FIXME: replace with progress indicator
//		if(permnum%10000==0)
//			std::cerr << "permutation " << permnum << std::endl;
		iterator prod=tr.append_child(sum, str_node("\\prod"));
		int sgn=combin::ordersign(ci[permnum].begin(), ci[permnum].end(),
		                          ci.original.begin(), ci.original.end());
		multiplier_t mult=multiplier_t(ci.multiplier(permnum))/multiplier_t(combin::fact((unsigned long)ci.original.size()))*sgn;
		//		std::cerr << "multipliers: " << mult << " = " << ci.multiplier(permnum)
		//					 << " / " << combin::fact(ci.original.size()) << " * " << sgn << std::endl;
		multiply(prod->multiplier, mult);
		multiply(prod->multiplier, *st->multiplier);
		sibling_iterator ind=tr.begin(st);
		for(unsigned int pairnum=0; pairnum<ci.original.size(); ++pairnum) {
			iterator delta=tr.append_child(prod, str_node(st->name));//"\\delta"));
			if(permute_second_set)
				tr.append_child(delta, (iterator)(ind));
			tr.append_child(delta, ci[permnum][pairnum].begin());
			++ind;
			if(!permute_second_set)
				tr.append_child(delta, (iterator)(ind));
			++ind;
			}
		}
//	std::cerr << "flatten" << std::endl;

	if(rep.number_of_children(sum)==1) { // flatten \sum node if there was only one term
		rep.flatten(sum);
		rep.erase(sum);
		}
	st=tr.replace(st, rep.begin());

//	std::cerr << "cleanup" << std::endl;
	cleanup_dispatch(kernel, tr, st);
//	std::cerr << "done" << std::endl;

	// After the return, we need to indicate that no new dummies were
	// introduced, otherwise we go into rename_replacement_dummies,
	// which can be extremely time-consuming.
//	std::cerr << "####### returning new value " << std::endl;
	
	return result_t::l_applied_no_new_dummies;
	}

