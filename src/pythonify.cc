#include <boost/python.hpp>
#include <stdexcept>

static int i=3;

char const* yay()
	{
	return "Yay!";
	}

int num()
	{
	return i++;
	}

class ParseException : public std::logic_error {
	public:
		ParseException(std::string s) : std::logic_error(s) {}
};

class Ex {
	public:
		Ex(std::string ex) : ex_(ex) {}
		std::string get() { return ex_; }
		int kasper() { throw ParseException("parse error"); }

	private:
		std::string ex_;
};

PyObject *ParseExceptionType = NULL;

void translate_ParseException(const ParseException &e)
	{
	assert(ParseExceptionType != NULL);
	boost::python::object pythonExceptionInstance(e);
	PyErr_SetObject(ParseExceptionType, pythonExceptionInstance.ptr());
	}

BOOST_PYTHON_MODULE(cadabra)
	{
	using namespace boost::python;
	def("yay", yay);
	def("num", num);

	class_<ParseException> pyParseException("ParseException", init<std::string>());
	ParseExceptionType=pyParseException.ptr();

	class_<Ex> pyEx("Ex", init<std::string>());
	pyEx.def("get", &Ex::get).def("kasper", &Ex::kasper);

	register_exception_translator<ParseException>(&translate_ParseException);
	}
