


#include <memory>

// make boost::python understand std::shared_ptr when compiled with clang.
// http://stackoverflow.com/questions/13986581/using-boost-python-stdshared-ptr

// This now works on both Linux and OS X El Capitan, but your mileage may vary. 
//
#if (defined(__clang__) && defined(__linux__)) 
namespace boost {
   template<typename T>
   T *get_pointer(std::shared_ptr<T> p)
		{
		return p.get();
		}
   }
#endif

#include "PythonCdb.hh"
#include "SympyCdb.hh"

#include "Parser.hh"
#include "Bridge.hh"
#include "Exceptions.hh"
#include "DisplayTeX.hh"
#include "DisplaySympy.hh"
#include "DisplayTerminal.hh"
#include "Cleanup.hh"
#include "PreClean.hh"
#include "PythonException.hh"
#include "ProgressMonitor.hh"
//#include "ServerWrapper.hh"

#include <boost/python/implicit.hpp>
#include <boost/parameter/preprocessor.hpp>
//#include <boost/parameter/python.hpp>
//#include <boost/python.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/def.hpp>
#include <boost/python/module.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/slice.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <sstream>

// Properties.

#include "properties/Accent.hh"
#include "properties/AntiCommuting.hh"
#include "properties/AntiSymmetric.hh"
#include "properties/Commuting.hh"
#include "properties/Coordinate.hh"
#include "properties/Depends.hh"
#include "properties/DependsInherit.hh"
#include "properties/Derivative.hh"
#include "properties/DifferentialForm.hh"
#include "properties/DiracBar.hh"
#include "properties/GammaMatrix.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/DAntiSymmetric.hh"
#include "properties/Diagonal.hh"
#include "properties/Distributable.hh"
#include "properties/EpsilonTensor.hh"
#include "properties/ExteriorDerivative.hh"
#include "properties/FilledTableau.hh"
#include "properties/ImaginaryI.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/Indices.hh"
#include "properties/IndexInherit.hh"
#include "properties/Integer.hh"
#include "properties/InverseMetric.hh"
#include "properties/KroneckerDelta.hh"
#include "properties/LaTeXForm.hh"
#include "properties/Matrix.hh"
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
#include "properties/Symbol.hh"
#include "properties/Symmetric.hh"
#include "properties/Tableau.hh"
#include "properties/TableauSymmetry.hh"
#include "properties/Traceless.hh"
#include "properties/Weight.hh"
#include "properties/WeightInherit.hh"
#include "properties/WeylTensor.hh"

// Algorithms.

#include "algorithms/canonicalise.hh"
#include "algorithms/collect_components.hh"
#include "algorithms/collect_factors.hh"
#include "algorithms/collect_terms.hh"
#include "algorithms/combine.hh"
#include "algorithms/complete.hh"
#include "algorithms/decompose_product.hh"
#include "algorithms/distribute.hh"
#include "algorithms/drop_weight.hh"
#include "algorithms/einsteinify.hh"
#include "algorithms/eliminate_kronecker.hh"
#include "algorithms/eliminate_metric.hh"
#include "algorithms/epsilon_to_delta.hh"
#include "algorithms/evaluate.hh"
#include "algorithms/expand.hh"
#include "algorithms/expand_delta.hh"
#include "algorithms/expand_diracbar.hh"
#include "algorithms/expand_power.hh"
#include "algorithms/factor_in.hh"
#include "algorithms/factor_out.hh"
#include "algorithms/fierz.hh"
#include "algorithms/flatten_sum.hh"
#include "algorithms/indexsort.hh"
#include "algorithms/integrate_by_parts.hh"
#include "algorithms/join_gamma.hh"
#include "algorithms/keep_terms.hh"
#include "algorithms/lr_tensor.hh"
#include "algorithms/map_sympy.hh"
#include "algorithms/order.hh"
#include "algorithms/product_rule.hh"
#include "algorithms/reduce_delta.hh"
#include "algorithms/rename_dummies.hh"
#include "algorithms/sort_product.hh"
#include "algorithms/sort_spinors.hh"
#include "algorithms/sort_sum.hh"
#include "algorithms/split_gamma.hh"
#include "algorithms/split_index.hh"
#include "algorithms/substitute.hh"
#include "algorithms/sym.hh"
#include "algorithms/take_match.hh"
#include "algorithms/replace_match.hh"
#include "algorithms/rewrite_indices.hh"
#include "algorithms/unwrap.hh"
#include "algorithms/vary.hh"
#include "algorithms/young_project.hh"
#include "algorithms/young_project_product.hh"
#include "algorithms/young_project_tensor.hh"


using namespace cadabra;

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

// Wrap the 'totals' member of ProgressMonitor to return a Python list.

boost::python::list ProgressMonitor_totals_helper(ProgressMonitor& self)
	{
	boost::python::list list;
	auto totals = self.totals();
	for(auto& total: totals)
		list.append(total);
	return list;
	}


// Split a 'sum' expression into its individual terms. FIXME: now deprecated because we have operator[]?

boost::python::list terms(const Ex& ex) 
	{
	Ex::iterator it=ex.begin();

	if(*it->name!="\\sum")
		throw ArgumentException("terms() expected a sum expression.");

	boost::python::list ret;

	auto sib=ex.begin(it);
	while(sib!=ex.end(it)) {
		ret.append(Ex(sib));
		++sib;
		}
	
	return ret;
	}

Ex lhs(const Ex& ex)
	{
	auto it=ex.begin();
	if(it==ex.end()) 
		throw ArgumentException("Empty expression passed to 'lhs'.");

	if(*it->name!="\\equals")
		throw ArgumentException("Cannot take 'lhs' of expression which is not an equation.");

	return Ex(ex.begin(ex.begin()));
	}

Ex rhs(const Ex& ex)
	{
	auto it=ex.begin();
	if(it==ex.end()) 
		throw ArgumentException("Empty expression passed to 'rhs'.");

	if(*it->name!="\\equals")
		throw ArgumentException("Cannot take 'rhs' of expression which is not an equation.");

	auto sib=ex.begin(ex.begin());
	++sib;
	return Ex(sib);
	}

Ex Ex_getslice(Ex &ex, boost::python::slice slice)
	{
	Ex result;

	boost::python::slice::range<Ex::sibling_iterator> range;
	try {
		range = slice.get_indices(ex.begin(ex.begin()), ex.end(ex.begin()));
		}
	catch (std::invalid_argument) {
		return result;
		}

	// Set head
	auto it = result.set_head(*ex.begin());

	// Iterate over fully-closed range.
	for (; range.start != range.stop; std::advance(range.start, range.step)) {
		Ex::iterator toadd(range.start);
		result.append_child(it, toadd);
		}
	Ex::iterator toadd(range.start);
	result.append_child(it, toadd);
	return result;
	}

Ex Ex_getitem(Ex &ex, int index)
	{
	Ex::iterator it=ex.begin();

	size_t num=ex.number_of_children(it);
	if(index>=0 && (size_t)index<num)
		return Ex(ex.child(it, index));
	else {
//		if(num==0 && index==0) {
//			std::cerr << "returning " << ex << std::endl;
//			return Ex(ex);
//			}
//		else
			throw ArgumentException("index "+std::to_string(index)+" out of range, must be smaller than "+std::to_string(num));
		}
	}

void Ex_setitem(Ex &ex, int index, Ex val)
	{
	Ex::iterator it=ex.begin();

	size_t num=ex.number_of_children(it);
	if(index>=0 && (size_t)index<num) 
		ex.replace(ex.child(it, index), val.begin());
	else 
		throw ArgumentException("index "+std::to_string(index)+" out of range, must be smaller than "+std::to_string(num));
	}

size_t Ex_len(Ex& ex)
	{
	Ex::iterator it=ex.begin();

	return ex.number_of_children(it);
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

	// std::cerr << ex_ << std::endl;
	
	// Parse the string expression.

	Parser parser(ptr);
	std::stringstream str(ex_);

	try {
	  //	  std::cerr << "parsing " << ex_ << std::endl;
		str >> parser;
//		Ex::print_recursive_treeform(std::cout, ptr->begin());
		}
	catch(std::exception& except) {
	  //	  std::cerr << "cannot parse " << ex_ << std::endl;
		throw ParseException("Cannot parse");
		}
	parser.finalise();
	//	std::cerr << "finalised" << std::endl;

	// First pull in any expressions referred to with @(...) notation, because the
	// full expression may not have consistent indices otherwise.
	pull_in(ptr);
	//	std::cerr << "pulled in" << std::endl;
   
	// Basic cleanup of rationals and subtractions, followed by
   // cleanup of nested sums and products.
	Kernel *kernel=get_kernel_from_scope();
	pre_clean_dispatch_deep(*kernel, *ptr);
	cleanup_dispatch_deep(*kernel, *ptr);
	check_index_consistency(*kernel, *ptr, (*ptr).begin());
	call_post_process(*kernel, *ptr);
	//	std::cerr << "cleaned up" << std::endl;

	// The local variable stack is not writeable so we cannot insert '_'
	// as a local variable. Instead, we push it onto the global stack.
	
	if(make_ref) {
	  //	  std::cerr << "making ref" << std::endl;
	  auto globals_c = PyEval_GetGlobals();
	  if(globals_c!=NULL) {
	    // std::cerr << "globals_c non-null" << std::endl;
	    boost::python::object globals(boost::python::borrowed(globals_c));
	    // std::cerr << "inserting ref" << std::endl;
	    globals["_"]=ptr;
	  }
	}
	// std::cerr << "make_ex_from_string done" << std::endl;
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


Ex operator+(const Ex& ex1, const Ex& ex2)
	{
	if(ex1.size()==0) return ex2;
	if(ex2.size()==0) return ex1;

	bool comma1 = (*ex1.begin()->name=="\\comma");
	bool comma2 = (*ex2.begin()->name=="\\comma");

	if(comma1 || comma2) {
		if(comma1) {
			Ex ret(ex1);
			auto loc = ret.append_child(ret.begin(), ex2.begin());
			if(comma2)
				ret.flatten_and_erase(loc);
			return ret;
			}
		else {
			Ex ret(ex2);
			auto loc = ret.prepend_child(ret.begin(), ex1.begin());
			if(comma1)
				ret.flatten_and_erase(loc);
			return ret;
			}
		}
	else {
		Ex ret(ex1);
		if(*ret.begin()->name!="\\sum") 
			ret.wrap(ret.begin(), str_node("\\sum"));
		ret.append_child(ret.begin(), ex2.begin());
		
		auto it=ret.begin();
		cleanup_dispatch(*get_kernel_from_scope(), ret, it);
		return ret;
		}
	}

Ex operator-(const Ex& ex1, const Ex& ex2)
	{
	if(ex1.size()==0) {
		if(ex2.size()!=0) {
			Ex ret(ex2);
			multiply(ex2.begin()->multiplier, -1);
			auto it=ret.begin();
			cleanup_dispatch(*get_kernel_from_scope(), ret, it);
			return ret;
			}
		else return ex2;
		}
	if(ex2.size()==0) return ex1;

	Ex ret(ex1);
	if(*ret.begin()->name!="\\sum") 
		ret.wrap(ret.begin(), str_node("\\sum"));
	multiply( ret.append_child(ret.begin(), ex2.begin())->multiplier, -1 );

	auto it=ret.begin();
	cleanup_dispatch(*get_kernel_from_scope(), ret, it);
	
	return ret;
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
		if(it->first->hidden()) continue;
		
		// print the property name if we are at the end or if the next entry is for
		// a different property.
		decltype(it) nxt=it;
		++nxt;
		if(res=="" && (nxt!=props.pats.end() && it->first==nxt->first)) {
			res+="{";
			multi=true;
			}


		//std::cerr << Ex(it->second->obj) << std::endl;
//		DisplayTeX dt(*get_kernel_from_scope(), it->second->obj);
		std::ostringstream str;
		// std::cerr << "displaying" << std::endl;
//		dt.output(str);

		str << it->second->obj;
		
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

//boost::python::list indices() 
//	{
//	Kernel *kernel=get_kernel_from_scope();
//	}

// Debug function to display an expression in tree form.

std::string print_tree(Ex *ex)
	{
	std::ostringstream str;
	ex->print_entire_tree(str);
	return str.str();
	}

// Setup logic to pass C++ exceptions down to Python properly.
 
PyObject *ParseExceptionType = NULL;
PyObject *ArgumentExceptionType = 0;
PyObject *ConsistencyExceptionType = 0;
PyObject *RuntimeExceptionType = 0;
PyObject *NonScalarExceptionType = NULL;
PyObject *InternalErrorType = NULL;
PyObject *NotYetImplementedType = NULL;

// Create a new Python exception class which derives from Exception (as it should to
// be a good Python exception). Taken from http://stackoverflow.com/questions/11448735.
PyObject* createExceptionClass(const char* name, PyObject* baseTypeObj = PyExc_Exception)
	{
	std::string scope_name = boost::python::extract<std::string>(boost::python::scope().attr("__name__"));
	std::string qualified0 = scope_name + "." + name;
	char*       qualified1 = const_cast<char*>(qualified0.c_str());
	
	PyObject* typeObj = PyErr_NewException(qualified1, baseTypeObj, 0);
	if(!typeObj) boost::python::throw_error_already_set();
	boost::python::scope().attr(name) = boost::python::handle<>(boost::python::borrowed(typeObj));
	return typeObj;
	}

// Handler to translate a C++ exception and pass it on as a Python exception
void translate_ArgumentException(const ArgumentException& x) 
	{
	assert(ArgumentExceptionType != 0);
	boost::python::object exc(x); // wrap the C++ exception
//	boost::python::object exc_t(boost::python::handle<>(boost::python::borrowed(ArgumentExceptionType)));
//	exc_t.attr("cause") = exc; // add the wrapped exception to the Python exception
	
//    PyErr_SetString(ArgumentExceptionType, x.what());
	PyErr_SetObject(ArgumentExceptionType, exc.ptr()); //exc_t.ptr());
	}

void translate_ConsistencyException(const ConsistencyException& x) 
	{
	assert(ConsistencyExceptionType != 0);
	boost::python::object exc(x); // wrap the C++ exception
//	boost::python::object exc_t(boost::python::handle<>(boost::python::borrowed(ArgumentExceptionType)));
//	exc_t.attr("cause") = exc; // add the wrapped exception to the Python exception
	
//    PyErr_SetString(ArgumentExceptionType, x.what());
	PyErr_SetObject(ConsistencyExceptionType, exc.ptr()); //exc_t.ptr());
	}

void translate_ParseException(const ParseException &e)
	{
	assert(ParseExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ParseExceptionType, pythonExceptionInstance.ptr());
	}

void translate_RuntimeException(const RuntimeException &e)
	{
	assert(RuntimeExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(RuntimeExceptionType, pythonExceptionInstance.ptr());
	}

//void translate_ArgumentException(const ArgumentException &e)
//	{
//	assert(ArgumentExceptionType != NULL);
//	boost::python::object pythonExceptionInstance(e);
//	PyErr_SetObject(ArgumentExceptionType, pythonExceptionInstance.ptr());
//	}

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

void translate_NotYetImplemented(const NotYetImplemented &e)
	{
	assert(NotYetImplementedType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(NotYetImplementedType, pythonExceptionInstance.ptr());
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

	k->inject_property(new Distributable(),      make_Ex_from_string("\\prod{#}",false), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\prod{#}",false), 0);
	k->inject_property(new CommutingAsProduct(), make_Ex_from_string("\\prod{#}",false), 0);
	k->inject_property(new DependsInherit(),     make_Ex_from_string("\\prod{#}",false), 0);
	k->inject_property(new NumericalFlat(),      make_Ex_from_string("\\prod{#}",false), 0);
	auto wi2=new WeightInherit();
	wi2->combination_type = WeightInherit::multiplicative;
	auto wa2=make_Ex_from_string("label=all, type=multiplicative", false);
	k->inject_property(wi2,                      make_Ex_from_string("\\prod{#}",false), wa2);

	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\frac{#}",false), 0);
	k->inject_property(new DependsInherit(),     make_Ex_from_string("\\frac{#}",false), 0);
	
	k->inject_property(new Distributable(),      make_Ex_from_string("\\wedge{#}",false), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\wedge{#}",false), 0);
//	k->inject_property(new CommutingAsProduct(), make_Ex_from_string("\\prod{#}",false), 0);
	k->inject_property(new DependsInherit(),     make_Ex_from_string("\\wedge{#}",false), 0);
	k->inject_property(new NumericalFlat(),      make_Ex_from_string("\\wedge{#}",false), 0);
	auto wi4=new WeightInherit();
	wi4->combination_type = WeightInherit::multiplicative;
	auto wa4=make_Ex_from_string("label=all, type=multiplicative", false);
	k->inject_property(wi4,                      make_Ex_from_string("\\wedge{#}",false), wa4);

	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\sum{#}",false), 0);
	k->inject_property(new CommutingAsSum(),     make_Ex_from_string("\\sum{#}",false), 0);
	k->inject_property(new DependsInherit(),     make_Ex_from_string("\\sum{#}",false), 0);
	auto wi=new WeightInherit();
	auto wa=make_Ex_from_string("label=all, type=additive", false);
	k->inject_property(wi,                       make_Ex_from_string("\\sum{#}", false), wa);

	auto d = new Derivative();
	d->hidden(true);
	k->inject_property(d,                        make_Ex_from_string("\\cdbDerivative{#}",false), 0);
	
	k->inject_property(new Derivative(),         make_Ex_from_string("\\commutator{#}",false), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\commutator{#}",false), 0);

	k->inject_property(new Derivative(),         make_Ex_from_string("\\anticommutator{#}",false), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\anticommutator{#}",false), 0);

	k->inject_property(new Distributable(),      make_Ex_from_string("\\indexbracket{#}",false), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\indexbracket{#}",false), 0);

	k->inject_property(new DependsInherit(),     make_Ex_from_string("\\pow{#}",false), 0);
	auto wi3=new WeightInherit();
	auto wa3=make_Ex_from_string("label=all, type=power", false);
	k->inject_property(wi3,                      make_Ex_from_string("\\pow{#}",false), wa3);

	k->inject_property(new NumericalFlat(),      make_Ex_from_string("\\int{#}",false), 0);
	k->inject_property(new IndexInherit(),       make_Ex_from_string("\\int{#}",false), 0);

	// Accents, necessary for proper display.
	k->inject_property(new Accent(),             make_Ex_from_string("\\hat{#}",false), 0);
	k->inject_property(new Accent(),             make_Ex_from_string("\\bar{#}",false), 0);
	k->inject_property(new Accent(),             make_Ex_from_string("\\overline{#}",false), 0);
	k->inject_property(new Accent(),             make_Ex_from_string("\\tilde{#}",false), 0);

	post_process_enabled=true;
//	k->inject_property(new Integral(),           make_Ex_from_string("\\int{#}",false), 0);
	}


// Property constructor and display members for Python purposes.

template<class Prop>
Property<Prop>::Property(std::shared_ptr<Ex> ex, std::shared_ptr<Ex> param) 
	{
	for_obj = ex;
	Kernel *kernel=get_kernel_from_scope();
	prop = new Prop(); // we keep a pointer, but the kernel owns it.
//	std::cerr << "Declaring property " << prop->name() << " in kernel " << kernel << std::endl;
	kernel->inject_property(prop, ex, param);
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

//	HERE: this text should go away, property should just print itself in a python form,
//   the decorating text should be printed in a separate place.

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

ProgressMonitor *pm=0;

template<class F>
Ex* dispatch_base(Ex& ex, F& algo, bool deep, bool repeat, unsigned int depth, bool pre_order)
	{
	Ex::iterator it=ex.begin();
	if(ex.is_valid(it)) { // This may be called on an empty expression; just safeguard against that.
		if(pm==0) {
			try {
				boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
				boost::python::object obj = globals["server"];
				pm = boost::python::extract<ProgressMonitor *>(obj); 
				}
			catch(boost::python::error_already_set& err) {
				std::cerr << "Cannot find ProgressMonitor derived 'server' object." << std::endl;
				}
			}

		algo.set_progress_monitor(pm);
		if(!pre_order)
			ex.update_state(algo.apply_generic(it, deep, repeat, depth));
		else
			ex.update_state(algo.apply_pre_order(repeat));			
		call_post_process(*get_kernel_from_scope(), ex);
		}
	return &ex;
	}

Ex* map_sympy_wrapper(Ex& ex, std::string head)
	{
	map_sympy algo(*get_kernel_from_scope(), ex, head);
	return dispatch_base(ex, algo, true, false, 0, true);
	}

void call_post_process(Kernel& kernel, Ex& ex) 
	{
	// Find the 'post_process' function, and if found, turn off
	// post-processing, then call the function on the current Ex.
	if(post_process_enabled) {
		if(ex.number_of_children(ex.begin())==0)
			return;

		post_process_enabled=false;

		boost::python::object post_process;
		
		try {
			// First try the locals.
			boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
			post_process = locals["post_process"];
			// std::cerr << "local post_process" << std::endl;
			}
		catch(boost::python::error_already_set const &exc) {
			// In order to prevent the error from propagating, we have to read it out. 			
			std::string err = parse_python_exception();
			try {
				boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
				post_process = globals["post_process"];
				// std::cerr << "global post_process" << std::endl;				
				}
			catch(boost::python::error_already_set const &exc) {
				// In order to prevent the error from propagating, we have to read it out. 
				std::string err = parse_python_exception();
				post_process_enabled=true;
				return;
				}
			}
		// std::cerr << "calling post-process" << std::endl;
		post_process(boost::ref(kernel), boost::ref(ex));
		post_process_enabled=true;
		}
	}

template<class F>
Ex* dispatch_ex(Ex& ex, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), ex);
	return dispatch_base(ex, algo, deep, repeat, depth, false);
	}

template<class F, typename Arg1>
Ex* dispatch_ex(Ex& ex, Arg1 arg, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), ex, arg);
	return dispatch_base(ex, algo, deep, repeat, depth, false);
	}

template<class F, typename Arg1, typename Arg2>
Ex* dispatch_ex(Ex& ex, Arg1 arg1, Arg2 arg2, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), ex, arg1, arg2);
	return dispatch_base(ex, algo, deep, repeat, depth, false);
	}

template<class F, typename Arg1, typename Arg2, typename Arg3>
Ex* dispatch_ex(Ex& ex, Arg1 arg1, Arg2 arg2, Arg3 arg3, bool deep, bool repeat, unsigned int depth)
	{
	F algo(*get_kernel_from_scope(), ex, arg1, arg2, arg3);
	return dispatch_base(ex, algo, deep, repeat, depth, false);
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

	pr.def("__str__", &Property<P>::str_).def("__repr__", &Property<P>::repr_).def("_latex_", &Property<P>::latex_);
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

	// Declare the Kernel object for Python so we can store it in the local Python context.
	// We add a 'cadabra2.__cdbkernel__' object to the main module scope, and will 
	// pull that into the interpreter scope in the 'cadabra2_default.py' file.
	class_<Kernel, boost::noncopyable> pyKernel("Kernel", init<>());
	boost::python::object kernel=pyKernel();
	inject_defaults(boost::python::extract<Kernel*>(kernel));
	boost::python::scope().attr("__cdbkernel__")=kernel;

	// Make our profiling class known to the Python world.
	class_<ProgressMonitor>("ProgressMonitor")
		.def("print", &ProgressMonitor::print)
		.def("totals", &ProgressMonitor_totals_helper);
	
	class_<ProgressMonitor::Total>("Total")
		.def_readonly("name", &ProgressMonitor::Total::name)
		.def_readonly("call_count", &ProgressMonitor::Total::call_count)
		.def_readonly("time_spent", &ProgressMonitor::Total::time_spent_as_long)
		.def_readonly("total_steps", &ProgressMonitor::Total::total_steps)
		.def("__str__", &ProgressMonitor::Total::str);

	// Declare the Ex object to store expressions and manipulate on the Python side.
	// We do not allow initialisation/construction except through the two 
	// make_Ex_from_... functions, which take care of creating a '_' reference
   // on the Python side as well.

//	class_<Ex, std::shared_ptr<Ex> > pyEx("Ex", boost::python::no_init);
	class_<Ex, std::shared_ptr<Ex> >("Ex", boost::python::no_init)
		.def("__init__", boost::python::make_constructor(&construct_Ex_from_string))
		.def("__init__", boost::python::make_constructor(&construct_Ex_from_string_2))
		.def("__init__",    boost::python::make_constructor(&construct_Ex_from_int))
		.def("__init__",    boost::python::make_constructor(&construct_Ex_from_int_2))
		.def("__str__",     &Ex_str_)
		.def("_latex_",     &Ex_latex_)
		.def("__repr__",    &Ex_repr_)
		.def("__eq__",      &__eq__Ex_Ex)
		.def("__eq__",      &__eq__Ex_int)
		.def("_sympy_",     &Ex_to_Sympy)
		.def("__getitem__", &Ex_getitem)
		.def("__getitem__", &Ex_getslice)
		.def("__setitem__", &Ex_setitem)
		.def("__len__",     &Ex_len)
		.def("state",       &Ex::state)
		.def("reset",       &Ex::reset_state)
		.def("changed",     &Ex::changed_state)
		.def(self + self)
		.def(self - self);
	
	enum_<Algorithm::result_t>("result_t")
		.value("checkpointed", Algorithm::result_t::l_checkpointed)
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
	def("map_sympy", &map_sympy_wrapper, (arg("ex"), arg("function")=""), return_internal_reference<1>());

	def("create_scope", &create_scope, 
		 return_value_policy<manage_new_object>() );
	def("create_scope_from_global", &create_scope_from_global, 
		 return_value_policy<manage_new_object>());
	def("create_empty_scope", &create_empty_scope,
		 return_value_policy<manage_new_object>());

	// Algorithms which spit out a new Ex (or a list of new Exs), instead of 
	// modifying the existing one. 

	def("terms", &terms);

	def("lhs", &lhs);
	def("rhs", &rhs);

	// We do not use implicitly_convertible to convert a string
	// parameter to an Ex object automatically (it never
	// worked). However, we wouldn't want to do this either, because we
	// now use a clear construction: the cadabra python modifications
	// interpret $...$ as a mathematical expression and turn it into an
	// Ex declaration.

	// Algorithms with only the Ex as argument.
	def_algo_1<canonicalise>("canonicalise");
	def_algo_1<collect_components>("collect_components");
	def_algo_1<collect_factors>("collect_factors");
 	def_algo_1<collect_terms>("collect_terms");
 	def_algo_1<combine>("combine");
	def_algo_1<decompose_product>("decompose_product");
	def_algo_1<distribute>("distribute");
	def_algo_1<eliminate_kronecker>("eliminate_kronecker");
 	def_algo_1<expand>("expand");
	def_algo_1<expand_delta>("expand_delta");
	def_algo_1<expand_diracbar>("expand_diracbar");
	def_algo_1<expand_power>("expand_power");
	def_algo_1<flatten_sum>("flatten_sum");
	def_algo_1<indexsort>("indexsort");
	def_algo_1<product_rule>("product_rule");
	def_algo_1<reduce_delta>("reduce_delta");
//	def_algo_1<reduce_sub>("reduce_sub");
	def_algo_1<sort_product>("sort_product");
	def_algo_1<sort_spinors>("sort_spinors");
	def_algo_1<sort_sum>("sort_sum");
	def_algo_1<young_project_product>("young_project_product");

	def("complete", &dispatch_ex<complete, Ex&>, 
		 (arg("ex"),arg("add"),
		  arg("deep")=false,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("drop_weight", &dispatch_ex<drop_weight, Ex&>, 
		 (arg("ex"),arg("condition"),
		  arg("deep")=false,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("eliminate_metric", &dispatch_ex<eliminate_metric, Ex&>, 
	    (arg("ex"),arg("preferred")=new Ex(), 
	     arg("deep")=true,arg("repeat")=false,arg("depth")=0),
	    return_internal_reference<1>() );

	def("keep_weight", &dispatch_ex<keep_weight, Ex&>, 
		 (arg("ex"),arg("condition"),
		  arg("deep")=false,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("integrate_by_parts", &dispatch_ex<integrate_by_parts, Ex&>, 
		 (arg("ex"),arg("away_from"),
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

	// Automatically convert from Python sets and so on of integers to std::vector.
	iterable_converter().from_python<std::vector<int> >();

	// Automatically convert from C++ vectors to Python lists.
//	boost::python::class_<std::vector<ProgressMonitor::Total>>("TotalVector")
	
//	.def(boost::python::vector_indexing_suite<std::vector<ProgressMonitor::Total>>());

	
	def("einsteinify", &dispatch_ex<einsteinify, Ex&>,
	    (arg("ex"), arg("metric")=new Ex(),
		  arg("deep")=false,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
		  
	def("evaluate", &dispatch_ex<evaluate, Ex&, bool, bool>,
		 (arg("ex"), arg("components")=new Ex(), arg("rhsonly")=false, arg("simplify")=true,
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

	def("epsilon_to_delta", &dispatch_ex<epsilon_to_delta, bool>,
		 (arg("ex"),
		  arg("reduce")=true,
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("rename_dummies", &dispatch_ex<rename_dummies, std::string, std::string>, 
		 (arg("ex"),
		  arg("set")="", arg("to")="",
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("sym", &dispatch_ex<sym, Ex&, bool>, 
		 (arg("ex"),
		  arg("items"), arg("antisymmetric")=false,
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	def("asym", &dispatch_ex<sym, Ex&, bool>, 
		 (arg("ex"),
		  arg("items"), arg("antisymmetric")=true,
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );

	// Algorithms which take a second Ex as argument.
	def_algo_1<lr_tensor>("lr_tensor");

	def("factor_in", &dispatch_ex<factor_in, Ex&>, 
		 (arg("ex"),
		  arg("factors"),
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	def("factor_out", &dispatch_ex<factor_out, Ex&, bool>, 
		 (arg("ex"),
		  arg("factors"),arg("right")=false,
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	def("fierz", &dispatch_ex<fierz, Ex&>, 
		 (arg("ex"),
		  arg("spinors"),
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
	def("rewrite_indices", &dispatch_ex<rewrite_indices, Ex&, Ex&>, 
		 (arg("ex"),arg("preferred"),arg("converters"),
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	def("vary", &dispatch_ex<vary, Ex&>, 
		 (arg("ex"),
		  arg("rules"),
		  arg("deep")=false,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	def("split_gamma", &dispatch_ex<split_gamma, bool>, 
		 (arg("ex"),
		  arg("on_back"),
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	def("split_index", &dispatch_ex<split_index, Ex&>, 
		 (arg("ex"),
		  arg("rules"),
		  arg("deep")=true,arg("repeat")=false,arg("depth")=0),
		 return_internal_reference<1>() );
	
	def("unwrap", &dispatch_ex<unwrap, Ex&>, 
		 (arg("ex"),
		  arg("wrapper")=new Ex(),
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
	def_prop<DifferentialForm>();
	def_prop<Distributable>();
	def_prop<DiracBar>();
	def_prop<EpsilonTensor>();
	def_prop<ExteriorDerivative>();
	def_prop<FilledTableau>();
	def_prop<GammaMatrix>();
	def_prop<ImaginaryI>();	
	def_prop<ImplicitIndex>();	
	def_prop<IndexInherit>();
	def_prop<Indices>();	
	def_prop<Integer>();
	def_prop<InverseMetric>();
	def_prop<KroneckerDelta>();
	def_prop<LaTeXForm>();
	def_prop<Matrix>();
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
	def_prop<Symbol>();
	def_prop<Symmetric>();
	def_prop<Tableau>();
	def_prop<TableauSymmetry>();
	def_prop<Traceless>();
	def_prop<Weight>();
	def_prop<WeightInherit>();
	def_prop<WeylTensor>();

	// Register exceptions. FIXME: This is not right yet: we create a proper 
	// Python exception object derived from Exception, but then we 
	// create a _separate_ C++ object with the same name and a Python
	// wrapper around that. The problem is that PyErr_NewException produces
	// a PyObject but that is not related to the C++ object.

	ConsistencyExceptionType=createExceptionClass("ConsistencyException");
	class_<ConsistencyException> pyConsistencyException("ConsistencyException", init<std::string>());
	pyConsistencyException.def("__str__", &ConsistencyException::what);
	register_exception_translator<ConsistencyException>(&translate_ConsistencyException);

	ArgumentExceptionType=createExceptionClass("ArgumentException");
	class_<ArgumentException> pyArgumentException("ArgumentException", init<std::string>());
	pyArgumentException.def("__str__", &ArgumentException::what);
	register_exception_translator<ArgumentException>(&translate_ArgumentException);

	ParseExceptionType=createExceptionClass("ParseException");
	class_<ParseException> pyParseException("ParseException", init<std::string>());
	pyParseException.def("__str__", &ParseException::what);
	register_exception_translator<ParseException>(&translate_ParseException);

	RuntimeExceptionType=createExceptionClass("RuntimeException");
	class_<RuntimeException> pyRuntimeException("RuntimeException", init<std::string>());
	pyRuntimeException.def("__str__", &RuntimeException::what);
	register_exception_translator<RuntimeException>(&translate_RuntimeException);

	NonScalarExceptionType=createExceptionClass("NonscalarException");
	class_<NonScalarException> pyNonScalarException("NonScalarException", init<std::string>());
	pyNonScalarException.def("__str__", &NonScalarException::py_what);
	register_exception_translator<NonScalarException>(&translate_NonScalarException);

	InternalErrorType=createExceptionClass("InternalError");
	class_<InternalError> pyInternalError("InternalError", init<std::string>());
	pyInternalError.def("__str__", &InternalError::py_what);
	register_exception_translator<InternalError>(&translate_InternalError);
	
	NotYetImplementedType=createExceptionClass("NotYetImplemented");
	class_<NotYetImplemented> pyNotYetImplemented("NotYetImplemented", init<std::string>());
	pyNotYetImplemented.def("__str__", &NotYetImplemented::py_what);
	register_exception_translator<NotYetImplemented>(&translate_NotYetImplemented);
	
#if BOOST_VERSION >= 106000
	boost::python::register_ptr_to_python<std::shared_ptr<Ex> >();
	//	boost::python::register_ptr_to_python<std::shared_ptr<Property> >();
#endif	

	// How can we give Python access to information stored in properties?
	}
