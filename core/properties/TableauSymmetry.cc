
#include "properties/TableauSymmetry.hh"
#include "IndexIterator.hh"

std::string TableauSymmetry::name() const
	{
	return "TableauSymmetry";
	}

bool TableauSymmetry::parse(const Properties& properties, keyval_t& keyvals)
	{
   // Scan for the tableaux.
	keyval_t::const_iterator kvit=keyvals.begin();

	exptree::iterator indices;
	exptree::iterator shape;

	while(kvit!=keyvals.end()) {
		 if(kvit->first=="shape") 
			  shape=kvit->second;
		 if(kvit->first=="indices") 
			  indices=kvit->second;

		 if(shape!=tr.end() && indices!=tr.end()) {
			 // Make sure the shape and indices lists have a \comma node.
//			 tr.list_wrap_single_element(shape);
//			 tr.list_wrap_single_element(indices);
//			 
			 exptree::sibling_iterator si=shape.begin();
			 exptree::sibling_iterator ii=indices.begin();
			 
			 tab_t tab;
			 
			 keyval_t::const_iterator tmp=kvit;
			 ++tmp;
			 if(tmp!=keyvals.end()) {
				 if(tmp->first=="selfdual")
					 tab.selfdual_column=1;
				 else if(tmp->first=="antiselfdual")
					 tab.selfdual_column=-1;
				 }
			 
			 int rowind=0;
			 unsigned int tabdown=to_long(*si->multiplier);
//			 unsigned int numindices=number_of_indices(properties, pat);
			 // FIXME: we get the wrong pattern in case of a list! We should have
			 // been fed each individual item in the list, not the list itself.
//			  std::cout << numindices << " " << *pat->name << std::endl;
			 while(ii!=indices.end()) {
				 // FIXME: we cannot verify this at parse level, since we do not
				 // get passed the pattern at that stage.
				 // if(tabdown+1 > numindices) return false;

				 if(si==shape.end()) return false;
				 tab.add_box(rowind, to_long(*ii->multiplier));
				 ++ii;
				 if((--tabdown)==0 && ii!=indices.end()) {
					 ++si;
					 ++rowind;
					 tabdown=to_long(*si->multiplier);
					 }
				 }
			 tabs.push_back(tab);

//			 tr.list_unwrap_single_element(shape);
//			 tr.list_unwrap_single_element(indices);
//
			 shape=tr.end();
			 indices=tr.end();
			 }
		 ++kvit;
		}
	
	return true;
	}

void TableauSymmetry::display(std::ostream& str) const
	{
	for(unsigned int i=0; i<tabs.size(); ++i)
		str << tabs[i] << std::endl;
	}

unsigned int TableauSymmetry::size(exptree&, exptree::iterator) const
	{
	return tabs.size();
	}

TableauBase::tab_t TableauSymmetry::get_tab(exptree&, exptree::iterator, unsigned int num) const
	{
	assert(num<tabs.size());
	return tabs[num];
	}
