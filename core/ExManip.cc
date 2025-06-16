
#include "ExManip.hh"
#include "Kernel.hh"
#include "properties/Accent.hh"

using namespace cadabra;

ExManip::ExManip(const Kernel &k, Ex &e)
	: kernel(k)
	, tr(e)
	{
	}

bool ExManip::prod_wrap_single_term(iterator& it)
	{
	if(is_single_term(it)) {
		force_node_wrap(it, "\\prod");
		return true;
		}
	else return false;
	}

bool ExManip::sum_wrap_single_term(iterator& it)
	{
	if(is_single_term(it)) {
		force_node_wrap(it, "\\sum");
		return true;
		}
	else return false;
	}

void ExManip::force_node_wrap(iterator& it, std::string nm)
	{
	iterator prodnode=tr.insert(it, str_node(nm));
	sibling_iterator fr=it, to=it;
	++to;
	tr.reparent(prodnode, fr, to);
	prodnode->fl.bracket=it->fl.bracket;
	it->fl.bracket=str_node::b_none;
	if(nm!="\\sum") { // multipliers should sit on terms in a sum
		prodnode->multiplier=it->multiplier;
		one(it->multiplier);
		}
	it=prodnode;
	}

bool ExManip::prod_unwrap_single_term(iterator& it)
	{
	if((*it->name)=="\\prod") {
		if(tr.number_of_children(it)==1) {
			multiply(tr.begin(it)->multiplier, *it->multiplier);
			tr.begin(it)->fl.bracket=it->fl.bracket;
			tr.begin(it)->multiplier=it->multiplier;
			tr.flatten(it);
			it=tr.erase(it);
			return true;
			}
		}
	return false;
	}

bool ExManip::sum_unwrap_single_term(iterator& it)
	{
	if((*it->name)=="\\sum") {
		if(tr.number_of_children(it)==1) {
			multiply(tr.begin(it)->multiplier, *it->multiplier);
			tr.begin(it)->fl.bracket=it->fl.bracket;
			tr.begin(it)->multiplier=it->multiplier;
			tr.flatten(it);
			it=tr.erase(it);
			return true;
			}
		}
	return false;
	}

bool ExManip::is_single_term(iterator it)
	{
	if(*it->name!="\\prod" && *it->name!="\\sum" && *it->name!="\\asymimplicit"
	      && *it->name!="\\comma" && *it->name!="\\equals" && *it->name!="\\arrow") {

		if(tr.is_head(it) || *tr.parent(it)->name=="\\equals" || *tr.parent(it)->name=="\\int") return true;
		else if(*tr.parent(it)->name=="\\sum")
			return true;
		else if(*tr.parent(it)->name!="\\prod" && it->fl.parent_rel==str_node::parent_rel_t::p_none
		        && kernel.properties.get<Accent>(tr.parent(it))==0 ) {
#ifdef DEBUG
			std::cerr << "Found single term in " << tr.parent(it) << std::endl;
#endif
			return true;
			}
		}
	return false;
	}

bool ExManip::is_nonprod_factor_in_prod(iterator it)
	{
	if(*it->name!="\\prod" && *it->name!="\\sum" && *it->name!="\\asymimplicit" && *it->name!="\\comma"
	      && *it->name!="\\equals") {
		try {
			if(tr.is_head(it)==false && *tr.parent(it)->name=="\\prod")
				return true;
			}
		catch(navigation_error& ex) {
			// no parent, ignore
			}
		//		else return true;
		}
	return false;
	}

        
