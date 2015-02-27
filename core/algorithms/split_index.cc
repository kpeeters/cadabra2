
#include "algorithms/split_index.hh"
#include "properties/Indices.hh"

split_index::split_index(Kernel& k, exptree& tr, exptree& triple)
	: Algorithm(k, tr), part1_is_number(false), part2_is_number(false)
	{
	if(number_of_args()==1)
		if(*(args_begin()->name)=="\\comma") 
			if(tr.number_of_children(args_begin())==3) {
				iterator trip=args_begin();
				sibling_iterator iname=tr.begin(trip);
				full_class=properties::get<Indices>(iname, true);
				++iname;
				if(iname->is_integer()) {
					part1_is_number=true;
					num1=to_long(*(iname->multiplier));
					}
				else part1_class=properties::get<Indices>(iname, true);
				++iname;
				if(iname->is_integer()) {
					part2_is_number=true;
					num2=to_long(*(iname->multiplier));
					}
				else part2_class=properties::get<Indices>(iname, true);
				if(full_class && (part1_is_number || part1_class) && (part2_is_number || part2_class) )
					return;
				txtout << "The index types of (some of) these indices are not known." << std::endl;
				}
	throw algorithm::constructor_error();
	}

void split_index::description() const
	{
	txtout << "Split a dummy index into two." << std::endl;
	}

bool split_index::can_apply(iterator it)
	{
	// act on a single term in a sum, or on an isolated expression at the top node.
	if(*(it->name)!="\\sum") 
		if(*(tr.parent(it)->name)=="\\sum" || 
			( *(tr.parent(it)->name)=="\\expression" && !(*(it->name)=="\\asymimplicit"))) return true;

	return false;
	}

Algorithm::result_t split_index::apply(iterator& it)
	{
	exptree rep;
	rep.set_head(str_node("\\sum"));
	exptree workcopy(it); // so we can make changes without spoiling the big tree
//	assert(*it->multiplier==1); // see if this made a difference

//	txtout << "split index acting at " << *(it->name) << std::endl;

	// we only replace summed indices, so first find them.
	index_map_t ind_free, ind_dummy;
	classify_indices(workcopy.begin(), ind_free, ind_dummy);
//	txtout << "indices classified" << std::endl;

	index_map_t::iterator prs=ind_dummy.begin();
	while(prs!=ind_dummy.end()) {
		const Indices *tcl=properties::get<Indices>((*prs).second, true);
		if(tcl) {
			if((*tcl).set_name==(*full_class).set_name) {
				exptree dum1,dum2;
				if(!part1_is_number)
					dum1=get_dummy(part1_class, it);
				index_map_t::iterator current=prs;
				while(current!=ind_dummy.end() && tree_exact_equal((*prs).first,(*current).first,true)) {
					if(part1_is_number) {
						node_integer(current->second, num1);
//						(*prs).second->name=name_set.insert(to_string(num1)).first;
						}
					else {
//						txtout << "going to replace" << std::endl;
						(*current).second=tr.replace_index((*current).second, dum1.begin());
//						txtout << "replaced" << std::endl;
						}
					// Important: restoring (*prs).second in the line above.
					++current;
					}
				rep.append_child(rep.begin(), workcopy.begin());
				current=prs;
				if(!part2_is_number) 
					dum2=get_dummy(part2_class, it);
				while(current!=ind_dummy.end() && tree_exact_equal((*prs).first,(*current).first,true)) {
					if(part2_is_number) {
						node_integer(current->second, num2);
//						(*prs).second->name=name_set.insert(to_string(num2)).first;
						}
					else tr.replace_index((*current).second,dum2.begin());
					++current;
					}
				rep.append_child(rep.begin(), workcopy.begin());
				// Do not copy the multiplier; it has already been copied by cloning the original into workcopy.
            //	rep.begin()->multiplier=it->multiplier;
//				txtout << "cleaning up" << std::endl;
//				rep.print_recursive_treeform(txtout, rep.begin());
				it=tr.replace(it, rep.begin());
				cleanup_nests(tr, it);
				expression_modified=true;
				break;
				}
			}
		// skip other occurrances of this index
		index_map_t::iterator current=prs;
		while(prs!=ind_dummy.end() && tree_exact_equal((*prs).first,(*current).first,false))
			++prs;
		}

	if(expression_modified) return l_applied;
	else                    return l_no_action;
	}
