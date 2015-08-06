
#include "Functional.hh"

void cadabra::do_list(const exptree& tr, exptree::iterator it, std::function<void(exptree::iterator)> f)
	{
	if(*it->name=="\\expression")
		it=tr.begin(it);

   if(*it->name=="\\comma") {
		exptree::sibling_iterator sib=tr.begin(it);
		while(sib!=tr.end(it)) {
			f(sib);
			++sib;
			}
      }
	else {
		f(it);
		}
	}

exptree cadabra::make_list(exptree el)
	{
	auto it=el.begin();
	if(*it->name=="\\expression") 
		it=el.begin(it);

	if(*it->name!="\\comma")
		el.wrap(it, str_node("\\comma"));

	return el;
	}


