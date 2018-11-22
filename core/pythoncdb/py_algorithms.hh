#pragma once

#include <pybind11/pybind11.h>
#include "py_ex.hh"
#include "py_helpers.hh"
#include "py_kernel.hh"
#include "py_progress.hh"

namespace cadabra
	{
	template <class Algo, typename... Args>
	Ex_ptr apply_algo(Ex_ptr ex, Args... args, bool deep, bool repeat, unsigned int depth)
		{
		Algo algo(*get_kernel_from_scope(), *ex, args...);

		Ex::iterator it = ex->begin();
		if (ex->is_valid(it)) {
			//ProgressMonitor* pm = get_progress_monitor();
			ex->update_state(algo.apply_generic(it, deep, repeat, depth));
			call_post_process(*get_kernel_from_scope(), ex);
			}

		return ex;
		}

	template<class Algo, typename... Args, typename... PyArgs>
	void def_algo(pybind11::module& m, const char* name, bool deep = true, bool repeat = false, unsigned int depth = 0, PyArgs... args)
		{
		m.def(name,
			  &apply_algo<Algo, Args...>,
			  pybind11::arg("ex"),
			  std::forward<PyArgs>(args)...,
			  pybind11::arg("deep") = deep,
			  pybind11::arg("repeat") = repeat,
			  pybind11::arg("depth") = depth,
			  pybind11::doc(read_manual("algorithms", name).c_str()),
			  pybind11::return_value_policy::reference_internal);
		}

	template <class Algo, typename... Args>
	Ex_ptr apply_algo_preorder(Ex_ptr ex, Args... args, bool deep, bool repeat, unsigned int depth)
		{
		Algo algo(*get_kernel_from_scope(), *ex, args...);

		Ex::iterator it = ex->begin();
		if (ex->is_valid(it)) {
			//ProgressMonitor* pm = get_progress_monitor();
			ex->update_state(algo.apply_pre_order(repeat));
			call_post_process(*get_kernel_from_scope(), ex);
			}

		return ex;
		}

	template<class Algo, typename... Args, typename... PyArgs>
	void def_algo_preorder(pybind11::module& m, const char* name, bool deep = true, bool repeat = false, unsigned int depth = 0, PyArgs... args)
		{
		m.def(name,
			  &apply_algo_preorder<Algo, Args...>,
			  pybind11::arg("ex"),
			  std::forward<PyArgs>(args)...,
			  pybind11::arg("deep") = deep,
			  pybind11::arg("repeat") = repeat,
			  pybind11::arg("depth") = depth,
			  pybind11::doc(read_manual("algorithms", name).c_str()),
			  pybind11::return_value_policy::reference_internal);
		}


	void init_algorithms(pybind11::module& m);
	}
