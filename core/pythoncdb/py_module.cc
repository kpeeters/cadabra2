#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

#include "py_algorithms.hh"
#include "py_ex.hh"
#include "py_kernel.hh"
#include "py_packages.hh"
#include "py_progress.hh"
#include "py_properties.hh"
#include "py_stopwatch.hh"
#include "py_tableau.hh"
#include "py_ntensor.hh"
#include "py_media.hh"
#include "py_helpers.hh"

#include "Kernel.hh"

namespace cadabra {
	namespace py = pybind11;

	std::string init_ipython()
		{
		py::exec("from IPython.display import Math");
		return "Cadabra typeset output for IPython notebook initialised.";
		}

	PYBIND11_MODULE(cadabra2, m)
		{
		py::options options;
		options.disable_function_signatures();

//		m.doc() = "Cadabra symbolic computer algebra";
		m.def("init_ipython", &init_ipython);

		// These must be initialized in the order of which
		// symbols appear
		init_packages(m);
		init_kernel(m);
		init_progress_monitor(m);
		init_ntensor(m);
		init_media(m);
		init_stopwatch(m);
		init_ex(m);
		init_tableau(m);
		init_algorithms(m);
		init_properties(m);

		// Register the import hook which makes notebooks importable
		// as Python modules (see cdb_package_compiler.py).
		try {
			py::module::import("cdb_package_compiler").attr("install")(m.attr("compile_package__"));
			}
		catch(py::error_already_set& ex) {
			std::string msg = std::string("Cadabra notebook import hook not available: ") + ex.what();
			if(PyErr_WarnEx(PyExc_RuntimeWarning, msg.c_str(), 1) < 0)
				throw py::error_already_set();
			}
		}

	}
