
#include "Cleanup.hh"
#include "Exceptions.hh"

#include "algorithms/fierz.hh"

#include "properties/GammaMatrix.hh"
#include "properties/Integer.hh"
#include "properties/DiracBar.hh"

using namespace cadabra;

fierz::fierz(const Kernel& k, Ex& e, Ex& args)
	: Algorithm(k, e), spinor_list(Ex(args.begin()))
	{
	if(*(spinor_list.begin()->name)!="\\comma")
		throw ArgumentException("fierz: need a list of spinors");

	if(tr.number_of_children(spinor_list.begin())!=4)
		throw ArgumentException("fierz: need a list of 4 spinors.");
	}

bool fierz::can_apply(iterator it) 
	{
	if(*it->name!="\\prod") return false;

	// Find a Dirac bar, and then continue inside the product
	// to find the gamma matrix, fermion and second fermi bilinear.

	sibling_iterator sib=tr.begin(it);
	const Integer *indit=0;
	while(sib!=tr.end(it)) {
		const DiracBar *db=kernel.properties.get_composite<DiracBar>(sib);
		if(db) {
			// std::cerr << "found db" << Ex(sib) << std::endl;
			spin1=sib;
			prop1=kernel.properties.get_composite<Spinor>(spin1);
			sibling_iterator ch=sib;
			const GammaMatrix *gmnxt=0;
			const Spinor      *spnxt=0;
			// Skip to next spinor-index carrying object
			do {
				++ch;
				if(ch==tr.end(it)) break;
				gmnxt=kernel.properties.get_composite<GammaMatrix>(ch);
				spnxt=kernel.properties.get_composite<Spinor>(ch);
				} while(gmnxt==0 && spnxt==0);
			if(gmnxt) {
				// std::cerr << "found gam" << Ex(ch) << std::endl;
				// FIXME: should also work when there is a unit matrix in between.
				indit=kernel.properties.get_composite<Integer>(ch.begin(), true);
				indprop=kernel.properties.get_composite<Indices>(ch.begin(), true);
				if(!indit || !indprop) return false;
				dim=to_long(*indit->difference.begin()->multiplier);
				if(dim==1)
					return false;
				
				gam1=ch;
				// Skip to next spinor-index carrying object
				do {
					++ch;
					if(ch==tr.end(it)) break;
					spnxt=kernel.properties.get_composite<Spinor>(ch);
					gmnxt=kernel.properties.get_composite<GammaMatrix>(ch);
					} while(gmnxt==0 && spnxt==0);
				prop2=spnxt;
				if(prop2) { // one fermi bilinear found.
					// std::cerr << "found spin2 " << Ex(ch) << std::endl;
					spin2=ch;
					// Skip to next spinor-index carrying object
					do {
						++ch;
						if(ch==tr.end(it)) break;
						spnxt=kernel.properties.get_composite<Spinor>(ch);
						gmnxt=kernel.properties.get_composite<GammaMatrix>(ch);
						} while(gmnxt==0 && spnxt==0);
					db=kernel.properties.get_composite<DiracBar>(ch);
					if(db) {
						// std::cerr << "found db2" << std::endl;
						spin3=ch;
						prop3=spnxt;
						// Skip to next spinor-index carrying object
						do {
							++ch;
							if(ch==tr.end(it)) break;
							spnxt=kernel.properties.get_composite<Spinor>(ch);
							gmnxt=kernel.properties.get_composite<GammaMatrix>(ch);
							} while(gmnxt==0 && spnxt==0);
						if(gmnxt) {
							gam2=ch;
							// std::cerr << "found gam2: " << Ex(gam2) << std::endl;
							// Skip to next spinor-index carrying object
							do {
								++ch;
								if(ch==tr.end(it)) break;
								spnxt=kernel.properties.get_composite<Spinor>(ch);
								gmnxt=kernel.properties.get_composite<GammaMatrix>(ch);
								} while(gmnxt==0 && spnxt==0);
							prop4=spnxt;
							if(prop4) {
								// std::cerr << "found spin4" << std::endl;
								spin4=ch;
								return true;
								}
							}
						}
					}
				}
			}
		++sib;
		}
	return false;
	}

Algorithm::result_t fierz::apply(iterator& it)
	{
	sibling_iterator spt=spinor_list.begin(spinor_list.begin());

	// Catch terms with spinors in the right order.
	if(subtree_equal(&kernel.properties, tr.begin(spin1), spt)) {
		++spt;
		if(subtree_equal(&kernel.properties, spin2, spt)) {
			++spt;
			if(subtree_equal(&kernel.properties, tr.begin(spin3), spt)) {
				++spt;
				if(subtree_equal(&kernel.properties, spin4, spt)) {
//					txtout << "found term in right order" << std::endl;
					return result_t::l_no_action;
					}
				}
			}
		}

	// Catch terms with right spinors but wrong order.
	bool doit=false;
	spt=spinor_list.begin(spinor_list.begin());
	if(subtree_equal(&kernel.properties, tr.begin(spin1), spt)) {
		++spt;
		if(subtree_equal(&kernel.properties, spin4, spt)) {
			++spt;
			if(subtree_equal(&kernel.properties, tr.begin(spin3), spt)) {
				++spt;
				if(subtree_equal(&kernel.properties, spin2, spt)) {
//					txtout << "found term in wrong order" << std::endl;
					doit=true;
					}
				}
			}
		}
	if(!doit) return result_t::l_no_action;

//	txtout << "going to Fierz" << std::endl;

	Ex rep("\\sum");
	
	index_map_t ind_free, ind_dummy; 
	classify_indices(it, ind_free, ind_dummy);
	spinordim=(1 << dim/2);
	int maxind=dim;
	if(prop1->weyl || dim%2==1) 
		maxind/=2;

	for(int i=0; i<=maxind; ++i) {
//		std::cerr << i << " of " << maxind << std::endl;

		// Make a copy of this term, moving the gamma matrices into the 
		// first factor and inserting projector gamma matrices as well.
		Ex cpyterm("\\prod");
		cpyterm.begin()->multiplier=it->multiplier;
		multiply(cpyterm.begin()->multiplier, multiplier_t(-1)/multiplier_t(spinordim));
		if(i>0)
			multiply(cpyterm.begin()->multiplier, multiplier_t(1)/multiplier_t(combin::fact<int>(i)));
		sibling_iterator cpit=tr.begin(it);

		// Copy and put the gammas and projector gammas in the right spot.
		iterator locgam1,  locgam2;  // locations of the projector gammas
		while(cpit!=tr.end(it)) {
			iterator tmpit;
			if(cpit==spin2) 
				tmpit=cpyterm.append_child(cpyterm.begin(), spin4);
			else if(cpit==spin4)
				tmpit=cpyterm.append_child(cpyterm.begin(), spin2);
			else tmpit=cpyterm.append_child(cpyterm.begin(), (iterator)cpit);

			if(cpit==gam1) {
				if(i>0) {
					locgam1=cpyterm.append_child(cpyterm.begin(), gam1);
					cpyterm.erase_children(locgam1);
					}
				cpyterm.append_child(cpyterm.begin(), gam2);
				}
			if(cpit==gam2) {
				locgam2=tmpit;
				if(i==0) cpyterm.erase(locgam2);
				else 		cpyterm.erase_children(locgam2);
//				if(i>0)
//					std::cerr << "New gamma reads " << Ex(locgam2) << std::endl;
				}
			
			++cpit;
			}

		// Insert the indices on the projector gammas.
		index_map_t ind_added;
		for(int j=1; j<=i; ++j) { 
			Ex newdum=get_dummy(indprop, &ind_free, &ind_dummy, &ind_added);
			iterator loc1=cpyterm.append_child(locgam1, newdum.begin());
			ind_added.insert(index_map_t::value_type(newdum, loc1));
			if(indprop->position_type==Indices::free)
				loc1->fl.parent_rel=str_node::p_sub;
			else
				loc1->fl.parent_rel=str_node::p_super;
			// Add the indices in opposite order in the second gamma matrix
//			std::cerr << "inserting " << newdum << " at " << Ex(locgam2) << std::endl;
			iterator loc2=cpyterm.prepend_child(locgam2, newdum.begin());
			loc2->fl.parent_rel=str_node::p_sub;
			}
		
//		std::cerr << cpyterm << std::endl;
		rep.append_child(rep.begin(), cpyterm.begin());
		}

//	std::cerr << rep << std::endl;

	it=tr.replace(it, rep.begin());
	cleanup_dispatch(kernel, tr, it);

	return result_t::l_applied;
	}
