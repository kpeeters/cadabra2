
#include "algorithms/split_index.hh"

split_index::split_index(const Kernel& k, Ex& tr, Ex& triple)
	: Algorithm(k, tr), part1_is_number(false), part2_is_number(false)
	{
	iterator top=triple.begin(triple.begin());
	if(*(top->name)!="\\comma") {
		std::cout << "not comma" << std::endl;		
		throw ArgumentException("split_index: Need a list of three index names.");
		}
	else if(triple.number_of_children(top)!=3) {
		std::cout << "not 3" << std::endl;
		throw ArgumentException("split_index: Need a list of three (no more, no less) index names.");
		}

	sibling_iterator iname=triple.begin(top);
	full_class=kernel.properties.get<Indices>(iname, true);
	++iname;
	if(iname->is_integer()) {
		part1_is_number=true;
		num1=to_long(*(iname->multiplier));
		}
	else part1_class=kernel.properties.get<Indices>(iname, true);
	++iname;
	if(iname->is_integer()) {
		part2_is_number=true;
		num2=to_long(*(iname->multiplier));
		}
	else part2_class=kernel.properties.get<Indices>(iname, true);
	if(full_class && (part1_is_number || part1_class) && (part2_is_number || part2_class) )
		return;
	
	std::cout << "no type" << std::endl;
	throw ArgumentException("split_index: The index types of (some of) these indices are not known.");
	}

bool split_index::can_apply(iterator it)
	{
	return is_termlike(it);
	}

Algorithm::result_t split_index::apply(iterator& it)
	{
	result_t ret=result_t::l_no_action;

	Ex rep;
	rep.set_head(str_node("\\sum"));
	Ex workcopy(it); // so we can make changes without spoiling the big tree
//	assert(*it->multiplier==1); // see if this made a difference

//	txtout << "split index acting at " << *(it->name) << std::endl;

	// we only replace summed indices, so first find them.
	index_map_t ind_free, ind_dummy;
	classify_indices(workcopy.begin(), ind_free, ind_dummy);
//	txtout << "indices classified" << std::endl;

	index_map_t::iterator prs=ind_dummy.begin();
	while(prs!=ind_dummy.end()) {
		const Indices *tcl=kernel.properties.get<Indices>((*prs).second, true);
		if(tcl) {
			if((*tcl).set_name==(*full_class).set_name) {
				Ex dum1,dum2;
				if(!part1_is_number)
					dum1=get_dummy(part1_class, it);
				index_map_t::iterator current=prs;
				while(current!=ind_dummy.end() && tree_exact_equal(&kernel.properties, (*prs).first,(*current).first,true)) {
					if(part1_is_number) {
						node_integer(current->second, num1);
//						(*prs).second->name=name_set.insert(to_string(num1)).first;
						}
					else {
//						txtout << "going to replace" << std::endl;
						(*current).second=tr.replace_index((*current).second, dum1.begin(), true);
//						txtout << "replaced" << std::endl;
						}
					// Important: restoring (*prs).second in the line above.
					++current;
					}
				rep.append_child(rep.begin(), workcopy.begin());
				current=prs;
				if(!part2_is_number) 
					dum2=get_dummy(part2_class, it);
				while(current!=ind_dummy.end() && tree_exact_equal(&kernel.properties, (*prs).first,(*current).first,true)) {
					if(part2_is_number) {
						node_integer(current->second, num2);
//						(*prs).second->name=name_set.insert(to_string(num2)).first;
						}
					else tr.replace_index((*current).second,dum2.begin(), true);
					++current;
					}
				rep.append_child(rep.begin(), workcopy.begin());
				// Do not copy the multiplier; it has already been copied by cloning the original into workcopy.
            //	rep.begin()->multiplier=it->multiplier;
//				txtout << "cleaning up" << std::endl;
//				rep.print_recursive_treeform(txtout, rep.begin());
				it=tr.replace(it, rep.begin());

				// FIXME: need to cleanup nests

//				cleanup_nests(tr, it);

				ret=result_t::l_applied;
				break;
				}
			}
		// skip other occurrances of this index
		index_map_t::iterator current=prs;
		while(prs!=ind_dummy.end() && tree_exact_equal(&kernel.properties, (*prs).first,(*current).first,false))
			++prs;
		}

	return ret;
	}
