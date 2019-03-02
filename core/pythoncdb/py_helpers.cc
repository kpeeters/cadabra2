#include "../Config.hh"
#include <fstream>
#include "json/json.h"
#include "py_helpers.hh"

namespace cadabra {

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

	std::string read_manual(const char* category, const char* name)
		{
		Json::Value root;
		Json::Reader reader;
		std::ifstream ifs(std::string(CMAKE_INSTALL_PREFIX) + "/share/cadabra2/manual/" + category + "/" + name + ".cnb");
		bool parsing_successful = reader.parse(ifs, root, false);
		if (!parsing_successful) {
			return "Failed to collect help information";
			}
		return (*root["cells"].begin())["source"].asString();
		}

	}