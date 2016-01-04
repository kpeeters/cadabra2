
#include "Functional.hh"

void cadabra::do_list(const Ex& tr, Ex::iterator it, std::function<void(Ex::iterator)> f)
	{
	if(*it->name=="\\expression")
		it=tr.begin(it);

   if(*it->name=="\\comma") {
		Ex::sibling_iterator sib=tr.begin(it);
		while(sib!=tr.end(it)) {
			Ex::sibling_iterator nxt=sib;
			++nxt;
			f(sib);
			sib=nxt;
			}
      }
	else {
		f(it);
		}
	}

Ex::iterator cadabra::find_in_list(const Ex& tr, Ex::iterator it, std::function<Ex::iterator(Ex::iterator)> f)
	{
	if(*it->name=="\\expression")
		it=tr.begin(it);

   if(*it->name=="\\comma") {
		Ex::sibling_iterator sib=tr.begin(it);
		while(sib!=tr.end(it)) {
			Ex::iterator ret = f(sib);
			if(ret!=tr.end())
				return ret;
			++sib;
			}
		return tr.end();
      }
	else {
		return f(it);
		}
	}

Ex cadabra::make_list(Ex el)
	{
	auto it=el.begin();
	if(*it->name=="\\expression") 
		it=el.begin(it);

	if(*it->name!="\\comma")
		el.wrap(it, str_node("\\comma"));

	return el;
	}


