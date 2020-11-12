#include "../Config.hh"
#include <fstream>
#include "py_helpers.hh"
#include "nlohmann/json.hpp"

namespace cadabra {

	namespace py = pybind11;

	py::object get_locals()
		{
		return py::reinterpret_borrow<py::object>(PyEval_GetLocals());
		}

	py::object get_globals()
		{
		return py::globals();
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
		std::ifstream ifs(std::string(CMAKE_INSTALL_PREFIX) + "/share/cadabra2/manual/" + category + "/" + name + ".cnb");
		try {
			nlohmann::json root=nlohmann::json::parse(ifs);
			return (*root["cells"].begin())["source"].get<std::string>();
			}
		catch(nlohmann::json::exception& ex) {
			return "Failed to collect help information";
			}
		}

	}
