#include <string>
#include <pybind11/pybind11.h>

namespace cadabra
	{
	void compile_package(const std::string& in_name, const std::string& out_name);

	void init_packages(pybind11::module& m);
	}