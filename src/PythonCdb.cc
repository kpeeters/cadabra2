
#include "PythonCdb.hh"
#include "Parser.hh"
#include "Exceptions.hh"
#include "Kernel.hh"
#include "DisplayTeX.hh"
#include "PreClean.hh"
#include <boost/python/implicit.hpp>
#include <sstream>

#include "properties/Distributable.hh"
#include "properties/Indices.hh"
#include "properties/IndexInherit.hh"
#include "algorithms/distribute.hh"
#include "algorithms/rename_dummies.hh"
#include "algorithms/substitute.hh"

Kernel kernel;

Ex::Ex(const Ex& other)
	{
	}

// Output routines for Ex objects.

std::string Ex::str_() const
	{
//	std::cout << "reached Ex::str_ " << std::endl;
//	std::cout << *(tree.begin()->name)<< std::endl;
	std::ostringstream str;
	DisplayTeX dt(kernel.properties, tree);
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
	pre_clean(kernel, tree, tree.begin());
	}

std::string Ex::get() const
	{ 
	return ex; 
	}

void Ex::append(std::string v) 
	{
	ex+=v;
	}


// Templates to dispatch function calls in Python to algorithms in C++.

template<class F>
Ex *dispatch_1(Ex *ex, bool repeat)
	{
	F algo(kernel, ex->tree);

	exptree::iterator it=ex->tree.begin().begin();
	if(algo.can_apply(it)) {
		algo.apply(it);
		}
	else {
		std::cout << "cannot apply" << std::endl;
		}

	return ex;
	}

template<class F>
Ex *dispatch_1_defaults(Ex *ex)
	{
	return dispatch_1<F>(ex, true);
	}

template<class F>
Ex *dispatch_1_string(const std::string& ex, bool repeat)
	{
	Ex *exobj = new Ex(ex);
	return dispatch_1<F>(exobj, repeat);
	}

template<class F>
Ex *dispatch_1_string_defaults(const std::string& ex)
	{
	return dispatch_1_string<F>(ex, true);
	}

template<class F>
Ex *dispatch_2(Ex *ex, Ex *args, bool repeat)
	{
	F algo(kernel, ex->tree, args->tree);

	exptree::iterator it=ex->tree.begin().begin();
	if(repeat) {
//		std::cout << "applying on " << *it->name << " recursively" << std::endl;
		algo.apply_recursive(it);
		}
	else {
//		std::cout << "applying on " << *it->name << " once" << std::endl;
		algo.apply_once(it);
		}
	return ex;
	}

template<class F>
Ex *dispatch_2_defaults(Ex *ex, Ex *args)
	{
	return dispatch_2<F>(ex, args, true);
	}

template<class F>
Ex *dispatch_2_string(Ex *ex, const std::string& args, bool repeat)
	{
	Ex *argsobj = new Ex(args);
	return dispatch_2<F>(ex, argsobj, repeat);
	}

template<class F>
Ex *dispatch_2_string_defaults(Ex *ex, const std::string& args)
	{
	return dispatch_2_string<F>(ex, args, true);
	}


 
PyObject *ParseExceptionType = NULL;

void translate_ParseException(const ParseException &e)
	{
	assert(ParseExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ParseExceptionType, pythonExceptionInstance.ptr());
	}


// Templates to attach properties to Ex objects.
// FIXME: if we let Python manage the Property<> object, we need some
// way in which we can sync removal of that object with the removal of the
// C++ object.

template<class Prop>
Property<Prop> *attach(Ex *ex)
	{
	exptree::iterator it=ex->tree.begin();
	assert(*(it->name)=="\\expression");
	it=ex->tree.begin(it);
	kernel.properties.master_insert(exptree(it), new Prop());

	return new Property<Prop>("Assigned property to ...");
	}

BaseProperty::BaseProperty(const std::string& s)
	: creation_message(s)
	{
	}

template<class Prop>
Property<Prop>::Property(Ex *ex) 
   : BaseProperty("Assigned property to ...")
	{
	exptree::iterator it=ex->tree.begin();
	assert(*(it->name)=="\\expression");
	it=ex->tree.begin(it);
	creation_message = kernel.properties.master_insert(exptree(it), new Prop());
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

	def(name.c_str(),  &dispatch_1<F>,                 (arg("ex"),arg("repeat")), return_internal_reference<1>() );
	def(name.c_str(),  &dispatch_1_defaults<F>,        (arg("ex")),               return_internal_reference<1>() );
	def(name.c_str(),  &dispatch_1_string<F>,          (arg("ex"),arg("repeat")), return_value_policy<manage_new_object>() );
	def(name.c_str(),  &dispatch_1_string_defaults<F>, (arg("ex")),               return_value_policy<manage_new_object>() );
	}

template<class F>
void def_algo_2(const std::string& name) 
	{
	using namespace boost::python;

	def(name.c_str(),  &dispatch_2<F>,                 (arg("ex"),arg("args"),arg("repeat")), return_internal_reference<1>() );
	def(name.c_str(),  &dispatch_2_defaults<F>,        (arg("ex"),arg("args")),               return_internal_reference<1>() );
	def(name.c_str(),  &dispatch_2_string<F>,          (arg("ex"),arg("args"),arg("repeat")), return_value_policy<manage_new_object>() );
	def(name.c_str(),  &dispatch_2_string_defaults<F>, (arg("ex"),arg("args")),               return_value_policy<manage_new_object>() );
	}

void callback(boost::python::object obj, Ex *ex) 
	{
	std::cout << "calling back to python" << std::endl;
	std::cout << *(ex->tree.begin()->name) << std::endl;
	obj(boost::ref(ex));
	}

// Entry point for registration of the Cadabra Python module. 
// This registers the main Ex class which wraps Cadabra expressions, as well
// as the various algorithms that can act on these.
// http://stackoverflow.com/questions/6050996/boost-python-overloaded-functions-with-default-arguments-problem

BOOST_PYTHON_MODULE(pcadabra)
	{
	using namespace boost::python;

	class_<ParseException> pyParseException("ParseException", init<std::string>());
	ParseExceptionType=pyParseException.ptr();

	class_<Ex> pyEx("Ex", init<std::string>());
	pyEx.def("get",     &Ex::get)
		.def("append",   &Ex::append)
		.def("__str__",  &Ex::str_)
		.def("__repr__", &Ex::repr_);

	// test
	def("callback", &callback);

	// TODO: in order to be able to insert already defined objects into an existing tree,
	// we need to use 'extract'. How does that work with extracting an Ex?

//	implicitly_convertible<std::string, Ex>();

	// You can call algorithms on objects like this. The parameters are
	// labelled by names.
	def_algo_1<distribute>("distribute");
	def_algo_1<rename_dummies>("rename_dummies");
	def_algo_1<sort_product>("sort_product");
	def_algo_2<substitute>("substitute");

	class_<BaseProperty> pyBaseProperty("Property", no_init);
	pyBaseProperty.def("__str__", &BaseProperty::str_)
		.def("__repr__", &BaseProperty::repr_);
	
	class_<Property<Distributable>, bases<BaseProperty> >("Distributable", init<Ex *>());
	class_<Property<Indices>,       bases<BaseProperty> >("Indices", init<Ex *>());
	class_<Property<IndexInherit>,  bases<BaseProperty> >("IndexInherit", init<Ex *>());


	// How can we give a handle to the tree in python? And how can we give
	// Python access to properties?

	register_exception_translator<ParseException>(&translate_ParseException);
	}
