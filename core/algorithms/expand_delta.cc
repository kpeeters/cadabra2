
#include "algorithms/expand_delta.hh"
#include "properties/KroneckerDelta.hh"
#include "properties/TableauBase.hh"
#include "Cleanup.hh"

using namespace cadabra;

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
	combin::combinations<str_node> ci;

	std::vector<iterator> remove_these;

// 	// Determine implicit anti-symmetrisation by looking for anti-symmetric
// 	// tensors in the product (if any).
// 	// FIXME: finding these sort of index sets has to be done in a more general way.
// 	if(*(tr.parent(st)->name)=="\\prod") {
// 		sibling_iterator oth=tr.begin(tr.parent(st));
// 		while(oth!=tr.end(tr.parent(st))) {
// 			if(oth!=st) {
// 				const TableauBase *tb=kernel.properties.get<TableauBase>(oth);
// 				if(tb) {
// 					for(unsigned int i=0; i<tb->size(kernel.properties, tr, oth); ++i) {
// 						TableauBase::tab_t tmptab(tb->get_tab(kernel.properties, tr, oth, i));
// 						for(unsigned int asset=0; asset<tmptab.row_size(0); ++asset) {
// 							if(tmptab.column_size(asset)>1) {
// 								// CHECK!!! It looks as if I get more than one asym set, and overlapping indices. That cannot be the case, but better check carefully!!!
// //							txtout << "asym set: ";
// 								Ex comma("\\comma");
// 								
// 								iterator comma=tr.append_child(this_command, str_node("\\comma", str_node::b_none));
// 								for(unsigned int asel=0; asel<tmptab.column_size(asset); ++asel) {
// 									int indexnum=tmptab(asel,asset);
// 									Ex::index_iterator oth2=tr.begin_index(oth);
// 									oth2+=indexnum;
// 									comma.append_child(comma.begin(), *oth2);
// 									}
// 								remove_these.push_back(comma);
// 								}
// 							}
// 						}
// 					}
// 				}
// 			++oth;
// 			}
// 		}
// 	args_begin_=tr.end(); // FIXME: this is really ugly...
//	unsigned int thisnum=1;
//	tr.print_recursive_treeform(debugout, this_command, thisnum) << std::endl;

// 	// First determine the overlap factors.
 	multiplier_t divfactor1=1, divfactor2=1;
// 	sibling_iterator args_it=args_begin();
// 	while(args_it!=args_end()) {
// 		if(*args_it->name=="\\comma") {
// 			sibling_iterator cst=tr.begin(args_it);
// 			unsigned int overlap1=0, overlap2=0;
// 			while(cst!=tr.end(args_it)) {
// 				sibling_iterator ind=tr.begin(st);
// 				for(unsigned int pairnum=0; pairnum<tr.number_of_children(st); pairnum+=2) {
// 					if(ind->name==cst->name) ++overlap1;
// 					++ind;
// 					if(ind->name==cst->name) ++overlap2;
// 					++ind;
// 					}
// 				++cst;
// 				}
// 			divfactor1*=combin::fact(overlap1);
// 			divfactor2*=combin::fact(overlap2);
// 			}
// 		++args_it;
// 		}

//	unsigned int numm=1;
//	tr.print_recursive_treeform(debugout, st, numm);
	bool permute_second_set=(divfactor2>divfactor1);

	// construct the original permutation
	sibling_iterator ind=tr.begin(st);
	for(unsigned int pairnum=0; pairnum<tr.number_of_children(st); pairnum+=2) {
		if(permute_second_set) 
			++ind;
		ci.original.push_back(*ind);
		++ind;
		if(!permute_second_set)
			++ind;
		ci.sublengths.push_back(1);
		}
// 	// Now construct the output asym ranges.
// 	sibling_iterator it=args_begin();
// 	while(it!=args_end()) {
// 		if(*it->name=="\\comma") {
// 			sibling_iterator cst=tr.begin(it);
// 			combin::range_t asymrange1;
// 			while(cst!=tr.end(it)) {
// 				for(unsigned int i1=0; i1<ci.original.size(); ++i1) {
// 					if(ci.original[i1].name==cst->name) {
// 						asymrange1.push_back(i1);
// 						break;
// 						}
// 					}
// 				++cst;
// 				}
// 			ci.input_asym.push_back(asymrange1);
// 			}
// 		++it;
// 		}

	// do it
	ci.permute();

	// for each permutation, add a product of deltas with two indices
	for(unsigned int permnum=0; permnum<ci.size(); ++permnum) {
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
			tr.append_child(delta, ci[permnum][pairnum]);
			++ind;
			if(!permute_second_set)
				tr.append_child(delta, (iterator)(ind));
			++ind;
			}
		}

	if(rep.number_of_children(sum)==1) { // flatten \sum node if there was only one term
		rep.flatten(sum);
		rep.erase(sum);
		}
	st=tr.replace(st, rep.begin());
	cleanup_dispatch(kernel, tr, st);

	// remove the stuff we added
//	for(unsigned int i=0; i<remove_these.size(); ++i)
//		tr.erase(remove_these[i]);

	// std::cerr << "applied" << std::endl;

	return result_t::l_applied;
	}

