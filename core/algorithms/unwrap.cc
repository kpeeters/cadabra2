
#include "Compare.hh"
#include "Cleanup.hh"
#include "algorithms/unwrap.hh"
#include "properties/Derivative.hh"
#include "properties/Accent.hh"
#include "properties/DependsBase.hh"
//#include "algorithms/prodcollectnum.hh"

unwrap::unwrap(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool unwrap::can_apply(iterator it)
	{
	const Derivative *der=kernel.properties.get<Derivative>(it);
	const Accent     *acc=kernel.properties.get<Accent>(it);
	if(der || acc) 
		return true;
	return false;
	}

// \del{a*f*A*b*C*d*e} -> a*f*\del{A*b*C}*d*e
//
// locate first dependent factor
// locate first following independent factor
// ...
//
// Should also work for brackets, like
// 
//    \poisson( A )( B ) -> 0
//
// if either A or B has vanishing poisson bracket, and
//
//    \poisson( N1 D1 )( N2 D2 ) -> N1 N2 \poisson( D1 )( D2 ).
//
// So all "derivatives" 

Algorithm::result_t unwrap::apply(iterator& it) 
	{
	// std::cerr << "Applying unwrap at " << Ex(it) << std::endl;
	result_t res = result_t::l_no_action;

	bool is_accent=kernel.properties.get<Accent>(it);

	// Wrap the 'derivative' in a product node so we can take
	// child nodes out and stuff them inside the product.

	iterator old_it=it;
	it=tr.wrap(it, str_node("\\prod"));

	bool all_arguments_moved_out=true;
	sibling_iterator acton=tr.begin(old_it);
	while(acton!=tr.end(old_it)) {
		// Only look at child nodes which are not indices.
		if(acton->is_index()==false) { 
			sibling_iterator derarg=acton;
			++acton; // don't use this anymore this loop

			if(*derarg->name=="\\sum") {
				all_arguments_moved_out=false;
				continue; // FIXME: Don't know how to handle this yet.
				}

			// std::cerr << "doing " << *derarg->name << std::endl;

			// If the argument of the derivative is not a product, make
			// into one, so we can handle everything using the same code.
			if(*derarg->name!="\\prod")
				derarg=tr.wrap(derarg, str_node("\\prod"));
			
			// Iterate over all arguments of the product sitting inside
			// the derivative (but see the comment above). 
			sibling_iterator factor=tr.begin(derarg);
			while(factor!=tr.end(derarg)) {
				// std::cerr << "checking " << Ex(factor) << std::endl;

				sibling_iterator nxt=factor;
				++nxt;
				bool move_out=true;
				
				// First figure out whether there is implicit dependence on the operator.
				// or on the coordinate.
				const DependsBase *dep=kernel.properties.get_composite<DependsBase>(factor);
				if(dep!=0) {
					// std::cerr << *factor->name << " depends" << std::endl;
					Ex deps=dep->dependencies(kernel, factor /* it */);
					sibling_iterator depobjs=deps.begin(deps.begin());
					while(depobjs!=deps.end(deps.begin())) {
//						std::cout << "?" << *it->name << " == " << *depobjs->name << std::endl;
//						if(subtree_exact_equal(it, depobjs)) { WRONG! Depends(\del) should work
// without having any arguments in \del. Otherwise we would need to write this as Depends(\del{#})
						if(old_it->name == depobjs->name) {
							move_out=false;
							break;
							}
						else {
							// compare all indices
							sibling_iterator indit=tr.begin(old_it);
							while(indit!=tr.end(old_it)) {
								if(indit->is_index()) {
									if(subtree_exact_equal(&kernel.properties, indit, depobjs)) {
										move_out=false;
										break;
										}
									}
								++indit;
								}
							if(!move_out) break;
							}
						++depobjs;
						}
					}
				
				// Finally, there may also be explicit dependence.
				if(move_out) {
					// FIXME: This certainly does not handle Y(a,b) correctly
               sibling_iterator chldit=tr.begin(factor);
               while(chldit!=tr.end(factor)) {
                  if(chldit->is_index()==false) {
                     sibling_iterator indit=tr.begin(old_it);
                     while(indit!=tr.end(old_it)) {
                        if(subtree_exact_equal(&kernel.properties, chldit, indit, 0)) {
                           move_out=false;
                           break;
                           }
                        ++indit;
                        }
                     if(!move_out) break;
                     }
                  ++chldit;
                  }
					}
				
				// If no dependence found, move this child out of the derivative.
				if(move_out) { 
               // FIXME: Does not handle subtree-compare properly, and does not look at the
					// commutativity property of the index wrt. the derivative is taken.
					int sign=1;
					if(factor!=tr.begin(derarg)) {
						Ex_comparator compare(kernel.properties);
						sign=compare.can_swap(tr.begin(derarg),factor,2);
						}
					
					res=result_t::l_applied;
					tr.move_before(old_it, factor);
					multiply(it->multiplier, sign);
					}
				
				factor=nxt;
				}

			// std::cerr << "after step " << Ex(it) << std::endl;
			
			// All factors in this argument have been handled now, let's see what's left.
			unsigned int derarg_num_chldr=tr.number_of_children(derarg);
			if(derarg_num_chldr==0) {
				// Empty accents should simply be ignored, but empty derivatives vanish.
				if(!is_accent) {
					zero(it->multiplier);
					break; // we can stop now, the entire expression is zero.
					}
				}
			else {
				all_arguments_moved_out=false;
				if(derarg_num_chldr==1) {
					 derarg=tr.flatten_and_erase(derarg);
					}
				}
			}
		else ++acton;
		}


	// All non-index arguments have now been handled. 
	if(all_arguments_moved_out && is_accent) {
		zero(it->multiplier);
		}
	else if(*it->multiplier!=0) {
		if(tr.number_of_children(it)==1) { // nothing was moved out
			tr.flatten(it);
			it=tr.erase(it);
			}
		else {
			 // Moving factors around has potentially led to a top-level product
			 // which contains children with non-unit multiplier.
			cleanup_dispatch(kernel, tr, it);
			
			// If the derivative acts on another derivative, we need
			// to un-nest the argument of the outer (and this situation
			// can only happen if there is only one non-index child node)
			iterator itarg=tr.begin(it);
			while(itarg->is_index()) 
				++itarg;

			cleanup_dispatch(kernel, tr, itarg);
			}
		}
	cleanup_dispatch(kernel, tr, it);

	// std::cerr << "unwrap done " << Ex(it) << std::endl;

	return res;
	}
