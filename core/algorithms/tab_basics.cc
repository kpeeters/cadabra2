
#include "algorithms/tab_basics.hh"

tab_basics::tab_basics(Kernel& k, Ex& tr) 
	: Algorithm(k, tr)
	{
	}

unsigned int tab_basics::find_obj(const Ex& other)
	{
	for(unsigned int i=0; i<num_to_it.size(); ++i) {
		if(tree_exact_equal(&kernel.properties, num_to_it[i], other))
			return i;
		}
	throw std::logic_error("internal error in tab_basics::find_obj");
	}

void tab_basics::tree_to_numerical_tab(iterator tab1, uinttab_t& one) 
	{
	unsigned int prevsize=num_to_it.size();

	// First determine the sort order of the children of tab1.
	sibling_iterator sib=tr.begin(tab1);
	while(sib!=tr.end(tab1)) {
		if(*sib->name=="\\comma") {
			sibling_iterator sib2=tr.begin(sib);
			while(sib2!=tr.end(sib)) {
				 num_to_it.push_back(sib2);
				 ++sib2;
				 }
			}
		else {
			 num_to_it.push_back(sib);
			 }			
		 ++sib;
		 }

	tree_exact_less_obj cmp(&kernel.properties);
	std::vector<Ex::iterator>::iterator startit=num_to_it.begin();
	startit+=prevsize;
	std::sort(startit, num_to_it.end(), cmp);
	
	// Now fill the uinttab.
	sib=tr.begin(tab1);
	unsigned int currow=0;
	while(sib!=tr.end(tab1)) {
		if(*sib->name=="\\comma") {
			sibling_iterator sib2=tr.begin(sib);
			while(sib2!=tr.end(sib)) {
				 one.add_box(currow, find_obj(Ex(sib2)) );
				 ++sib2;
				}
			}
		else {
			 one.add_box(currow, find_obj(Ex(sib)) );
			 }
		++sib;
		++currow;
		}
	}

void tab_basics::tabs_to_singlet_rules(uinttabs_t& tabs, iterator top)
	{
	uinttabs_t::tableau_container_t::iterator tabit=tabs.storage.begin();

	while(tabit!=tabs.storage.end()) {
		 // Keep only the diagrams which lead to a singlet.
		 for(unsigned int r=0; r<(*tabit).number_of_rows(); ++r) 
			  if((*tabit).row_size(r)%2!=0)
					goto next_tab;

		 { 
		 iterator tprod=tr.append_child(top, str_node("\\prod"));
		 for(unsigned int r=0; r<(*tabit).number_of_rows(); ++r) {
			  for(unsigned int c=0; c<(*tabit).row_size(r); ++c) {
					iterator tt=tr.append_child(tprod, str_node("\\delta"));
					tr.append_child(tt, num_to_it[(*tabit)(r,c++)]);
					tr.append_child(tt, num_to_it[(*tabit)(r,c)]);
					}
			  }
		 }

  	    next_tab:
		 ++tabit;
		 }
	}


void tab_basics::tabs_to_tree(uinttabs_t& tabs, iterator top, iterator tabpat, bool even_only)
	{
	uinttabs_t::tableau_container_t::iterator tabit=tabs.storage.begin();

	while(tabit!=tabs.storage.end()) {
		 // Keep only the diagrams which lead to a singlet if requested.
		 if(even_only)
			  for(unsigned int r=0; r<(*tabit).number_of_rows(); ++r) 
					if((*tabit).row_size(r)%2!=0)
						 goto next_tab;

		 { iterator tt=tr.append_child(top, str_node(tabpat->name));
		 multiply(tt->multiplier, tabit->multiplicity);
		 for(unsigned int r=0; r<(*tabit).number_of_rows(); ++r) {
			  unsigned int rs=(*tabit).row_size(r);
			  if(rs==1) 
					tr.append_child(tt, num_to_it[(*tabit)(r,0)]);
			  else {
					iterator tmp=tr.append_child(tt, str_node("\\comma"));
					for(unsigned int c=0; c<rs; ++c) 
						 tr.append_child(tmp, num_to_it[(*tabit)(r,c)]);
					}
			  }
			  }

	    next_tab:
		 ++tabit;
		 }
	}

