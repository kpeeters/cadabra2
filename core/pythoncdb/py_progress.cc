#include <iostream>

#include "py_progress.hh"
#include "py_helpers.hh"

namespace cadabra {
	namespace py = pybind11;

	py::list ProgressMonitor_totals_helper(ProgressMonitor& self)
		{
		py::list list;
		auto totals = self.totals();
		for (auto& total : totals)
			list.append(total);
		return list;
		}


	ProgressMonitor *get_progress_monitor()
		{
		ProgressMonitor *pm = nullptr;

		try {
			pybind11::dict globals = get_globals();
			if (scope_has(globals, "__cdb_progress_monitor__")) {
				pm = globals["__cdb_progress_monitor__"].cast<ProgressMonitor*>();
				return pm;
				}
			pm = new ProgressMonitor();
			globals["__cdb_progress_monitor__"] = pm;
			}
		catch (pybind11::error_already_set& ex) {
			std::cerr << "*!?!?" << ex.what() << std::endl;
			}
		return pm;
		}


	void init_progress_monitor(py::module& m)
		{
		py::class_<ProgressMonitor>(m, "ProgressMonitor")
		.def("print", &ProgressMonitor::print)
		.def("totals", &ProgressMonitor_totals_helper);

		pybind11::class_<ProgressMonitor::Total>(m, "Total")
		.def_readonly("name", &ProgressMonitor::Total::name)
		.def_readonly("call_count", &ProgressMonitor::Total::call_count)
		.def_readonly("total_steps", &ProgressMonitor::Total::total_steps)
		.def("__str__", &ProgressMonitor::Total::str);

		}

	}