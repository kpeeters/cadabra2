
#include "ServerWrapper.hh"
#include <boost/python.hpp>

ServerWrapper::ServerWrapper()
	{
	boost::python::object globals(boost::python::borrowed(PyEval_GetGlobals()));
	server = globals["server"];
	python_group    = server.attr("group");
	python_progress = server.attr("progress");
	}

void ServerWrapper::group(std::string name)
	{
	python_group(name);
	}

void ServerWrapper::progress(int n, int total)
	{
	}
