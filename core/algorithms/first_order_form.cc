
#include "algorithms/first_order_form.hh"
#include "Algorithm.hh"
#include "Bridge.hh"
#include "Compare.hh"
#include "Equals.hh"
#include "Exceptions.hh"
#include "Functional.hh"
#include "properties/PartialDerivative.hh"

#define DEBUG __FILE__
#include "Debug.hh"

using namespace cadabra;

first_order_form::first_order_form(const Kernel &k, Ex &tr, Ex& funcs)
	: Algorithm(k, tr)
	, functions(funcs)
	{
	}

bool first_order_form::can_apply(iterator it)
	{
	// FIXME: move some of the checks in `apply` here.
	// determine that this is a set of ODEs or a single ODE
	// determine that each is an equality
	// determine that the highest-order derivatives appear linearly
  
	return *it->name=="\\comma";
	}

//	List odes(tr);
//	for(auto& ode: odes) {
//		}
//
//	List funs(functions);
//	for(auto& fun: funs) {
//		auto deps = kernel.properties.get<Depends>(fun.begin());
//		}


Algorithm::result_t first_order_form::apply(iterator& it)
	{
	// If we have a single ODE, convert to a list with one ODE in it.
	make_list(tr);

	// Ensure that each ODE is an equality. Move all terms to the lhs.
	do_list(tr, tr.begin(),
			  [this](iterator ode)
				  {
				  visit::Equals eq(kernel, tr, ode);
				  eq.move_all_to_lhs();
				  return true;
				  }
			  );
	
	// If we have a single function, convert to a list with one function in it.
	make_list(functions);

	// Determine the independent variable.
	tree_exact_less_obj comp(&kernel.properties);
	std::set<Ex, tree_exact_less_obj> all_deps(comp);
	do_list(functions, functions.begin(),
			  [this, &all_deps](Ex::iterator fun)
				  {
				  auto deps = dependencies(fun);
				  all_deps.insert(deps.begin(), deps.end());
				  return true;
				  }
			  );
	if(all_deps.size()!=1) {
		DEBUGLN( for(auto var: all_deps) std::cerr << "first_order_form: dep " << var << std::endl; );
		throw std::logic_error("first_order_form: more than one dependent variable present");
		}
	DEBUGLN( std::cerr << "first_order_form: found dependent variable " << *all_deps.begin() << std::endl; );
	
	// Walk all nodes in the ODEs, and determine the location of the
	// functions and their derivative orders. The goal latre will be to
	// write this as a linear system in which the highest-order
	// derivatives for each variable are the 'independents'. We then
	// solve for those to get the ODEs in canonical form.
	do_list(tr, it,
			  [this](Ex::iterator ode)
				  {
				  DEBUGLN( std::cerr << "process ODE " << ode << std::endl; );
				  // FIXME: move all terms to the lhs.
				  do_sum(tr, tr.begin(ode),
							[this](Ex::iterator term)
								{
								DEBUGLN( std::cerr << "  process term " << term << std::endl; );
								do_subtree(tr, term,
											  [this, term](Ex::iterator el)
												  {
												  // If we find one of our functions, walk up the tree to determine
												  // the order of the derivative.
												  do_list(functions, functions.begin(),
															 [this, el, term](Ex::iterator fun)
																 {
																 if(subtree_exact_equal(&kernel.properties, el, fun)) {
																	 DEBUGLN( std::cerr << "  Found " << fun << std::endl; );
																	 // Determine the derivative order.
																	 Ex::iterator walk=el;
																	 int order=0;
																	 do {
																		 walk = tr.parent(walk);
																		 const auto *pd = kernel.properties.get<PartialDerivative>(walk);
																		 if(pd) {
																			 ++order;
																			 DEBUGLN( std::cerr << "     derivative!" << std::endl; );
																			 }
																		 else break;
																		 } while(walk != term);
//																	 if(derivative_order_map.find(fun) == derivative_order_map.end())
//																		 derivative_order_map[fun] = order;
//																	 else
//																		 derivative_order_map[fun] = std::max(derivative_order_map[fun], order);
																	 }
																 return true;
																 }
															 );
												  return el;
												  }
											  );
								return true;
								}
							);
				  return true;
				  }
			  );

	// Move all terms except the highest-order derivative term to the
	// rhs of every ODE. If this term is not linear in that highest-derivative
	// term, throw an exception.
	
//	for(auto& fun: derivative_order_map) {
//		std::cerr << "function " << fun.first << " order " << fun.second << std::endl;
//		}
	
	// We now know the location of the highest order derivatives of each function.
	// Treat these as independent variables and construct a linear system so that
	// we can solve for these independent variables.

	//LinearExSystem ...;
	
	// 
	return result_t::l_no_action;
	}

        
