
#include "PythonCdb.hh"

// make boost::python understand std::shared_ptr when compiled with clang.
// http://stackoverflow.com/questions/13986581/using-boost-python-stdshared-ptr

// This now works on both Linux and OS X El Capitan, but your mileage may vary. 
//
#if (defined(__clang__) && defined(__linux__)) || defined(__GNUG__)
//#ifdef __GNUG__
namespace boost {
//#endif
template<typename T>
T *get_pointer(std::shared_ptr<T> p)
		{
		return p.get();
		}
#ifdef __GNUG__
}
  #endif
#endif

#include "Parser.hh"
#include "Exceptions.hh"
#include "DisplayTeX.hh"
#include "DisplaySympy.hh"
#include "DisplayTerminal.hh"
#include "Cleanup.hh"
#include "PreClean.hh"
#include "PythonException.hh"

#include <boost/python/implicit.hpp>
#include <boost/parameter/preprocessor.hpp>
#include <boost/parameter/python.hpp>
#include <boost/python.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/def.hpp>
#include <boost/python/module.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/slice.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <sstream>
#include <memory>

// Properties.

#include "properties/Accent.hh"
#include "properties/AntiCommuting.hh"
#include "properties/AntiSymmetric.hh"
#include "properties/Commuting.hh"
#include "properties/Coordinate.hh"
#include "properties/Depends.hh"
#include "properties/DependsInherit.hh"
#include "properties/Derivative.hh"
#include "properties/DiracBar.hh"
#include "properties/GammaMatrix.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/DAntiSymmetric.hh"
#include "properties/Diagonal.hh"
#include "properties/Distributable.hh"
#include "properties/EpsilonTensor.hh"
#include "properties/FilledTableau.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/Indices.hh"
#include "properties/IndexInherit.hh"
#include "properties/Integer.hh"
#include "properties/InverseMetric.hh"
#include "properties/KroneckerDelta.hh"
#include "properties/LaTeXForm.hh"
#include "properties/Metric.hh"
#include "properties/NonCommuting.hh"
#include "properties/NumericalFlat.hh"
#include "properties/PartialDerivative.hh"
#include "properties/RiemannTensor.hh"
#include "properties/SatisfiesBianchi.hh"
#include "properties/SelfAntiCommuting.hh"
#include "properties/SelfCommuting.hh"
#include "properties/SelfNonCommuting.hh"
#include "properties/SortOrder.hh"
#include "properties/Spinor.hh"
#include "properties/Symmetric.hh"
#include "properties/Tableau.hh"
#include "properties/TableauSymmetry.hh"
#include "properties/Traceless.hh"
#include "properties/Weight.hh"
#include "properties/WeightInherit.hh"
#include "properties/WeylTensor.hh"

// Algorithms.

#include "algorithms/canonicalise.hh"
#include "algorithms/collect_factors.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/complete.hh"
#include "algorithms/decompose_product.hh"
#include "algorithms/distribute.hh"
#include "algorithms/eliminate_kronecker.hh"
#include "algorithms/epsilon_to_delta.hh"
#include "algorithms/evaluate.hh"
#include "algorithms/expand_delta.hh"
#include "algorithms/expand_diracbar.hh"
#include "algorithms/factor_in.hh"
#include "algorithms/flatten_sum.hh"
#include "algorithms/indexsort.hh"
#include "algorithms/join_gamma.hh"
#include "algorithms/keep_terms.hh"
#include "algorithms/lr_tensor.hh"
#include "algorithms/order.hh"
#include "algorithms/product_rule.hh"
#include "algorithms/rename_dummies.hh"
#include "algorithms/split_index.hh"
#include "algorithms/substitute.hh"
#include "algorithms/sym.hh"
#include "algorithms/take_match.hh"
#include "algorithms/replace_match.hh"
#include "algorithms/unwrap.hh"
#include "algorithms/vary.hh"
#include "algorithms/young_project.hh"
#include "algorithms/young_project_product.hh"
#include "algorithms/young_project_tensor.hh"


// TODO: 
//
// - Make a list of useful things to pass to functions which are not Ex objects. 
//   Then abstract a new def_algo from there.
//
//        keep_terms:  list of integers
//        young_project: one list of integers describing tableau shape, one list of integers indicating indices.
//        

template<typename T>
std::vector<T> to_std_vector(const boost::python::list& iterable )
	{
	return std::vector<T>( boost::python::stl_input_iterator< T >( iterable ),
								  boost::python::stl_input_iterator< T >( ) );
	}

bool output_ipython=false;
bool post_process_enabled=true;

// Output routines in display, input, latex and html formats (the latter two
// for use with IPython).

std::string Ex_str_(const Ex& ex) 
	{
 	std::ostringstream str;
// 
// //	if(state()==Algorithm::result_t::l_no_action)
// //		str << "(unchanged)" << std::endl;
// 	DisplayTeX dt(get_kernel_from_scope()->properties, ex);

	DisplayTerminal dt(*get_kernel_from_scope(), ex);
	dt.output(str);

	return str.str();
	}

std::string Ex_latex_(const Ex& ex) 
	{
 	std::ostringstream str;
	DisplayTeX dt(*get_kernel_from_scope(), ex);
	dt.output(str);
	return str.str();
	}

std::string Ex_repr_(const Ex& ex) 
	{
	Ex::iterator it = ex.begin();
	std::ostringstream str;
	ex.print_entire_tree(str);
	return str.str();
	}

boost::python::object Ex_to_Sympy(const Ex& ex)
	{
	// Check to see if the expression is a scalar without dummy indices.
//	Algorithm::index_map_t ind_free, ind_dummy;
//	Algorithm::classify_indices(ex.begin(), ind_free, ind_dummy);
//	if(ind_dummy.size()>0) 
//		throw NonScalarException("Expression contains dummy indices.");
//	if(ind_free.size()>0) 
//		throw NonScalarException("Expression contains free indices.");

	// Call sympify on our textual representation.
	auto module = boost::python::import("sympy.parsing.sympy_parser");
	auto parse  = module.attr("parse_expr");
	std::ostringstream str;
	DisplaySympy dt(*get_kernel_from_scope(), ex);
	dt.output(str);

	boost::python::object ret=parse(str.str());

	return ret;
	}

// Fetch objects from the Python side using their Python identifier.

std::shared_ptr<Ex> fetch_from_python(const std::string& nm)
	{
	try {
		boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
		boost::python::object obj = locals[nm];

		if(obj.is_none()) // We never actually get here, an exception will have been thrown.
			std::cout << "object unknown" << std::endl;
		else {
			// We can include this Python object into the expression only if it is an Ex object.
			boost::python::extract<std::shared_ptr<Ex> > extract_Ex(obj);
			if(extract_Ex.check()) {
				std::shared_ptr<Ex> ex = extract_Ex();
				return ex;
				}
			// getting python objects included is tricky because of memory management of the generated Ex.
			//			boost::python::extract<int> extract_int(obj);
			//			if(extract_int.check()) {
			//				int x = extract_int();
			//				std::ostringstream str;
			//				str << "Ex('" << x << "')" << std::ends;
			//				std::cout << "recursive for " << str.str() << std::endl;
			//				return fetch_from_python(str.str());
			//				}
			std::cout << nm << " is not of type cadabra.Ex" << std::endl;
			return 0;
			}
		}
	catch(boost::python::error_already_set const &) {
		// In order to prevent the error from propagating, we have to read
		// it out. And in any case, we want to give some feedback to the user.
		std::string err = parse_python_exception();
		if(err.substr(0,29)=="<type 'exceptions.TypeError'>")
			std::cout << nm << " is not of type cadabra.Ex." << std::endl;
		else 
			std::cout << nm << " is not defined." << std::endl;
		}

	return 0;
	}


void pull_in(std::shared_ptr<Ex> ex)
	{
	collect_terms rr(*get_kernel_from_scope(), *ex);
	
	Ex::iterator it=ex->begin();
	while(it!=ex->end()) {
		if(*it->name=="@") {
			if(ex->number_of_children(it)==1) {
				std::string pobj = *(ex->begin(it)->name);
				std::shared_ptr<Ex> ex = fetch_from_python(pobj);
				if(ex) {
					// The top node is an \expression, so we need the first child of that.
					// FIMXE: assert consistency.
					Ex::iterator expression_it = ex->begin();
					Ex::iterator topnode_it    = ex->begin(expression_it);

					multiplier_t mult=*(it->multiplier);
					it=ex->replace(it, topnode_it);
					multiply(it->multiplier, mult);
					rr.rename_replacement_dummies(it, false);
					}
				}
			}
		++it;
		}

	return;
	}


bool __eq__Ex_Ex(const Ex& one, const Ex& other) 
	{
	return tree_equal(&(get_kernel_from_scope()->properties), one, other);
	}

bool __eq__Ex_int(const Ex& one, int other) 
	{
	Ex ex(other);
	return __eq__Ex_Ex(one, ex);
	}


// Functions to construct an Ex object and then create an additional
// reference '_' on the global python stack that points to this object
// as well. We disable automatic constructor generation in the declaration
// of Ex on the Python side, so all creation of Ex objects in Python goes
// through these two functions.

std::shared_ptr<Ex> make_Ex_from_string(const std::string& ex_, bool make_ref=true) 
	{
	auto ptr = std::make_shared<Ex>();

	// Parse the string expression.

	Parser parser(ptr);
	std::stringstream str(ex_);

	try {
		str >> parser;
//		Ex::print_recursive_treeform(std::cout, ptr->begin());
		}
	catch(std::exception& except) {
		throw ParseException("Cannot parse");
		}

	// First pull in any expressions referred to with @(...) notation, because the
	// full expression may not have consistent indices otherwise.
	pull_in(ptr);
   
	// Basic cleanup of rationals and subtractions, followed by
   // cleanup of nested sums and products.
	pre_clean_dispatch_deep(*get_kernel_from_scope(), *ptr);
	cleanup_dispatch_deep(*get_kernel_from_scope(), *ptr);
	call_post_process(*ptr);

	// The local variable stack is not writeable so we cannot insert '_'
	// as a local variable. Instead, we push it onto the global stack.
	
	if(make_ref) {
		boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
		globals["_"]=ptr;
		}
	return ptr;
	}

std::shared_ptr<Ex> construct_Ex_from_string(const std::string& ex_) 
	{
	return make_Ex_from_string(ex_, true);
	}

std::shared_ptr<Ex> construct_Ex_from_string_2(const std::string& ex_, bool add_ref) 
	{
	return make_Ex_from_string(ex_, add_ref);
	}

std::shared_ptr<Ex> make_Ex_from_int(int num, bool make_ref=true)
	{
	auto ptr = std::make_shared<Ex>(num);
	if(make_ref) {
		boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
		globals["_"]=ptr;
		}
	return ptr;
	}

std::shared_ptr<Ex> construct_Ex_from_int(int num)
	{
	return make_Ex_from_int(num, true);
	}

std::shared_ptr<Ex> construct_Ex_from_int_2(int num, bool add_ref)
	{
	return make_Ex_from_int(num, add_ref);
	}


// Initialise mathematics typesetting for IPython.

std::string init_ipython()
	{
	boost::python::object obj = boost::python::exec("from IPython.display import Math");
	return "Cadabra typeset output for IPython notebook initialised.";
	}

// Generate a Python list of all properties declared in the current scope. These will
// (FIXME: should be) displayed in input form, i.e. they can be fed back into Python.
// FIXME: most of this has nothing to do with Python, factor back into core.

boost::python::list list_properties()
	{
//	std::cout << "listing properties" << std::endl;
	Kernel *kernel=get_kernel_from_scope();
	Properties& props=kernel->properties;

	boost::python::list ret;
	std::string res;
	bool multi=false;
	for(auto it=props.pats.begin(); it!=props.pats.end(); ++it) {
		// print the property name if we are at the end or if the next entry is for
		// a different property.
		decltype(it) nxt=it;
		++nxt;
		if(res=="" && (nxt!=props.pats.end() && it->first==nxt->first)) {
			res+="{";
			multi=true;
			}


		DisplayTeX dt(*get_kernel_from_scope(), it->second->obj);
		std::ostringstream str;
		// std::cerr << "displaying" << std::endl;
		dt.output(str);
		// std::cerr << "displayed " << str.str() << std::endl;
		res += str.str();

		if(nxt==props.pats.end() || it->first!=nxt->first) {
			if(multi)
				res+="}";
			multi=false;
			res += "::";
			res +=(*it).first->name();
			ret.append(res);
			res="";
			}
		else {
			res+=", ";
			}
		}

	return ret;
	}

// Debug function to display an expression in tree form.

std::string print_tree(Ex *ex)
	{
	std::ostringstream str;
	ex->print_entire_tree(str);
	return str.str();
	}

// Setup logic to pass C++ exceptions down to Python properly.
 
PyObject *ParseExceptionType = NULL;
PyObject *ArgumentExceptionType = NULL;
PyObject *NonScalarExceptionType = NULL;
PyObject *InternalErrorType = NULL;

void translate_ParseException(const ParseException &e)
	{
	assert(ParseExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ParseExceptionType, pythonExceptionInstance.ptr());
	}

void translate_ArgumentException(const ArgumentException &e)
	{
	assert(ArgumentExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ArgumentExceptionType, pythonExceptionInstance.ptr());
	}

void translate_NonScalarException(const NonScalarException &e)
	{
	assert(NonScalarExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(NonScalarExceptionType, pythonExceptionInstance.ptr());
	}

void translate_InternalError(const InternalError &e)
	{
	assert(InternalErrorType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(InternalErrorType, pythonExceptionInstance.ptr());
	}

// Return the kernel (with symbol __cdbkernel__) in local scope if
// any, or the one in global scope if none is available in local
// scope.

Kernel *get_kernel_from_scope()
	{
	// Lookup the kernel in the local/global scope. 
	boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
	Kernel *local_kernel=0;
	try {
		boost::python::object obj = locals["__cdbkernel__"];
		local_kernel = boost::python::extract<Kernel *>(obj);
		}
	catch(boost::python::error_already_set& err) {
		std::string err2 = parse_python_exception();
		local_kernel=0;
		}
	if(local_kernel!=0)  {
		return local_kernel;
		}

	// If there is no kernel in local scope, find one in global scope.

 	boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
	Kernel *global_kernel=0;
	try {
		boost::python::object obj = globals["__cdbkernel__"];
		global_kernel = boost::python::extract<Kernel *>(obj);
		}
	catch(boost::python::error_already_set& err) {
		std::string err2 = parse_python_exception();
		global_kernel=0;
		}
	
	if(global_kernel!=0) {
		return global_kernel;
		}

	// If there is no kernel in global scope either, construct one.

	global_kernel = new Kernel();

	// Store this as a Python object, making sure (using boost::ref) that the
	// kernel Python refers to by __cdbkernel__ is the same object as the one
	// we will return to our caller.
	globals["__cdbkernel__"]=boost::ref(global_kernel);
	inject_defaults(global_kernel);

	return global_kernel;
	}

// The following handle setup of local scope for Cadabra properties.

Kernel *create_scope()
	{
	Kernel *k=create_empty_scope();
	inject_defaults(k);
	return k;
	}

Kernel *create_scope_from_global()
	{
	Kernel *k=create_empty_scope();
	// FIXME: copy global properties
	return k;
	}

Kernel *create_empty_scope()
	{
	Kernel *k = new Kernel();
	return k;
	}

void inject_defaults(Kernel *k)
	{
	// Create and inject properties; these then get owned by the kernel.
	post_process_enabled=false;

	inject_property(k, new Distributable(),      make_Ex_from_string("\\prod{#}"), 0);
	inject_property(k, new IndexInherit(),       make_Ex_from_string("\\prod{#}"), 0);
	inject_property(k, new CommutingAsProduct(), make_Ex_from_string("\\prod{#}"), 0);
	inject_property(k, new DependsInherit(),     make_Ex_from_string("\\prod{#}"), 0);
	inject_property(k, new NumericalFlat(),      make_Ex_from_string("\\prod{#}"), 0);

	inject_property(k, new IndexInherit(),       make_Ex_from_string("\\sum{#}"), 0);
	inject_property(k, new CommutingAsSum(),     make_Ex_from_string("\\sum{#}"), 0);

	inject_property(k, new Derivative(),         make_Ex_from_string("\\commutator{#}"), 0);
	inject_property(k, new IndexInherit(),       make_Ex_from_string("\\commutator{#}"), 0);

	inject_property(k, new Derivative(),         make_Ex_from_string("\\anticommutator{#}"), 0);
	inject_property(k, new IndexInherit(),       make_Ex_from_string("\\anticommutator{#}"), 0);

	inject_property(k, new Distributable(),      make_Ex_from_string("\\indexbracket{#}"), 0);
	inject_property(k, new IndexInherit(),       make_Ex_from_string("\\indexbracket{#}"), 0);

	inject_property(k, new DependsInherit(),     make_Ex_from_string("\\pow{#}"), 0);

	// Accents, necessary for proper display.
	inject_property(k, new Accent(),             make_Ex_from_string("\\hat{#}"), 0);
	inject_property(k, new Accent(),             make_Ex_from_string("\\bar{#}"), 0);
	inject_property(k, new Accent(),             make_Ex_from_string("\\overline{#}"), 0);
	inject_property(k, new Accent(),             make_Ex_from_string("\\tilde{#}"), 0);

	post_process_enabled=true;
//	inject_property(k, new Integral(),           make_Ex_from_string("\\int{#}"), 0);
	}

void inject_property(Kernel *kernel, property *prop, std::shared_ptr<Ex> ex, std::shared_ptr<Ex> param)
	{
	Ex::iterator it=ex->begin();
	assert(*(it->name)=="\\expression");
	it=ex->begin(it);

	if(param) {
		keyval_t keyvals;
		prop->parse_to_keyvals(*param, keyvals);
		prop->parse(*kernel, keyvals);
		}
	prop->validate(*kernel, Ex(it));
	kernel->properties.master_insert(Ex(it), prop);
	}

// Property constructor and display members for Python purposes.

template<class Prop>
Property<Prop>::Property(std::shared_ptr<Ex> ex, std::shared_ptr<Ex> param) 
	{
	for_obj = ex;
	Kernel *kernel=get_kernel_from_scope();
	prop = new Prop(); // we keep a pointer, but the kernel owns it.
	inject_property(kernel, prop, ex, param);
	}

template<class Prop>
std::string Property<Prop>::str_() const
	{
	std::ostringstream str;
	str << "Attached property ";
	prop->latex(str); // FIXME: this should call 'str' on the property, which does not exist yet
	str << " to "+Ex_str_(*for_obj)+".";
	return str.str();
	}

template<class Prop>
std::string Property<Prop>::latex_() const
	{
	std::ostringstream str;
	str << "\\text{Attached property ";
	prop->latex(str);
	std::string bare=Ex_latex_(*for_obj);
	str << " to~}"+bare+".";
	return str.str();
	}

template<>
std::string Property<LaTeXForm>::latex_() const
	{
	std::ostringstream str;
	str << "\\text{Attached property ";
	prop->latex(str);
	std::string bare=Ex_str_(*for_obj);
	boost::replace_all(bare, "\\", "$\\backslash{}$}");
	str << " to {\\tt "+bare+"}.";
	return str.str();
	}

template<class Prop>
std::string Property<Prop>::repr_() const
	{
	// FIXME: this needs work, it does not output things which can be fed back into python.
	return "Property::repr: "+prop->name();
	}

// Templates to dispatch function calls in Python to algorithms in
// C++.  All algorithms take the 'deep' and 'repeat' boolean
// arguments, the 'depth' integer argument, plus an arbitrary list of
// additional arguments (of any type) indicated by 'args' (see the
// declaration of 'join_gamma' below for an example of how to declare
// those additional arguments).

template<class F>
Ex* dispatch_base(Ex& ex, F& algo, bool deep, bool repeat, unsigned int depth)
	{
	Ex::iterator it=ex.begin().begin();
	if(ex.is_valid(it)) { // This may be called on an empty expression; just safeguard against that.
		if(*it->name=="\\equals") 
			it=ex.child(it,1);
		ex.reset_state();
		ex.update_state(algo.apply_generic(it, deep, repeat, depth));
		call_post_process(ex);
		}
	return &ex;
	}

void call_post_process(Ex& ex) 
	{
	// Find the 'post_process' function, and if found, turn off
	// post-processing, then call the function on the current Ex.
	if(post_process_enabled) {
		if(ex.number_of_children(ex.begin())==0)
			return;

		post_process_enabled=false;
		boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
		try {
			boost::python::object post_process = globals["post_process"];
			post_process(boost::ref(ex));
			}
		catch(boost::python::error_already_set const &exc) {
			// In order to prevent the error from propagating, we have to read it out. 
			std::string err = parse_python_exception();
//			throw;
			}
		post_process_enabled=true;
		}
	}

template<class F>
Ex* dispatch_ex(Ex& ex, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), ex);
	return dispatch_base(ex, algo, deep, repeat, depth);
	}

template<class F, typename Arg1>
Ex* dispatch_ex(Ex& ex, Arg1 arg, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), ex, arg);
	return dispatch_base(ex, algo, deep, repeat, depth);
	}

template<class F, typename Arg1, typename Arg2>
Ex* dispatch_ex(Ex& ex, Arg1 arg1, Arg2 arg2, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), ex, arg1, arg2);
	return dispatch_base(ex, algo, deep, repeat, depth);
	}

template<class F, typename... Args>
Ex* dispatch_string(const std::string& ex, Args... args, bool deep, bool repeat, unsigned int depth)
	{
	auto exobj = make_Ex_from_string(ex, false);
	return dispatch_ex<F, Args...>(*exobj, args..., deep, repeat, depth);
	}


// template<class F, typename Arg1>
// Ex* dispatch_2_ex_string_new(Ex& ex, const std::string& args, Arg1 arg1, bool deep, bool repeat, unsigned int depth)
// 	{
// 	auto argsobj = make_Ex_from_string(args, false);
// 	return dispatch_2_ex_ex_new<F, Arg1>(ex, *argsobj, arg1, deep, repeat, depth);
// 	}
// 
// template<class F>
// Ex* dispatch_2_ex_ex(Ex& ex, Ex& args, bool deep, bool repeat, unsigned int depth)
// 	{
// 	F algo(*get_kernel_from_scope(), ex, args);
// 
// 	Ex::iterator it=ex.begin().begin();
// 
// 	ex.reset_state();
// 	ex.update_state(algo.apply_generic(it, deep, repeat, depth));
// 	
// 	return &ex;
// 	}

// template<class F>
// Ex* dispatch_2_ex_string(Ex& ex, const std::string& args, bool deep, bool repeat, unsigned int depth)
// 	{
// 	auto argsobj = make_Ex_from_string(args, false);
// 	return dispatch_2_ex_ex<F>(ex, *argsobj, deep, repeat, depth);
// 	}
// 
// template<class F>
// Ex* dispatch_2_string_string(const std::string& ex, const std::string& args, bool deep, bool repeat, unsigned int depth)
// 	{
// 	auto exobj   = make_Ex_from_string(ex);
// 	auto argsobj = make_Ex_from_string(args, false);
// 	return dispatch_2_ex_ex<F>(*exobj, *argsobj, deep, repeat, depth);
// 	}

// Templated function which declares various forms of the algorithm entry points in one shot.
// First the ones with no argument, just a deep flag.

template<class F>
void def_algo_1(const std::string& name) 
	{
	using namespace boost::python;

	def(name.c_str(),  &dispatch_ex<F>,        (arg("ex"),arg("deep")=true,arg("repeat")=false,arg("depth")=0), 
		 return_internal_reference<1>() );
//	def(name.c_str(),  &dispatch_string<F>, (arg("ex"),arg("deep")=true,arg("repeat")=false,arg("depth")=0), 
//		 return_value_policy<manage_new_object>() );
	}

// // Then the ones which take an additional Ex argument (e.g. substitute).
// 
// template<class F>
// void def_algo_2(const std::string& name) 
// 	{
// 	using namespace boost::python;
// 
// 	def(name.c_str(),  &dispatch_2_ex_ex<F>,     (arg("ex"),arg("args"),arg("deep")=true,arg("repeat")=false,arg("depth")=0), 
// 		 return_internal_reference<1>() );
// 	
// 	// The algorithm returns a pointer to the 'ex' argument, which for the 'ex_string' version of the
// 	// algorithm is something that was already present on the python side. Hence return_internal_reference,
// 	// not manage_new_object.
// 
// 	def(name.c_str(),  &dispatch_2_ex_string<F>, (arg("ex"),arg("args"),arg("deep")=true,arg("repeat")=false,arg("depth")=0), 
// 		 return_internal_reference<1>() );
// 
// 	// The following does lead to a new object being created from the string, and this new object needs
// 	// to be managed.
// 
// 	def(name.c_str(),  &dispatch_2_string_string<F>, (arg("ex"),arg("args"),arg("deep")=true,arg("repeat")=false,arg("depth")=0), 
// 		 return_value_policy<manage_new_object>() );
// 	}

// Declare a property. These take one Ex to which they will be attached, and
// one optional additional Ex which is a list of parameters. The latter are thus always
// Cadabra expressions, and cannot easily contain Python constructions (for the time
// being this follows most closely the setup we had in Cadabra v1; if the need arises
// we can make this more complicated later). 

template<class P>
void def_prop()
	{
	using namespace boost::python;
	P *p = new P;

	class_<Property<P>, bases<BaseProperty> > pr(p->name().c_str(), init<std::shared_ptr<Ex>, 
																optional<std::shared_ptr<Ex> > >());

	delete p;

	pr.def("__str__", &Property<P>::str_).def("__repr__", &Property<P>::repr_).def("_latex", &Property<P>::latex_);
	}


// Converter from Python iterables to std::vector and friends.
// http://stackoverflow.com/questions/15842126/

class iterable_converter {
	public:
		template <typename Container>
		iterable_converter&
		from_python() 
			{
			boost::python::converter::registry::push_back(
				&iterable_converter::convertible,
				&iterable_converter::construct<Container>,
				boost::python::type_id<Container>());

			return *this;
			}

		static void* convertible(PyObject* object)
			{
			return PyObject_GetIter(object) ? object : NULL;
			}

		template <typename Container>
		static void construct(PyObject* object, boost::python::converter::rvalue_from_python_stage1_data* data)
			{
			namespace python = boost::python;
			// Object is a borrowed reference, so create a handle indicting it is
			// borrowed for proper reference counting.
			python::handle<> handle(python::borrowed(object));
			
			// Obtain a handle to the memory block that the converter has allocated
			// for the C++ type.
			typedef python::converter::rvalue_from_python_storage<Container> storage_type;
			void* storage = reinterpret_cast<storage_type*>(data)->storage.bytes;
			
			typedef python::stl_input_iterator<typename Container::value_type> iterator;
			
			// Allocate the C++ type into the converter's memory block, and assign
			// its handle to the converter's convertible variable.  The C++
			// container is populated by passing the begin and end iterators of
			// the python object to the container's constructor.
			new (storage) Container(
				iterator(python::object(handle)), // begin
				iterator());                      // end
			data->convertible = storage;
			}
};

// Entry point for registration of the Cadabra Python module. 
// This registers the main Ex class which wraps Cadabra expressions, as well
// as the various algorithms that can act on these and the properties that can
// be attached to Cadabra patterns.
// http://stackoverflow.com/questions/6050996/boost-python-overloaded-functions-with-default-arguments-problem

BOOST_PYTHON_MODULE(cadabra2)
	{
	using namespace boost::python;

	class_<ParseException> pyParseException("ParseException", init<std::string>());
	pyParseException.def("__str__", &ParseException::what);
	ParseExceptionType=pyParseException.ptr();

	class_<ArgumentException> pyArgumentException("ArgumentException", init<std::string>());
	pyArgumentException.def("__str__", &ArgumentException::py_what);
	ArgumentExceptionType=pyArgumentException.ptr();

	class_<NonScalarException> pyNonScalarException("NonScalarException", init<std::string>());
	pyNonScalarException.def("__str__", &NonScalarException::py_what);
	NonScalarExceptionType=pyNonScalarException.ptr();

	class_<InternalError> pyInternalError("InternalError", init<std::string>());
	pyInternalError.def("__str__", &InternalError::py_what);
	InternalErrorType=pyInternalError.ptr();

	// Declare the Kernel object for Python so we can store it in the local Python context.
	class_<Kernel> pyKernel("Kernel", init<>());

	// Declare the Ex object to store expressions and manipulate on the Python side.
	// We do not allow initialisation/construction except through the two 
	// make_Ex_from_... functions, which take care of creating a '_' reference
   // on the Python side as well.

//	class_<Ex, std::shared_ptr<Ex> > pyEx("Ex", boost::python::no_init);
	class_<Ex, std::shared_ptr<Ex> >("Ex", boost::python::no_init)
		.def("__init__", boost::python::make_constructor(&construct_Ex_from_string))
	   .def("__init__", boost::python::make_constructor(&construct_Ex_from_string_2))
		.def("__init__", boost::python::make_constructor(&construct_Ex_from_int))
		.def("__init__", boost::python::make_constructor(&construct_Ex_from_int_2))
		.def("__str__",  &Ex_str_)
		.def("_latex",   &Ex_latex_)
		.def("__repr__", &Ex_repr_)
		.def("__eq__",   &__eq__Ex_Ex)
		.def("__eq__",   &__eq__Ex_int)
		.def("__sympy__",  &Ex_to_Sympy)
		.def("state",    &Ex::state);
	
	enum_<Algorithm::result_t>("result_t")
		.value("changed", Algorithm::result_t::l_applied)
		.value("unchanged", Algorithm::result_t::l_no_action)
		.value("error", Algorithm::result_t::l_error)
		.export_values()
		;

	// Inspection algorithms and other global functions which do not fit into the C++
   // framework anymore.

	def("tree", &print_tree);
	def("init_ipython", &init_ipython);
	def("properties", &list_properties);

	def("create_scope", &create_scope, 
		 return_value_policy<manage_new_object>() );
	def("create_scope_from_global", &create_scope_from_global, 
		 return_value_policy<manage_new_object>());
	def("create_empty_scope", &create_empty_scope, 
		 return_value_policy<manage_new_object>());

	// We do not use implicitly_convertible to convert a string
	// parameter to an Ex object automatically (it never
	// worked). However, we wouldn't want to do this either, because we
	// now use a clear construction: the cadabra python modifications
	// interpret $...$ as a mathematical expression and turn it into an
	// Ex declaration.

	// Algorithms with only the Ex as argument.
	def_algo_1<canonicalise>("canonicalise");
	def_algo_1<collect_factors>("collect_factors");
	def_algo_1<collect_terms>("collect_terms");
	def_algo_1<decompose_product>("decompose_product");
	def_algo_1<distribute>("distribute");
	def_algo_1<eliminate_kronecker>("eliminate_kronecker");
	def_algo_1<epsilon_to_delta>("epsilon_to_delta");
	def_algo_1<expand_delta>("expand_delta");
	def_algo_1<expand_diracbar>("expand_diracbar");
	def_algo_1<flatten_sum>("flatten_sum");
	def_algo_1<indexsort>("indexsort");
	def_algo_1<product_rule>("product_rule");
	def_algo_1<rename_dummies>("rename_dummies");
//	def_algo_1<reduce_sub>("reduce_sub");
	def_algo_1<sort_product>("sort_product");
	def_algo_1<unwrap>("unwrap");
	def_algo_1<young_project_product>("young_project_product");

	def("complete", &dispatch_ex<complete, Ex&>, 
		 (arg("ex"),arg("add"),
		  arg("deep")=false,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("young_project_tensor", &dispatch_ex<young_project_tensor, bool>, 
		 (arg("ex"),arg("modulo_monoterm")=false,
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("join_gamma",  &dispatch_ex<join_gamma, bool, bool>, 
		 (arg("ex"),arg("expand")=true,arg("use_gendelta")=false,
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	// Automatically convert Python sets and so on of integers to std::vector.
	iterable_converter().from_python<std::vector<int> >();

	def("evaluate", &dispatch_ex<evaluate, Ex&>,
		 (arg("ex"), arg("components"),
		  arg("deep")=false,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
		  
	def("keep_terms", &dispatch_ex<keep_terms, std::vector<int> >,
		 (arg("ex"), 
		  arg("terms"),
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("young_project", &dispatch_ex<young_project, std::vector<int>, std::vector<int> >, 
		 (arg("ex"),
		  arg("shape"), arg("indices"), 
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("order", &dispatch_ex<order, Ex&, bool>, 
		 (arg("ex"),
		  arg("factors"), arg("anticommuting")=false,
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("order", &dispatch_ex<order, Ex&, bool>, 
		 (arg("ex"),
		  arg("factors"), arg("anticommuting")=false,
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("sym", &dispatch_ex<sym, Ex&, bool>, 
		 (arg("ex"),
		  arg("items"), arg("anticommuting")=false,
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("asym", &dispatch_ex<sym, Ex&, bool>, 
		 (arg("ex"),
		  arg("items"), arg("anticommuting")=true,
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	// Algorithms which take a second Ex as argument.
	def_algo_1<lr_tensor>("lr_tensor");

	def("factor_in", &dispatch_ex<factor_in, Ex&>, 
		 (arg("ex"),
		  arg("factors"),
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	def("substitute", &dispatch_ex<substitute, Ex&>, 
		 (arg("ex"),
		  arg("rules"),
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	def("take_match", &dispatch_ex<take_match, Ex&>, 
		 (arg("ex"),
		  arg("rules"),
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	def("replace_match", &dispatch_ex<replace_match>, 
		 (arg("ex"),
		  arg("deep")=false,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	def("vary", &dispatch_ex<vary, Ex&>, 
		 (arg("ex"),
		  arg("rules"),
		  arg("deep")=false,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	def("split_index", &dispatch_ex<split_index, Ex&>, 
		 (arg("ex"),
		  arg("rules"),
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );


	// Properties are declared as objects on the Python side as well. They all take two
	// Ex objects as constructor parameters: the first one is the object(s) to which the
	// property is attached, the second one is the argument list (represented as an Ex).
	// 
	// It might have been more logical to make property declarations through a function,
	// so that we have Indices(ex=Ex('{a,b,c,}'), name='vector') being a function that takes
   // one expression and a number of arguments. From the Python point of view this would be
   // added to a map in the cadabra2.Kernel object. However, keeping control over an object 
	// also has the advantage that we can refer to it again if necessary from the Python side,
	// and keeps C++ and Python more in sync.

	class_<BaseProperty>("Property", no_init);

	def_prop<Accent>();
	def_prop<AntiCommuting>();
	def_prop<AntiSymmetric>();
	def_prop<Coordinate>();
	def_prop<Commuting>();
	def_prop<CommutingAsProduct>();
	def_prop<CommutingAsSum>();
	def_prop<DAntiSymmetric>();
	def_prop<Depends>();
	def_prop<Derivative>();
	def_prop<Diagonal>();
	def_prop<Distributable>();
	def_prop<DiracBar>();
	def_prop<EpsilonTensor>();
	def_prop<FilledTableau>();
	def_prop<GammaMatrix>();
	def_prop<ImplicitIndex>();	
	def_prop<IndexInherit>();
	def_prop<Indices>();	
	def_prop<Integer>();
	def_prop<InverseMetric>();
	def_prop<KroneckerDelta>();
	def_prop<LaTeXForm>();
	def_prop<Metric>();
	def_prop<NonCommuting>();
	def_prop<NumericalFlat>();
	def_prop<PartialDerivative>();
	def_prop<RiemannTensor>();
	def_prop<SatisfiesBianchi>();
	def_prop<SelfAntiCommuting>();
	def_prop<SelfCommuting>();
	def_prop<SelfNonCommuting>();
	def_prop<SortOrder>();
	def_prop<Spinor>();
	def_prop<Symmetric>();
	def_prop<Tableau>();
	def_prop<TableauSymmetry>();
	def_prop<Traceless>();
	def_prop<Weight>();
	def_prop<WeightInherit>();
	def_prop<WeylTensor>();

	register_exception_translator<ParseException>(&translate_ParseException);
	register_exception_translator<ArgumentException>(&translate_ArgumentException);
	register_exception_translator<NonScalarException>(&translate_NonScalarException);
	register_exception_translator<InternalError>(&translate_InternalError);

	// How can we give Python access to information stored in properties?
	}
