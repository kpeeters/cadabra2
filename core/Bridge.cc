
#include "pythoncdb/py_ex.hh"
#include "Bridge.hh"
#include "algorithms/collect_terms.hh"

using namespace cadabra;

void pull_in(std::shared_ptr<Ex> ex, Kernel *kernel)
	{
	collect_terms rr(*kernel, *ex);

	bool acted=false;
	Ex::iterator it=ex->begin();
	while(it!=ex->end()) {
		if(*it->name=="@") {
			std::string pobj = *(Ex::begin(it)->name);
			std::shared_ptr<Ex> pull_ex = fetch_from_python(pobj);
			if(pull_ex) {
				acted=true;
				multiplier_t mult = *(it->multiplier);
				auto topnode_it   = pull_ex->begin();
				auto at_arg       = ex->begin(it);

				ex->erase(at_arg);           // erase argument of @
				it=ex->replace(it, *(topnode_it));     // replace @ with head of ex
				if(ex->number_of_children(topnode_it)>0) {
					Ex::sibling_iterator walk=ex->end(topnode_it);
					do {
						--walk;
						ex->prepend_child(it, (Ex::iterator)walk);
						}
					while(walk!=ex->begin(topnode_it));
					}
				// FIXME: prepend_children is broken!
				//				ex->prepend_children(it, ex->begin(topnode_it), ex->end(topnode_it)); // add children of ex
				multiply(it->multiplier, mult);
				rr.rename_replacement_dummies(it, false);
				}
			else throw ArgumentException("Python object '"+pobj+"' does not exist.");
			}
		++it;
		}

	//	if(acted)
	//		std::cerr << "pull_in done: " << *ex << std::endl;

	return;
	}

