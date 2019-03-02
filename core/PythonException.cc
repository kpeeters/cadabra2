
#include <pybind11/pybind11.h>
#include <stdexcept>
#include "PythonException.hh"

std::string parse_python_exception()
	{
	PyObject *type_ptr = NULL, *value_ptr = NULL, *traceback_ptr = NULL;

	// Fetch the exception info from the Python C API
	PyErr_Fetch(&type_ptr, &value_ptr, &traceback_ptr);

	std::string ret("Unfetchable Python error");
	if(type_ptr != NULL) {
		pybind11::handle h_type(type_ptr);
		pybind11::str    type_pstr(h_type);
		try {
			ret = type_pstr.cast<std::string>();
			} catch(const pybind11::value_error& e) {
			ret = "Unknown exception type";
			}
		}
	if(value_ptr) {
		pybind11::handle h_val(value_ptr);
		pybind11::str    a(h_val);
		try {
			ret += a.cast<std::string>();
			} catch(const pybind11::value_error &e) {
			ret += ": Unparseable Python error: ";
			}
		}

	// Parse lines from the traceback using the Python traceback module
	if(traceback_ptr) {
		pybind11::handle h_tb(traceback_ptr);
		// Load the traceback module and the format_tb function
		pybind11::object tb(pybind11::module::import("traceback"));
		pybind11::object fmt_tb(tb.attr("format_tb"));
		// Call format_tb to get a list of traceback strings
		pybind11::object tb_list(fmt_tb(h_tb));
		// Extract the string, check the extraction, and fallback in necessary
		try {
			for(auto &s: tb_list)
				ret += s.cast<std::string>();
			} catch(const pybind11::value_error &e) {
			ret += ": Unparseable Python traceback";
			}
		}

	return ret;
	}
