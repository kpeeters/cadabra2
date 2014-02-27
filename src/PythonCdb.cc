
#include "PythonCdb.hh"
#include "Parser.hh"
#include "Exceptions.hh"
#include "Kernel.hh"
#include "DisplayTeX.hh"
#include <boost/python/implicit.hpp>
#include <sstream>

#include "properties/Distributable.hh"
#include "algorithms/distribute.hh"

Kernel kernel;

Ex::Ex(const Ex& other)
	{
	}

std::string Ex::str_() const
	{
	std::ostringstream str;
//	tree.print_entire_tree(str);
	DisplayTeX dt(kernel.properties, tree);
	dt.output(str);

	return str.str();
	}

std::string Ex::repr_() const
	{
	return "repr";
	}

Ex& Ex::operator=(const Ex& other)
	{
	ex="C_{m n}";
	return *this;
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
Ex *dispatch(Ex *ex, bool repeat)
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
Ex *dispatch_defaults(Ex *ex)
	{
	return dispatch<F>(ex, true);
	}

template<class F>
Ex *dispatch_string(const std::string& ex, bool repeat)
	{
	Ex *exobj = new Ex(ex);
	return dispatch<F>(exobj, repeat);
	}

template<class F>
Ex *dispatch_string_defaults(const std::string& ex)
	{
	return dispatch_string<F>(ex, true);
	}


 
PyObject *ParseExceptionType = NULL;

void translate_ParseException(const ParseException &e)
	{
	assert(ParseExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ParseExceptionType, pythonExceptionInstance.ptr());
	}

// Properties. This should probably return a reference to a 'property' object
// so we can query it from Python as well.
// Fixme: we also had this kind of stuff in src/modules/properties.cc; can that all go?

void attach_Distributable(Ex *ex) 
	{
	exptree::iterator it=ex->tree.begin();
	assert(*(it->name)=="\\expression");
	it=ex->tree.begin(it);
	kernel.properties.insert_prop(exptree(it), new Distributable());
	}

// Templates to attach properties to Ex objects.

template<class Prop>
void attach(Ex *ex)
	{
	exptree::iterator it=ex->tree.begin();
	assert(*(it->name)=="\\expression");
	it=ex->tree.begin(it);
	kernel.properties.insert_prop(exptree(it), new Prop());
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


	// TODO: in order to be able to insert already defined objects into an existing tree,
	// we need to use 'extract'. How does that work with extracting an Ex?

//	implicitly_convertible<std::string, Ex>();

	// You can call algorithms on objects like this. The parameters are
	// labelled by names.
	def("distribute",  &dispatch<distribute>,                 (arg("ex"),arg("repeat")), return_internal_reference<1>() );
	def("distribute",  &dispatch_defaults<distribute>,        (arg("ex")),               return_internal_reference<1>() );
	def("distribute",  &dispatch_string<distribute>,          (arg("ex"),arg("repeat")), return_value_policy<manage_new_object>() );
	def("distribute",  &dispatch_string_defaults<distribute>, (arg("ex")),               return_value_policy<manage_new_object>() );

	def("Distributable",  &attach<Distributable>, return_value_policy<manage_new_object>() );


	// How can we give a handle to the tree in python? And how can we give
	// Python access to properties?

	register_exception_translator<ParseException>(&translate_ParseException);
	}
