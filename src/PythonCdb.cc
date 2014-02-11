
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
	}

std::string Ex::get() 
	{ 
	return ex; 
	}

PyObject *ParseExceptionType = NULL;

void translate_ParseException(const ParseException &e)
	{
	assert(ParseExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ParseExceptionType, pythonExceptionInstance.ptr());
	}


// Entry point for registration of the Cadabra Python module.

BOOST_PYTHON_MODULE(cadabra)
	{
	using namespace boost::python;

	class_<ParseException> pyParseException("ParseException", init<std::string>());
	ParseExceptionType=pyParseException.ptr();

	class_<Ex> pyEx("Ex", init<std::string>());
	pyEx.def("get", &Ex::get);

	register_exception_translator<ParseException>(&translate_ParseException);
	}
