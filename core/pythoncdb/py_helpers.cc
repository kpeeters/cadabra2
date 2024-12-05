#include "../Config.hh"
#include "../InstallPrefix.hh"
#include <fstream>
#include "py_helpers.hh"
#include <pybind11/embed.h>
#include "nlohmann/json.hpp"
#include <filesystem>
#include <iostream>
#include <regex>
#include "Kernel.hh"

namespace py = pybind11;

namespace cadabra {

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
		for (const auto& item : dict) {
			if (item.first.cast<std::string>() == obj) {
				return true;
				}
			}
		return false;
		}

	// The `install_prefix` function of `InstallPrefix.cc` will return the
	// installation prefix of the python interpreter when we run `cadabra2`
	// (the python script) or just import the cadabra2 module from a python
	// session. That is typically *not* where the manual pages are installed;
	// those are installed relative to the location of the cadabra2 module,
	// not relative to the python interpreter. We can get this location using
	// a bit of Python.
	
	std::string install_prefix_of_module()
		{
		static std::string spath;

		if(spath=="") {
			// We have an empty helper module 'cdb.main' which we import
			// to figure out the location, as the __file__ attribute will
			// contain that after successful import.
			py::module_ cdb = py::module_::import("cdb.main");
			py::object  result = cdb.attr("__file__");
			std::string tmp = result.cast<std::string>();
			auto dpath = std::filesystem::path(tmp).parent_path().parent_path();
			if(!std::filesystem::is_regular_file(dpath / "cadabra2_defaults.py")) {
				throw std::logic_error("The cadabra2 binary module is in "
											  +dpath.string()
											  +" but the initialisation file 'cadabra2_defaults.py' is not there;"
											  +" giving up.");
				}
			spath = dpath.string();
			}  		
		return spath;
		}

	std::string read_manual(pybind11::module& m, const char* category, const char* name)
		{
		// If the module is installed in
		//
		//    /usr/local/lib/pythonX.YY/dist-packages/
		//
		// (which is `install_prefix_of_module()`) or
		//
		//    /usr/local/lib/pythonX.YY/site-packages/
		//
		// the manual pages can be found at
		//
		//    /usr/local/share/cadabra2/manual/
		
		std::string manual_page = install_prefix_of_module()
			+ "/../../../share/cadabra2/manual/" + category + "/" + name + ".cnb";
		std::ifstream ifs(manual_page);
		try {
			nlohmann::json root=nlohmann::json::parse(ifs);
			std::string ret = (*root["cells"].begin())["source"].get<std::string>();
			ret = std::regex_replace(ret, std::regex(R"(\\algorithm\{(.*)\}\{(.*)\})"), "$2");
			ret = std::regex_replace(ret, std::regex(R"(\\property\{(.*)\}\{(.*)\})"), "$2");
			ret = std::regex_replace(ret, std::regex(R"(\\algo\{([^\}]*)\})"), "$1");
			ret = std::regex_replace(ret, std::regex(R"(\\prop\{([^\}]*)\})"), "$1");
			ret += "\n\nFor more information see https://cadabra.science/manual/"+std::string(name)+".html";
			return ret;
			}
		catch(nlohmann::json::exception& ex) {
			return "Failed to collect help information; no info at "+manual_page+".";
			}
		}

	}
