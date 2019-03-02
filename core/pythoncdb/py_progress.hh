#pragma once

#include <pybind11/pybind11.h>

#include "../ProgressMonitor.hh"

namespace cadabra {

	pybind11::list ProgressMonitor_totals_helper(ProgressMonitor& self);
	ProgressMonitor *get_progress_monitor();

	void init_progress_monitor(pybind11::module& m);

	}