#include "py_helpers.hh"

namespace cadabra
	{

	namespace py = pybind11;

	py::object get_locals()
		{
		return py::reinterpret_borrow<py::object>(PyEval_GetLocals());
		}

	py::object get_globals()
		{

		return py::reinterpret_borrow<py::object>(PyEval_GetGlobals());
		}

	bool scope_has(const py::dict& dict, const std::string& obj)
		{
		for (const auto& item : dict)
			if (item.first.cast<std::string>() == obj)
				return true;
		return false;
		}

	}