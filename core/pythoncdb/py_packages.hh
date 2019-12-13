#include <string>
#include <pybind11/pybind11.h>

namespace cadabra {
	/// \ingroup files
	///
	/// Convert a Cadabra notebook file to a python package which can be imported
	/// using standard 'import' notation. Doing this will ignore all cells
	/// which are labelled `ignore_on_import`.

	void compile_package(const std::string& in_name, const std::string& out_name);

	void init_packages(pybind11::module& m);
	}
