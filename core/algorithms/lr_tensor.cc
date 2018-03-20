
#include "Cleanup.hh"
#include "algorithms/lr_tensor.hh"
#include "properties/Tableau.hh"
#include "properties/FilledTableau.hh"

using namespace cadabra;

lr_tensor::lr_tensor(const Kernel& k, Ex& tr)
	: tab_basics(k, tr)
	{
	}

bool lr_tensor::can_apply(iterator it) 
	{
	if(*it->name=="\\prod") {
		sibling_iterator sib=tr.begin(it);
		tab1=tr.end(it);
		tab2=tr.end(it);
		while(sib!=tr.end(it)) {
			if(kernel.properties.get<Tableau>(sib)) {
				if(tab1==tr.end(it))
					tab1=sib;
				else {
					tab2=sib;
					break;
					}
				}
			++sib;
			}
		if(tab2!=tr.end(it)) return true;
		
		sib=tr.begin(it);
		tab1=tr.end(it);
		tab2=tr.end(it);
		while(sib!=tr.end(it)) {
			if(kernel.properties.get<FilledTableau>(sib)) {
				if(tab1==tr.end(it))
					tab1=sib;
				else {
					tab2=sib;
					break;
					}
				}
			++sib;
			}
		if(tab2!=tr.end(it)) return true;
		}
	return false;
	}

Algorithm::result_t lr_tensor::apply(iterator& it)
	{
	if(kernel.properties.get<Tableau>(tab1)) do_tableau(it);
	else                                     do_filledtableau(it);

	return result_t::l_applied;
	}

// The format is \ftab{a,b,c}{d,e}{f}.
//
void lr_tensor::do_filledtableau(iterator& it)
	{
	bool even_only=false;
	bool singlet_rules=false;

// FIXME: put arguments back
//	if(has_argument("EvenOnly"))
//		 even_only=true;
//	if(has_argument("SingletRules"))
//		 singlet_rules=true;

	uinttab_t one, two;

	// For efficiency we store integers in the tableaux, not the actual
	// Ex objects.
	uinttabs_t prod;

	num_to_it.clear();
	tree_to_numerical_tab(tab1, one);
	tree_to_numerical_tab(tab2, two);

	yngtab::LR_tensor(one,two,999,prod.get_back_insert_iterator());

	Ex rep;
	iterator top=rep.set_head(str_node("\\oplus"));

	if(singlet_rules) tabs_to_singlet_rules(prod, top);
	else              tabs_to_tree(prod, top, tab1, even_only);

	sibling_iterator sib=rep.begin(top);
	while(sib!=rep.end(top)) {
		 sib->fl.bracket=str_node::b_round;
		 ++sib;
		 }

	tr.replace(tab1, rep.begin());
	tr.erase(tab2);
	cleanup_dispatch(kernel, tr, it);
	}

void lr_tensor::do_tableau(iterator& it)
	{
	bool even_only=false;
// FIXME: put arguments back in
//	if(has_argument("EvenOnly"))
//		 even_only=true;

	yngtab::tableau one, two;
	yngtab::tableaux<yngtab::tableau> prod;
	
	sibling_iterator sib=tr.begin(tab1);
	while(sib!=tr.end(tab1)) {
		one.add_row(to_long(*sib->multiplier));
		++sib;
		}
	sib=tr.begin(tab2);
	while(sib!=tr.end(tab2)) {
		two.add_row(to_long(*sib->multiplier));
		++sib;
		}
	yngtab::LR_tensor(one,two,999,prod.get_back_insert_iterator());

	Ex rep;
	iterator top=rep.set_head(str_node("\\oplus"));
	yngtab::tableaux<yngtab::tableau>::tableau_container_t::iterator tabit=prod.storage.begin();
	while(tabit!=prod.storage.end()) {
		// Keep only the diagrams which lead to a singlet if requested.
		if(even_only)
			for(unsigned int r=0; r<(*tabit).number_of_rows(); ++r) 
				if((*tabit).row_size(r)%2!=0)
					goto next_tab;

		{iterator tt=tr.append_child(top, str_node(tab1->name));
		multiply(tt->multiplier, tabit->multiplicity);
		for(unsigned int r=0; r<(*tabit).number_of_rows(); ++r) 
			multiply(tr.append_child(tt, str_node("1"))->multiplier, (*tabit).row_size(r));
			}

	   next_tab:
		++tabit;
		}

	tr.replace(tab1, rep.begin());
	tr.erase(tab2);
	cleanup_dispatch(kernel, tr, it);
	}



