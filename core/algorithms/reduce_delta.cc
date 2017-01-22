
#include "Exceptions.hh"
#include "algorithms/reduce_delta.hh"
#include "properties/Integer.hh"
#include "properties/KroneckerDelta.hh"

using namespace cadabra;

reduce_delta::reduce_delta(const Kernel& k, Ex& e)
	: Algorithm(k, e)
	{
	}

bool reduce_delta::can_apply(iterator st)
	{
	const KroneckerDelta *kr=kernel.properties.get<KroneckerDelta>(st);
	if(kr) {
		if(tr.number_of_children(st)>2)
			return true;
		}
	return false;
	}

Algorithm::result_t reduce_delta::apply(iterator& st)
	{
	result_t res=result_t::l_no_action;

	int num=0;
	while(one_step_(st)) {
		++num;
		res=result_t::l_applied;
		if(tr.number_of_children(st)==0) {
			st->name=name_set.insert("1").first;
			break;
			}
		};
	return res;
	}

bool reduce_delta::one_step_(sibling_iterator dl)
	{
	sibling_iterator up=tr.begin(dl), dn;
	int flip=999, masterflip=1;
	while(up!=tr.end(dl)) {
		flip=masterflip;
		dn=tr.begin(dl);
		++dn;
		while(dn!=tr.end(dl)) {
			if(up->name==dn->name) 
				goto found;
			++dn; ++dn;
			flip=-flip;
			}
		++up; ++up;
		masterflip=-masterflip;
		}
   return false;
   found:
//	std::cerr << "reduce_delta: eliminating " << *up->name << " contraction." << std::endl;
//	{unsigned int num=1;
//	tr.print_recursive_treeform(debugout, dl, num) << std::endl;}
	// FIXME: use properties for the dimension!
//	txtout << *dl->multiplier << std::endl;
	const Integer *itg=kernel.properties.get<Integer>(up, true);
	int dim;
	if(itg) {
		const nset_t::iterator onept=name_set.insert("1").first;
		if(itg->difference.begin()->name==onept)
			dim=to_long(*itg->difference.begin()->multiplier);
		else
			throw ConsistencyException("Summation range for index is not an integer.");
		}
	else throw ConsistencyException("No dimension known for summation index.");

	int mult=flip*(dim-tr.number_of_children(dl)/2+1);
	multiply(dl->multiplier, (multiplier_t)(mult));
	multiply(dl->multiplier, multiplier_t(2)/((multiplier_t)(tr.number_of_children(dl))));
//	txtout << "flip =" << flip << std::endl;

	// remove the indices
	sibling_iterator up2=up; ++up2; ++up2;
	while(up2!=tr.end(dl)) {
		up->name=up2->name;
		++up; ++up;
		++up2; ++up2;
		}
	sibling_iterator dn2=dn; ++dn2; ++dn2;
	while(dn2!=tr.end(dl)) {
		dn->name=dn2->name;
		++dn; ++dn;
		++dn2; ++dn2;
		}
	sibling_iterator lst=tr.end(dl); --lst; --lst;
	lst=tr.erase(lst);
	tr.erase(lst);

//	{unsigned int num=1;
//	tr.print_recursive_treeform(debugout, dl, num) << std::endl;}
	
	return true;
	}
