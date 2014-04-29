
#include "PythonCdb.hh"
#include "Parser.hh"
#include "Exceptions.hh"
#include "Kernel.hh"
#include "DisplayTeX.hh"
#include "PreClean.hh"
#include "PythonException.hh"

#include <boost/python/implicit.hpp>
#include <sstream>

#include "properties/AntiCommuting.hh"
#include "properties/AntiSymmetric.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/Distributable.hh"
#include "properties/Indices.hh"
#include "properties/IndexInherit.hh"

#include "algorithms/collect_terms.hh"
#include "algorithms/distribute.hh"
#include "algorithms/reduce_sub.hh"
#include "algorithms/rename_dummies.hh"
#include "algorithms/substitute.hh"

Kernel *get_kernel_from_scope(bool for_write=false) ;

// TODO: when we stick objects into the locals or globals, does Python become the owner and manage them?
//       where do the kernel copy constructor calls come from?

Ex::Ex(const Ex& other)
	{
	std::cout << "Ex copy constructor" << std::endl;
	}

// Output routines for Ex objects.

std::string Ex::str_() const
	{
//	std::cout << "reached Ex::str_ " << std::endl;
//	std::cout << *(tree.begin()->name)<< std::endl;
	std::ostringstream str;

	DisplayTeX dt(get_kernel_from_scope()->properties, tree);
	dt.output(str);

	return str.str();
	}

std::string Ex::repr_() const
	{
	std::ostringstream str;
	tree.print_entire_tree(str);
	return str.str();
	}

Ex::Ex(std::string ex_) 
	: ex(ex_)
	{
	Parser parser;
	std::stringstream str(ex);

	try {
		str >> parser;
		}
	catch(std::exception& except) {
		throw ParseException("Cannot parse");
		}

	tree=parser.tree;
	pre_clean(*get_kernel_from_scope(), tree, tree.begin());
	pull_in();
	register_as_last_expression();
	}

Ex *Ex::fetch_from_python(const std::string nm)
	{
	try {
		boost::python::object obj = boost::python::eval(nm.c_str());
		if(obj.is_none()) // We don't get here, an exception will have been thrown
			std::cout << "object unknown" << std::endl;
		else {
			Ex *ex = boost::python::extract<Ex *>(obj);
			return ex;
//			std::cout << *(ex->tree.begin()->name) << std::endl;
			}
		}
	catch(boost::python::error_already_set const &) {
		// In order to prevent the error from propagating, we have to read
		// it out. And in any case, we want to give some feedback to the user.
		std::string err = parse_python_exception();
		if(err.substr(0,29)=="<type 'exceptions.TypeError'>")
			std::cout << "ab is not a cadabra expression" << std::endl;
		else 
			std::cout << "ab is not defined" << std::endl;
//		std::cout << parse_python_exception() << std::endl;
		}

	return 0;
	}


void Ex::pull_in()
	{
	exptree::iterator it=tree.begin();
	while(it!=tree.end()) {
		if(*it->name=="@") {
			if(tree.number_of_children(it)==1) {
				std::string pobj = *(tree.begin(it)->name);
				Ex *ex = fetch_from_python(pobj);
				if(ex!=0) {
					it=tree.replace(it, ex->tree.begin());
					}
				}
			}
		++it;
		}

	return;
	}


std::string Ex::get() const
	{ 
	return ex; 
	}

void Ex::append(std::string v) 
	{
	ex+=v;
	}

void Ex::register_as_last_expression()
	{
	boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
	locals["_"]=boost::ref(this);
	}

// Templates to dispatch function calls in Python to algorithms in C++.

template<class F>
Ex *dispatch_1(Ex *ex, bool repeat)
	{
	F algo(*get_kernel_from_scope(), ex->tree);

	exptree::iterator it=ex->tree.begin().begin();
	if(repeat) {
		if(algo.apply_recursive(it)==false)
			std::cout << "no change" << std::endl;
		}
	else {
		if(algo.apply_once(it)==false)
			std::cout << "no change" << std::endl;
		}

	return ex;
	}

template<class F>
Ex *dispatch_1_string(const std::string& ex, bool repeat)
	{
	Ex *exobj = new Ex(ex);
	return dispatch_1<F>(exobj, repeat);
	}

template<class F>
Ex *dispatch_2(Ex *ex, Ex *args, bool repeat)
	{
	F algo(*get_kernel_from_scope(), ex->tree, args->tree);

	exptree::iterator it=ex->tree.begin().begin();
	if(repeat) {
		if(algo.apply_recursive(it)==false)
			std::cout << "no change" << std::endl;
		}
	else {
		if(algo.apply_once(it)==false)
			std::cout << "no change" << std::endl;
		}
	return ex;
	}

template<class F>
Ex *dispatch_2_string(Ex *ex, const std::string& args, bool repeat)
	{
	Ex *argsobj = new Ex(args);
	return dispatch_2<F>(ex, argsobj, repeat);
	}


std::string print_tree(Ex *ex)
	{
	std::ostringstream str;
	ex->tree.print_entire_tree(str);
	return str.str();
	}
 
PyObject *ParseExceptionType = NULL;

void translate_ParseException(const ParseException &e)
	{
	assert(ParseExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ParseExceptionType, pythonExceptionInstance.ptr());
	}

BaseProperty::BaseProperty(const std::string& s)
	: creation_message(s)
	{
	}

// Return the kernel in local scope if any, or the one in global scope if none is available
// in local scope. However, if a request for a writeable kernel is made, and no local is
// present, it will always be created (and filled with the content of the global one).

Kernel *get_kernel_from_scope(bool for_write) 
	{
	// Lookup the properties object in the local scope. 
	boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
	boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
	Kernel *local_kernel=0;
	Kernel *global_kernel=0;
	try {
		boost::python::object obj = locals["cadabra_kernel"];
		local_kernel = boost::python::extract<Kernel *>(obj);
		}
	catch(boost::python::error_already_set& err) {
		std::string err2 = parse_python_exception();
		}
	try {
		boost::python::object obj = globals["cadabra_kernel"];
		global_kernel = boost::python::extract<Kernel *>(obj);
		}
	catch(boost::python::error_already_set& err) {
		std::string err2 = parse_python_exception();
		}
	
	if(for_write) {
		// need the local kernel no matter what
		if(local_kernel==0) {
			local_kernel = new Kernel();
			if(global_kernel) {
				local_kernel->properties = global_kernel->properties; 
				}
			locals["cadabra_kernel"]=boost::ref(local_kernel);
			// FIXME: copy global kernel if present
			}
		return boost::ref(local_kernel);
		}
	else {
		if(local_kernel) {
			return local_kernel;
			}
		else if(global_kernel) {
			return boost::ref(global_kernel);
			}
		else {
			// On first call?
			global_kernel = new Kernel();
			globals["cadabra_kernel"]=boost::ref(global_kernel);
			return global_kernel; //boost::ref(global_kernel);
			}
		}
	}

template<class Prop>
Property<Prop>::Property(Ex *ex) 
   : BaseProperty("Assigned property to ...")
	{
	exptree::iterator it=ex->tree.begin();
	assert(*(it->name)=="\\expression");
	it=ex->tree.begin(it);
	creation_message = "";

	Kernel *kernel=get_kernel_from_scope(true);
	Prop *p=new Prop();
//	std::cout << "inserting " << p->name() << std::endl;
	kernel->properties.master_insert(exptree(it), p);
	}

std::string BaseProperty::str_() const
	{
	return creation_message;
	}

std::string BaseProperty::repr_() const
	{
	return "Property::repr";
	}

// Templated function which declares various forms of the algorithm entry points in one shot.
// First the ones with no argument, just a repeat flag.

template<class F>
void def_algo_1(const std::string& name) 
	{
	using namespace boost::python;

	def(name.c_str(),  &dispatch_1<F>,                 (arg("ex"),arg("repeat")=true), return_internal_reference<1>() );
	def(name.c_str(),  &dispatch_1_string<F>,          (arg("ex"),arg("repeat")=true), return_value_policy<manage_new_object>() );
	}

template<class F>
void def_algo_2(const std::string& name) 
	{
	using namespace boost::python;

	def(name.c_str(),  &dispatch_2<F>,                 (arg("ex"),arg("args"),arg("repeat")=true), return_internal_reference<1>() );
	def(name.c_str(),  &dispatch_2_string<F>,          (arg("ex"),arg("args"),arg("repeat")=true), return_value_policy<manage_new_object>() );
	}

//HERE:
// Do we want to accept a vector of Ex objects or something less general?

template<class F, class p1>
void def_algo_new(const std::string& name)
	{
	using namespace boost::python;

	def(name.c_str(),  &dispatch_2<F, p1>,        (arg("ex"),arg("args"),arg("repeat")=true), return_internal_reference<1>() );
//	def(name.c_str(),  &dispatch_2_string<F, p1>, (arg("ex"),arg("args"),arg("repeat")=true), return_value_policy<manage_new_object>() );
	
//	boost::python::list& ns
	}

void callback(Ex *ex, boost::python::object obj) 
	{
	std::cout << "calling back to python" << std::endl;
	if(obj.is_none()) {
		std::cout << "no callback given, ignoring" << std::endl;
		} 
	else {
		obj(boost::ref(ex));
		}
	}

void backdoor()
	{
	try {
		boost::python::object obj = boost::python::eval("ab");
		if(obj.is_none()) // We don't get here, an exception will have been thrown
			std::cout << "object unknown" << std::endl;
		else {
			Ex *ex = boost::python::extract<Ex *>(obj);
			std::cout << *(ex->tree.begin()->name) << std::endl;
			}
		}
	catch(boost::python::error_already_set const &) {
		// In order to prevent the error from propagating, we have to read
		// it out. And in any case, we want to give some feedback to the user.
		std::string err = parse_python_exception();
		if(err.substr(0,29)=="<type 'exceptions.TypeError'>")
			std::cout << "ab is not a cadabra expression" << std::endl;
		else 
			std::cout << "ab is not defined" << std::endl;
//		std::cout << parse_python_exception() << std::endl;
		}
	}

void fun() 
	{
	boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
	locals["fik"] = 42;
	locals["_fik"] = 43;
	}

// Entry point for registration of the Cadabra Python module. 
// This registers the main Ex class which wraps Cadabra expressions, as well
// as the various algorithms that can act on these.
// http://stackoverflow.com/questions/6050996/boost-python-overloaded-functions-with-default-arguments-problem

BOOST_PYTHON_MODULE(cadabra2)
	{
	using namespace boost::python;

	class_<ParseException> pyParseException("ParseException", init<std::string>());
	ParseExceptionType=pyParseException.ptr();

	class_<Kernel> pyPr("Kernel", init<>());

	class_<Ex> pyEx("Ex", init<std::string>());
	pyEx.def("get",     &Ex::get)
		.def("append",   &Ex::append)
		.def("__str__",  &Ex::str_)
		.def("__repr__", &Ex::repr_);

	// test
	def("callback", &callback, (arg("ex"), arg("callback")=object()) );
	def("backdoor", &backdoor);
	def("fun", &fun);

	def("tree", &print_tree);

	// You cannot use implicitly_convertible to convert a string parameter to an Ex object
	// automatically: think about how that would work in C++. You would need to be able to
	// pass a 'std::string' to a function that expects an 'Ex *'. That will never work.

	// You can call algorithms on objects like this. The parameters are
	// labelled by names.
	def_algo_1<collect_terms>("collect_terms");
	def_algo_1<distribute>("distribute");
	def_algo_1<rename_dummies>("rename_dummies");
	def_algo_1<reduce_sub>("reduce_sub");
	def_algo_1<sort_product>("sort_product");
	def_algo_2<substitute>("substitute");
//	def_algo_new<keep_terms, boost::python::list>("keep_terms");

	// All properties on the Python side derive from the C++ BaseProperty, which is
	// called Property on the Python side.
	class_<BaseProperty> pyBaseProperty("Property", no_init);
	pyBaseProperty.def("__str__", &BaseProperty::str_)
		.def("__repr__", &BaseProperty::repr_);
	
	class_<Property<AntiCommuting>,      bases<BaseProperty> >("AntiCommuting", init<Ex *>());
	class_<Property<AntiSymmetric>,      bases<BaseProperty> >("AntiSymmetric", init<Ex *>());
	class_<Property<CommutingAsProduct>, bases<BaseProperty> >("CommutingAsProduct", init<Ex *>());
	class_<Property<CommutingAsSum>,     bases<BaseProperty> >("CommutingAsSum", init<Ex *>());
	class_<Property<Distributable>,      bases<BaseProperty> >("Distributable", init<Ex *>());
	class_<Property<Indices>,            bases<BaseProperty> >("Indices", init<Ex *>());
	class_<Property<IndexInherit>,       bases<BaseProperty> >("IndexInherit", init<Ex *>());

	// How can we add parameters to the constructor?

	// How can we give Python access to properties?

	register_exception_translator<ParseException>(&translate_ParseException);
	}
