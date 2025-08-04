
#include "Sum.hh"
#include "Exceptions.hh"
#include "Functional.hh"
#include "Compare.hh"

using namespace cadabra;

visit::Sum::Sum(const Kernel &k, Ex &e, Ex::iterator i)
	: ReservedNode(k, e, i)
	{
	if(*top->name != "\\sum")
		throw ConsistencyException("Top not a sum.");
	}

std::vector<Ex::iterator> visit::Sum::find_terms_containing(Ex::iterator fnd) const
	{
	std::vector<Ex::iterator> ret;

	do_list(tr, top,
			  [this, &fnd, &ret](iterator term)
				  {
				  do_subtree(tr, term,
								 [this, &fnd, &ret](iterator el)
									 {
									 if(subtree_exact_equal(&kernel.properties, el, fnd)) {
										 ret.push_back(el);
										 }
									 return el;
									 }
								 );
				  return true;
				  }
			  );
	
	return ret;
	}
