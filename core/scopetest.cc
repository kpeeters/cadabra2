#include <boost/python.hpp>

void cfun() 
	{
	boost::python::object locals(boost::python::borrowed(PyEval_GetLocals()));
	locals["a"] = 5;
	}

BOOST_PYTHON_MODULE(scopetest)
	{
	boost::python::def("cfun", &cfun);
	}
