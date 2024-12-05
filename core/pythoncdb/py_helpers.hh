#pragma once

#include <pybind11/pybind11.h>
#include <memory>

namespace cadabra {

	pybind11::object get_locals();
	pybind11::object get_globals();

	/// Get the installation prefix of the binary cadabra2 module,
	/// typically something like `/usr/lib/pythonX.YY/site-packages/`.
	std::string install_prefix_of_module();

	/// Determine whether the given python scope contains a variable
	/// with the given name.
	bool scope_has(const pybind11::dict& dict, const std::string& obj);

	/// Return the manual page for the category ("property"/"algorithm")
	/// and given name.
	std::string read_manual(pybind11::module& m, const char* category, const char* name);

}
