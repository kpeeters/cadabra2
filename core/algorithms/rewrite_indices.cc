
#include "Cleanup.hh"
#include "algorithms/rewrite_indices.hh"
#include "properties/Derivative.hh"
#include "properties/Indices.hh"

rewrite_indices::rewrite_indices(const Kernel& k, Ex& e, Ex& pref, Ex& conv)
	: Algorithm(k, e), preferred(pref), converters(conv)
	{
	auto c=converters.begin();
	if(*c->name!="\\comma") {
		converters.wrap(c, str_node("\\comma"));
		}
	}

bool rewrite_indices::can_apply(iterator it) 
	{
	is_derivative_argument=false;
	if(*it->name=="\\prod" || is_single_term(it))
		return true;

	if(tr.is_valid(tr.parent(it))) { // FIXME: should eventually go into prod_wrap_single_term
		const Derivative *der=kernel.properties.get<Derivative>(tr.parent(it));
		if(der) {
			if(it->fl.parent_rel==str_node::p_none) {
				is_derivative_argument=true;
				return true;
				}
			}
		}

	return false;
	}

Algorithm::result_t rewrite_indices::apply(iterator& it) 
	{
	result_t res=result_t::l_no_action;

	if(is_derivative_argument) force_prod_wrap(it);
	else                       prod_wrap_single_term(it);

	index_map_t ind_free, ind_dummy;
	classify_indices(it, ind_free, ind_dummy);

	// Put arguments in canonical form.

	sibling_iterator objs=preferred.begin();
	sibling_iterator vielb=converters.begin().begin();
	std::cerr << "preferred: " << Ex(objs) << std::endl;
	std::cerr << "vielbein: "  << Ex(vielb) << std::endl;

	// Determine which conversion types are possible.
	// itype1 and itype2 are the index types of the 1st and 2nd index of the 
	// converter (i.e. vielbein or metric).
	
	sibling_iterator vbind=tr.begin(vielb);
	const Indices *itype1=kernel.properties.get<Indices>(vbind, true);
	str_node::parent_rel_t pr1=vbind->fl.parent_rel;
	++vbind;
	const Indices *itype2=kernel.properties.get<Indices>(vbind, true);
	str_node::parent_rel_t pr2=vbind->fl.parent_rel;	

	// Since this algorithm works both on dummy indices and on free
	// ones, we merge the two.

	ind_dummy.insert(ind_free.begin(), ind_free.end());

	// Go through all indices, determine on which object they sit,
	// and see if that object appears in the list of preferred-form
	// objects. If so, take appropriate action.

	// 'dit' is the index under consideration for a rewrite.
	index_map_t::const_iterator dit=ind_dummy.begin();
	while(dit!=ind_dummy.end()) {
		sibling_iterator par=tr.parent(dit->second);
		for(sibling_iterator prefit=tr.begin(objs); prefit!=tr.end(objs); ++prefit) {
			std::cerr << "one " << Ex(par) << ", " << Ex(prefit) << std::endl;
			if(subtree_equal(&kernel.properties, par, prefit, 1, false)) {
//				txtout << "found " << *par->name << std::endl;
				// Determine whether the indices are of preferred type or not.
				int num=std::distance(tr.begin(par), (sibling_iterator)dit->second);
				const Indices *origtype=kernel.properties.get<Indices>(dit->second, true);
				if(!origtype) {
					throw ArgumentException("Need to know about the index type of index "+*dit->second->name+".");
					}
//				txtout << "index " << *dit->second->name << "(" << num << ") has type " 
//						 << origtype->set_name << std::endl;

				// 'walk' is the index on the preferred form of the tensor, corresponding
				// to the index on the original tensor which is currently under consideration 
				// for change.
				sibling_iterator walk=begin_index(prefit);
				while(num-- > 0)
					++walk;

				const Indices *newtype=kernel.properties.get<Indices>(walk, true);
				if(!newtype) {
					throw ArgumentException("Need to know about the index type of index "+*walk->name+".");
					}
//				txtout << "prefi " << *walk->name << "(" << num << ") has type " 
//						 << newtype->set_name << std::endl;

				if(newtype->set_name == origtype->set_name) {
//					txtout << "index already has same type" << std::endl;
					if(origtype->position_type==Indices::free || walk->fl.parent_rel==dit->second->fl.parent_rel) {
//						txtout << "and position is also already fine" << std::endl;
						continue; // already fine
						}
//					txtout << "need to raise/lower" << std::endl;
					}

				Ex repvb(vielb);
				sibling_iterator vbi1=repvb.begin(repvb.begin());
				sibling_iterator vbi2=vbi1; ++vbi2;

				if(origtype->set_name == itype1->set_name && newtype->set_name == itype2->set_name) {
//					txtout << "hit 1" << std::endl;
					if( itype1->position_type==Indices::free || dit->second->fl.parent_rel == pr1 ) {
						if( itype2->position_type==Indices::free || walk->fl.parent_rel != pr2 ) {
//							txtout << "activate" << std::endl;
							tr.replace_index(vbi1, dit->second);
							Ex nd=get_dummy(itype2, par);
							tr.replace_index(vbi2, nd.begin());
							tr.replace_index(dit->second, nd.begin())->fl.parent_rel=walk->fl.parent_rel;
							}
						else continue;
						}
					else continue;
					}
				else if(origtype->set_name == itype2->set_name && newtype->set_name == itype1->set_name) {
//					txtout << "hit 2" << std::endl;
					if( itype2->position_type==Indices::free || dit->second->fl.parent_rel == pr2 ) {
						if( itype1->position_type==Indices::free || walk->fl.parent_rel != pr1 ) {
							tr.replace_index(vbi2, dit->second);
							Ex nd=get_dummy(itype1, par);
							tr.replace_index(vbi1,nd.begin());
							tr.replace_index(dit->second,nd.begin())->fl.parent_rel=walk->fl.parent_rel;
							}
						else continue;
						}
					else continue;
					}
				else continue; // next index

				// Insert the conversion object.
				iterator vbit;
				if(*tr.parent(par)->name=="\\sum") { // need to wrap inside a product
					iterator prod=tr.wrap(par, str_node("\\prod"));
					prod->fl.bracket=par->fl.bracket;
					par->fl.bracket=str_node::b_none;
					vbit=tr.append_child(prod, repvb.begin());
					res=result_t::l_applied;
					}
				else {
					assert(*tr.parent(par)->name=="\\prod");
					vbit=tr.append_child((iterator)tr.parent(par), repvb.begin());
					vbit->fl.bracket=par->fl.bracket;
					res=result_t::l_applied;
					}
				}
			}
		++dit;
		}

	prod_unwrap_single_term(it);

	return res;
	}
