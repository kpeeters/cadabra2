#pragma once

#include <pybind11/pybind11.h>
#include <memory>

namespace cadabra {

	pybind11::object get_locals();
	pybind11::object get_globals();
	std::string install_prefix_of_module();
	bool scope_has(const pybind11::dict& dict, const std::string& obj);
	std::string read_manual(pybind11::module& m, const char* category, const char* name);
	}
