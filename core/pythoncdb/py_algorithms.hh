#pragma once

#include <pybind11/pybind11.h>
#include "py_ex.hh"
#include "py_helpers.hh"
#include "py_kernel.hh"
#include "py_progress.hh"

namespace cadabra {
	
	/// \ingroup pythoncore
	///
	/// Generic internal entry point for the Python side to execute a
	/// C++ algorithm. This gets called by the various apply_algo
	/// functions below, which in turn get called by the def_algo
	/// functions.
	template <class Algo>
	Ex_ptr apply_algo_base(Algo& algo, Ex_ptr ex, bool deep, bool repeat, unsigned int depth, bool pre_order=false)
		{
		Ex::iterator it = ex->begin();
		if (ex->is_valid(it)) {
			ProgressMonitor* pm = get_progress_monitor();
			algo.set_progress_monitor(pm);
			if(pre_order)
				ex->update_state(algo.apply_pre_order(repeat));
			else
				ex->update_state(algo.apply_generic(it, deep, repeat, depth));
			call_post_process(*get_kernel_from_scope(), ex);
			}

		return ex;
		}

	template <class Algo>
	Ex_ptr apply_algo(Ex_ptr ex, bool deep, bool repeat, unsigned int depth)
		{
		Algo algo(*get_kernel_from_scope(), *ex);
		return apply_algo_base(algo, ex, deep, repeat, depth, false);
		}

	template <class Algo, typename Arg1>
	Ex_ptr apply_algo(Ex_ptr ex, Arg1 arg1, bool deep, bool repeat, unsigned int depth)
		{
		Algo algo(*get_kernel_from_scope(), *ex, arg1);
		return apply_algo_base(algo, ex, deep, repeat, depth, false);
		}

	template <class Algo, typename Arg1, typename Arg2>
	Ex_ptr apply_algo(Ex_ptr ex, Arg1 arg1, Arg2 arg2, bool deep, bool repeat, unsigned int depth)
		{
		Algo algo(*get_kernel_from_scope(), *ex, arg1, arg2);
		return apply_algo_base(algo, ex, deep, repeat, depth, false);
		}

	template <class Algo, typename Arg1, typename Arg2, typename Arg3>
	Ex_ptr apply_algo(Ex_ptr ex, Arg1 arg1, Arg2 arg2, Arg3 arg3, bool deep, bool repeat, unsigned int depth)
		{
		Algo algo(*get_kernel_from_scope(), *ex, arg1, arg2, arg3);
		return apply_algo_base(algo, ex, deep, repeat, depth, false);
		}


	/// \ingroup pythoncore
	///
	/// Method to declare a Python function with variable number of
	/// arguments, and make that call a C++ algorithm as specified in
	/// the Algo template parameter.  This will make the algorithm
	/// traverse post-order, that is to say, first on the innermost
	/// child (or leaf) of an expression tree, and then, if that fails,
	/// on parent nodes, and so on.
	template<class Algo, typename... Args, typename... PyArgs>
	void def_algo(pybind11::module& m, const char* name, bool deep, bool repeat, unsigned int depth, PyArgs... pyargs)
		{
		m.def(name,
		      &apply_algo<Algo, Args...>,
		      pybind11::arg("ex"),
		      std::forward<PyArgs>(pyargs)...,
		      pybind11::arg("deep") = deep,
		      pybind11::arg("repeat") = repeat,
		      pybind11::arg("depth") = depth,
		      pybind11::doc(read_manual(m, "algorithms", name).c_str()),
		      pybind11::return_value_policy::reference_internal);
		}

	template <class Algo>
	Ex_ptr apply_algo_preorder(Ex_ptr ex, bool deep, bool repeat, unsigned int depth)
		{
		Algo algo(*get_kernel_from_scope(), *ex);
		return apply_algo_base(algo, ex, deep, repeat, depth, true);
		}

	template <class Algo, typename Arg1>
	Ex_ptr apply_algo_preorder(Ex_ptr ex, Arg1 arg1, bool deep, bool repeat, unsigned int depth)
		{
		Algo algo(*get_kernel_from_scope(), *ex, arg1);
		return apply_algo_base(algo, ex, deep, repeat, depth, true);
		}

	template <class Algo, typename Arg1, typename Arg2>
	Ex_ptr apply_algo_preorder(Ex_ptr ex, Arg1 arg1, Arg2 arg2, bool deep, bool repeat, unsigned int depth)
		{
		Algo algo(*get_kernel_from_scope(), *ex, arg1, arg2);
		return apply_algo_base(algo, ex, deep, repeat, depth, true);
		}

	/// \ingroup pythoncore
	///
	/// Method to declare a Python function with variable number of arguments, and
	/// make that call a C++ algorithm as specified in the Algo template parameter.
	/// In contrast to def_algo, this one will apply the algorithm in pre-order
	/// traversal style, that is, it will first attempt to apply on a node itself
	/// before traversing further down the child nodes and attempting there.
	template<class Algo, typename... Args, typename... PyArgs>
	void def_algo_preorder(pybind11::module& m, const char* name, bool deep, bool repeat, unsigned int depth, PyArgs... pyargs)
		{
		m.def(name,
		      &apply_algo_preorder<Algo, Args...>,
		      pybind11::arg("ex"),
		      std::forward<PyArgs>(pyargs)...,
		      pybind11::arg("deep") = deep,
		      pybind11::arg("repeat") = repeat,
		      pybind11::arg("depth") = depth,
		      pybind11::doc(read_manual(m, "algorithms", name).c_str()),
		      pybind11::return_value_policy::reference_internal);
		}


	void init_algorithms(pybind11::module& m);
	}
