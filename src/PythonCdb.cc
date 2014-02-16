
#include "PythonCdb.hh"
#include "Parser.hh"
#include "Exceptions.hh"

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

	parser.tree.print_entire_tree(std::cout);
	}

std::string Ex::get() 
	{ 
	return ex; 
	}

std::string Algo(Ex& ex, bool repeat) 
	{
	if(repeat) std::cout << "true" << std::endl;
	else std::cout << "false" << std::endl;
	return ex.get();
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

BOOST_PYTHON_MODULE(cadabra)
	{
	using namespace boost::python;

	class_<ParseException> pyParseException("ParseException", init<std::string>());
	ParseExceptionType=pyParseException.ptr();

	class_<Ex> pyEx("Ex", init<std::string>());
	pyEx.def("get", &Ex::get);

	// You can call algorithms on objects like this. The parameters are
	// labelled by names.
	def("Algo",&Algo, (arg("ex"),arg("repeat")));
	
	// This does not work; Python does not allow us to use an exclamation mark.
	def("bang!", &bang);

	// How can we give a handle to the tree in python? And how can we give
	// Python access to properties?

	register_exception_translator<ParseException>(&translate_ParseException);
	}
