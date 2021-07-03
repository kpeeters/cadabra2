#include <pybind11/pybind11.h>
#include "pythoncdb/py_algorithms.hh"
#include "pythoncdb/py_kernel.hh"
#include "Algorithm.hh"

using namespace cadabra;

// Virtual base class which uses ExNode instead of Ex::iterator so that
// it can be extended in Python
class PyAlgorithm : public Algorithm
{
public:
	PyAlgorithm(Ex_ptr ex)
		: Algorithm(*get_kernel_from_scope(), *ex)
		, ex(ex)
	{

	}

	result_t py_apply_generic(bool deep, bool repeat, unsigned int depth)
	{
		return apply_generic(deep, repeat, depth);
	}

	result_t py_apply_pre_order(bool repeat)
	{
		return apply_pre_order(repeat);
	}

	virtual bool py_can_apply(ExNode node) = 0;

	bool can_apply(Ex::iterator it) override
	{
		ExNode node(*get_kernel_from_scope(), ex);
		node.ex = ex;
		node.topit = it;
		node.it = it;
		node.stopit = it;
		node.stopit.skip_children();
		++node.stopit;
		node.update(true);

		return py_can_apply(node);
	}

	virtual Algorithm::result_t py_apply(ExNode node) = 0;

	Algorithm::result_t apply(Ex::iterator& it) override
	{
		ExNode node(*get_kernel_from_scope(), ex);
		node.ex = ex;
		node.topit = it;
		node.it = it;
		node.stopit = it;
		node.stopit.skip_children();
		++node.stopit;
		node.update(true);

		auto res = py_apply(node);
		it = node.it;
		return res;
	}

private:
	Ex_ptr ex;
};

// Trampoline class which allows extending the virtual base class PyAlgorithm
// inside Python
class PyAlgorithmTrampoline : public PyAlgorithm
{
public:
	using PyAlgorithm::PyAlgorithm;

	bool py_can_apply(ExNode node) override {
		PYBIND11_OVERRIDE_PURE_NAME(
			bool,
			PyAlgorithm,
			"can_apply",
			py_can_apply,
			node
		);
	}

	Algorithm::result_t py_apply(ExNode node) override {
		PYBIND11_OVERRIDE_PURE_NAME(
			Algorithm::result_t,
			PyAlgorithm,
			"apply",
			py_apply,
			node
		);
	}
};


PYBIND11_MODULE(_algorithm, m)
{
	namespace py = pybind11;

	py::class_<PyAlgorithm, PyAlgorithmTrampoline>(m, "Algorithm")
		.def(py::init<Ex_ptr>())
		.def("can_apply", &PyAlgorithm::py_can_apply)
		.def("apply", &PyAlgorithm::py_apply)
		.def("apply_generic", &PyAlgorithm::py_apply_generic,
			py::arg("deep") = true, py::arg("repeat") = false, py::arg("depth") = 0)
		.def("apply_pre_order", &PyAlgorithm::py_apply_pre_order, py::arg("repeat") = false);

	m.def("apply_algo_base", apply_algo_base<PyAlgorithm>);
}
