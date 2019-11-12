#include <fstream>
#include <ctime>
#include "../CdbPython.hh"

#include "py_packages.hh"

namespace cadabra {


	void compile_package(const std::string& in_name, const std::string& out_name)
		{
		// Get current time info
		//		std::time_t t = std::time(nullptr);
		//		std::tm tm = *std::localtime(&t);

		// Only compile if the notebook is newer than the compiled package
		struct stat f1, f2;
		if (stat(in_name.c_str(), &f1) == 0 && stat(out_name.c_str(), &f2) == 0 && f1.st_mtime < f2.st_mtime)
			return;

		std::string pystr;
		if(in_name.size()>3 && in_name.substr(in_name.size()-4)==".cdb")
			pystr = cadabra::cdb2python(in_name, false);
		else
			pystr = cadabra::cnb2python(in_name, false);
		
		if (pystr != "") {
			std::ofstream ofs(out_name);
			//		std::ofstream ofs(name+".py");
			ofs << pystr;
			}
		}


	void init_packages(pybind11::module& m)
		{
		m.def("compile_package__", &compile_package);
		}

	}
