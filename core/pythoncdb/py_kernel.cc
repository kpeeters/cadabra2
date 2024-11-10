#include "properties/Distributable.hh"
#include "properties/IndexInherit.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/DependsInherit.hh"
#include "properties/NumericalFlat.hh"
#include "properties/WeightInherit.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/Derivative.hh"
#include "properties/Accent.hh"
#include "properties/Tableau.hh"
#include "properties/FilledTableau.hh"

#include "CdbPython.hh"

#include <pybind11/functional.h>
#include "py_globals.hh"
#include "py_helpers.hh"
#include "py_kernel.hh"
#include "py_ex.hh"

namespace cadabra {
	Kernel *create_scope()
		{
		Kernel *k = new Kernel(true);
		return k;
		}

	Kernel *create_scope_from_global()
		{
		Kernel *k = create_empty_scope();
		// FIXME: copy global properties
		return k;
		}

	Kernel *create_empty_scope()
		{
		Kernel *k = new Kernel(false);
		return k;
		}

	Kernel *get_kernel_from_scope()
		{
		Kernel *kernel = nullptr;

		// Try and find the kernel in the local scope
		auto locals = get_locals();
		if (locals && scope_has(locals, "__cdbkernel__")) {
			kernel = locals["__cdbkernel__"].cast<Kernel*>();
			return kernel;
			}

		// No kernel in local scope, find one in global scope.
		auto globals = get_globals();
		if (globals && scope_has(globals, "__cdbkernel__")) {
			kernel = globals["__cdbkernel__"].cast<Kernel*>();
			return kernel;
			}

		// No kernel in local or global scope, construct a new global one
		kernel = create_scope();
		globals["__cdbkernel__"] = kernel;
		return kernel;
		}

	void kernel_configure_warnings(Kernel& kernel, pybind11::kwargs kwargs)
		{
		if (kwargs) {
			for (auto item : kwargs) {
				auto key = item.first.cast<std::string>();
				if (key == "level") {
					try {
						auto value = item.second.cast<Kernel::warn_t>();
						kernel.warning_level = value;
						}
					catch (pybind11::cast_error&) {
						throw std::invalid_argument("named argument 'level' expected an integer");
						}
					}
				else if (key == "callback") {
					try {
						if (item.second.is_none()) {
							kernel.warning_callback = nullptr;
							}
						else {
							auto value = item.second.cast<std::function<void(const std::string&)>>();
							kernel.warning_callback = value;
							}
						}
					catch (pybind11::cast_error&) {
						throw std::invalid_argument("named argument 'callback' expected None or function with signature void(const std::string&)");
						}
					}
				else {
					throw std::invalid_argument("received unrecognised argument '" + key + "'");
					}
				}
			}
		}

	void init_kernel(pybind11::module& m)
		{
		// Declare the Kernel object for Python so we can store it in the local Python context.
		// We add a 'cadabra2.__cdbkernel__' object to the main module scope, and will
		// pull that into the interpreter scope in the 'cadabra2_default.py' file.
		pybind11::enum_<Kernel::scalar_backend_t>(m, "scalar_backend_t")
			.value("sympy", Kernel::scalar_backend_t::sympy)
			.value("mathematica", Kernel::scalar_backend_t::mathematica)
			.export_values();

		pybind11::enum_<Kernel::warn_t>(m, "warn_t")
			.value("notset", Kernel::warn_t::notset)
			.value("info", Kernel::warn_t::info)
			.value("debug", Kernel::warn_t::debug)
			.value("warning", Kernel::warn_t::warning)
			.value("error", Kernel::warn_t::error)
			.value("critical", Kernel::warn_t::critical);

		pybind11::class_<Kernel>(m, "Kernel", pybind11::dynamic_attr())
			.def(pybind11::init<bool>())
			.def_readonly_static("version", &Kernel::version)
			.def_readonly_static("build", &Kernel::build)
			.def_readwrite("display_fractions", &Kernel::display_fractions)
			.def_readwrite("call_embedded_python_functions", &Kernel::call_embedded_python_functions)
			.def_readwrite("scalar_backend", &Kernel::scalar_backend)
			.def("warn", &Kernel::warn, pybind11::arg("msg"), pybind11::arg("level") = 0)
			.def("configure_warnings", kernel_configure_warnings);

		pybind11::class_<ConvertData>(m, "ConvertData")
			.def(pybind11::init<>())
			.def(pybind11::init<std::string, std::string, std::string, std::string>())
			.def_readwrite("lhs", &ConvertData::lhs)
			.def_readwrite("rhs", &ConvertData::rhs)
			.def_readwrite("op", &ConvertData::op)			
			.def_readwrite("indent", &ConvertData::indent);
		
		Kernel* kernel = create_scope();
		m.attr("__cdbkernel__") = pybind11::cast(kernel);

		m.def("create_scope", &create_scope,
		      pybind11::return_value_policy::take_ownership);
		m.def("create_scope_from_global", &create_scope_from_global,
		      pybind11::return_value_policy::take_ownership);
		m.def("create_empty_scope", &create_empty_scope,
		      pybind11::return_value_policy::take_ownership);

		m.def("cdb2python", &cdb2python);
		m.def("cdb2python_string", &cdb2python_string);
		m.def("convert_line", &convert_line);
		}

	}
