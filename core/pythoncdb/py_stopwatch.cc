#include <sstream>
#include "../Stopwatch.hh"

#include "py_stopwatch.hh"

namespace cadabra
	{
	namespace py = pybind11;

	void init_stopwatch(py::module& m)
		{
		pybind11::class_<Stopwatch>(m, "Stopwatch")
			.def(pybind11::init<>())
			.def("start", &Stopwatch::start)
			.def("stop", &Stopwatch::stop)
			.def("reset", &Stopwatch::reset)
			.def("seconds", &Stopwatch::seconds)
			.def("useconds", &Stopwatch::useconds)
			.def("__str__", [](const Stopwatch& s) {
			std::stringstream ss;
			ss << s;
			return ss.str();
			});
		}
	}