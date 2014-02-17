
#include "PythonCdb.hh"
#include "Parser.hh"
#include "Exceptions.hh"
#include <boost/python/implicit.hpp>
#include <sstream>
#include "algorithms/distribute.hh"

Ex::Ex(const Ex& other)
	{
	// we cheat
	ex="B_{m n}";
	}

std::string Ex::to_string() const
	{
	std::ostringstream str;
	tree.print_entire_tree(str);

	return str.str();
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

Ex *Algo(Ex *ex, bool repeat) 
	{
//	if(repeat) std::cout << "true" << std::endl;
//	else std::cout << "false" << std::endl;
//	std::cout << ex->get() << std::endl;

	return ex;
	}

Ex *Algo2(const std::string& ex, bool repeat)
	{
//	std::cout << "from string" << std::endl;
	Ex *exobj = new Ex(ex);
	return Algo(exobj, repeat);
	}

Ex *distribute_algo(Ex *ex, bool repeat)
	{
	distribute dst(ex->tree, ex->tree.begin());

	exptree::iterator it=ex->tree.begin().begin();
	if(dst.can_apply(it)) {
		dst.apply(it);
		}
	else {
		std::cout << "cannot apply" << std::endl;
		}

	return ex;
	}

Ex *distribute_algo2(const std::string& ex, bool repeat)
	{
	Ex *exobj = new Ex(ex);
	return distribute_algo(exobj, repeat);
	}

PyObject *ParseExceptionType = NULL;

void translate_ParseException(const ParseException &e)
	{
	assert(ParseExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ParseExceptionType, pythonExceptionInstance.ptr());
	}

void bang(Ex& ex)
	{
	std::cout << "BANG! " << ex.get() << std::endl;
	}

// Entry point for registration of the Cadabra Python module. 
// This registers the main Ex class which wraps Cadabra expressions, as well
// as the various algorithms that can act on these.
// http://stackoverflow.com/questions/6050996/boost-python-overloaded-functions-with-default-arguments-problem

BOOST_PYTHON_MODULE(cadabra)
	{
	using namespace boost::python;

	class_<ParseException> pyParseException("ParseException", init<std::string>());
	ParseExceptionType=pyParseException.ptr();

	class_<Ex> pyEx("Ex", init<std::string>());
	pyEx.def("get",     &Ex::get)
		.def("append",   &Ex::append)
		.def("__repr__", &Ex::to_string);


//	implicitly_convertible<std::string, Ex>();

	// You can call algorithms on objects like this. The parameters are
	// labelled by names.
	def("Algo",  &Algo,  (arg("ex"),arg("repeat")), return_internal_reference<1>() );
	def("Algo",  &Algo2, (arg("ex"),arg("repeat")), return_value_policy<manage_new_object>() );

	def("distribute",  &distribute_algo,  (arg("ex"),arg("repeat")), return_internal_reference<1>() );
	def("distribute",  &distribute_algo2, (arg("ex"),arg("repeat")), return_value_policy<manage_new_object>() );


	// How can we give a handle to the tree in python? And how can we give
	// Python access to properties?

	register_exception_translator<ParseException>(&translate_ParseException);
	}
