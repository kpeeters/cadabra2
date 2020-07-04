
#include "pythoncdb/py_ex.hh"
#include "pythoncdb/py_helpers.hh"
#include "pythoncdb/py_globals.hh"
#include "Bridge.hh"
#include "algorithms/collect_terms.hh"

using namespace cadabra;
namespace py = pybind11;

void pull_in(std::shared_ptr<Ex> ex, Kernel *kernel)
	{
	collect_terms rr(*kernel, *ex);

//	bool acted=false;
	Ex::iterator it=ex->begin();
	while(it!=ex->end()) {
		if(*it->name=="@") {
			std::string pobj = *(Ex::begin(it)->name);
			std::shared_ptr<Ex> pull_ex = fetch_from_python(pobj);
			if(pull_ex) {
//				acted=true;
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

void run_python_functions(std::shared_ptr<Ex> ex, Kernel *kernel)
	{
	if(kernel->call_embedded_python_functions==false)
		return;
	
	Ex::post_order_iterator it = ex->begin_post();
	auto locals=get_locals();
	while(it!=ex->end_post()) {
		auto nxt=it;
		++nxt;
		// Only call functions if the cadabra symbols have one or
		// more child nodes which all have bracket_t::b_none.
		Ex::sibling_iterator sib=ex->begin(it);
		if(sib==ex->end(it)) {
			it=nxt;
			continue;
			}
		
		bool cancall=true;
		while(sib!=ex->end(it)) {
			if(sib->fl.parent_rel!=str_node::parent_rel_t::p_none) {
				cancall=false;
				break;
				}
			++sib;
			}
		if(!cancall) {
			it=nxt;
			continue;
			}
		
		if(scope_has(locals, *it->name)) {
			//std::cerr << "can run function " << *it->name << std::endl;
			py::object fun=locals[(*it->name).c_str()];
			Ex::sibling_iterator sib=ex->begin(it);
			py::object res;
			
			if(sib!=ex->end(it)) {
				Ex tmp1(sib);
				++sib;
				if(sib!=ex->end(it)) {
					Ex tmp2(sib);
					++sib;
					if(sib!=ex->end(it)) {
						Ex tmp3(sib);
						res = fun(tmp1, tmp2, tmp3);
						++sib;
						if(sib!=ex->end(it)) {
							throw RuntimeException("Cannot yet call inline functions with more than 3 arguments.");
							}
						}
					else res = fun(tmp1, tmp2);
					}
				else res = fun(tmp1);
				}
			else res = fun();
			
			Ex repl = res.cast<Ex>();
			Ex::iterator tmpit=it;
			ex->move_ontop(tmpit, repl.begin());
			}
		it=nxt;
		}
	}
