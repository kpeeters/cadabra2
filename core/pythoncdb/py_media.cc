#include <pybind11/pybind11.h>
#include "Media.hh"

namespace py = pybind11;

namespace cadabra {
	void init_media(py::module& m)
		{
   	py::class_<LaTeXString>(m, "LaTeXString")
			.def(py::init<std::string>())
			.def("_latex_", &LaTeXString::latex)
			;
		}
}
