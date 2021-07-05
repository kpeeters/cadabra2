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
			.def_readonly("scalar_backend", &Kernel::scalar_backend)
			.def_readwrite("display_fractions", &Kernel::display_fractions)
			.def("warn", &Kernel::warn, pybind11::arg("msg"), pybind11::arg("level") = 0)
			.def("configure_warnings", kernel_configure_warnings);

		Kernel* kernel = create_scope();
		m.attr("__cdbkernel__") = pybind11::cast(kernel);

		m.def("kernel", [](pybind11::kwargs dict) {
			Kernel *k = get_kernel_from_scope();
			for (auto& item : dict) {
				std::string key = item.first.cast<std::string>();
				if (key == "scalar_backend") {
					std::string val = item.second.cast<std::string>();
					if (val == "sympy")            k->scalar_backend = Kernel::scalar_backend_t::sympy;
					else if (val == "mathematica") k->scalar_backend = Kernel::scalar_backend_t::mathematica;
					else throw ArgumentException("scalar_backend must be 'sympy' or 'mathematica'.");
					}
				else if(key == "call_embedded_python_functions") {
					bool val = item.second.cast<bool>();
					k->call_embedded_python_functions=val;
					}
				else {
					throw ArgumentException("unknown argument '" + key + "'.");
					}
				}
			});

		m.def("create_scope", &create_scope,
		      pybind11::return_value_policy::take_ownership);
		m.def("create_scope_from_global", &create_scope_from_global,
		      pybind11::return_value_policy::take_ownership);
		m.def("create_empty_scope", &create_empty_scope,
		      pybind11::return_value_policy::take_ownership);

		m.def("cdb2python", &cdb2python);
		m.def("cdb2python_string", &cdb2python_string);
		}

	}
