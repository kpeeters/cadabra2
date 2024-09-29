#include "../Config.hh"
#include "../InstallPrefix.hh"
#include <fstream>
#include "py_helpers.hh"
#include "nlohmann/json.hpp"
#include <filesystem>
#include <iostream>
#include <regex>

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
			py::module_ sysconfig = py::module_::import("sysconfig");
			py::object result = sysconfig.attr("get_path")("platlib");
			spath = result.cast<std::string>();
			// Some older systems return the wrong path in platlib: they
			// use "dist-packages", but still return "site-packages". So we
			// test for the existence of platlib, and if it does not
			// exist, we swap "site-packages" <-> "dist-packages".
			auto dpath = std::filesystem::path(spath);
			if(!std::filesystem::is_directory(dpath)) {
				auto parent = dpath.parent_path();
				if(dpath.filename()=="site-packages")
					dpath = parent / "dist-packages";
				else
					dpath = parent / "site-packages";
				spath = dpath.string();
				}
			}  		
		return spath;
		}

	std::string read_manual(pybind11::module& m, const char* category, const char* name)
		{
		// We are assuming that if the module is installed in
		//    /usr/local/lib/python3.11/dist-packages/
		// that the manual pages can be found at
		//    /usr/local/share/cadabra2/manual/
		// So in other words, these are relative to `Python_SITEARCH`,
		// or what sysconfig calls `platlib`.
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
