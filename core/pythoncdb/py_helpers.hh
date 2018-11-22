#pragma once

#include <pybind11/pybind11.h>
#include <memory>

namespace cadabra
	{

	pybind11::object get_locals();
	pybind11::object get_globals();
	bool scope_has(const pybind11::dict& dict, const std::string& obj);

	}