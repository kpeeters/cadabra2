#include "properties/Distributable.hh"
#include "properties/IndexInherit.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/DependsInherit.hh"
#include "properties/NumericalFlat.hh"
#include "properties/WeightInherit.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/Derivative.hh"
#include "properties/Accent.hh"

#include "py_globals.hh"
#include "py_helpers.hh"
#include "py_kernel.hh"
#include "py_ex.hh"

namespace cadabra {
	Kernel *create_scope()
		{
		Kernel *k = create_empty_scope();
		inject_defaults(k);
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
		Kernel *k = new Kernel();
		return k;
		}

	Kernel *get_kernel_from_scope()
		{
		Kernel *kernel = nullptr;

		// Try and find the kernel in the local scope
		auto locals = get_locals();
		if (scope_has(locals, "__cdbkernel__")) {
			kernel = locals["__cdbkernel__"].cast<Kernel*>();
			return kernel;
			}

		// No kernel in local scope, find one in global scope.
		pybind11::dict globals = get_globals();
		if (scope_has(globals, "__cdbkernel__")) {
			kernel = globals["__cdbkernel__"].cast<Kernel*>();
			return kernel;
			}

		// No kernel in local or global scope, construct a new global one
		kernel = new Kernel();
		inject_defaults(kernel);
		globals["__cdbkernel__"] = kernel;
		return kernel;
		}

	void inject_defaults(Kernel *k)
		{
		// Create and inject properties; these then get owned by the kernel.
		post_process_enabled = false;

		k->inject_property(new Distributable(), Ex_from_string("\\prod{#}", false, k), 0);
		k->inject_property(new IndexInherit(), Ex_from_string("\\prod{#}", false, k), 0);
		k->inject_property(new CommutingAsProduct(), Ex_from_string("\\prod{#}", false, k), 0);
		k->inject_property(new DependsInherit(), Ex_from_string("\\prod{#}", false, k), 0);
		k->inject_property(new NumericalFlat(), Ex_from_string("\\prod{#}", false, k), 0);
		auto wi2 = new WeightInherit();
		wi2->combination_type = WeightInherit::multiplicative;
		auto wa2 = Ex_from_string("label=all, type=multiplicative", false, k);
		k->inject_property(wi2, Ex_from_string("\\prod{#}", false, k), wa2);

		k->inject_property(new IndexInherit(), Ex_from_string("\\frac{#}", false, k), 0);
		k->inject_property(new DependsInherit(), Ex_from_string("\\frac{#}", false, k), 0);

		k->inject_property(new Distributable(), Ex_from_string("\\wedge{#}", false, k), 0);
		k->inject_property(new IndexInherit(), Ex_from_string("\\wedge{#}", false, k), 0);

		k->inject_property(new DependsInherit(), Ex_from_string("\\wedge{#}", false, k), 0);
		k->inject_property(new NumericalFlat(), Ex_from_string("\\wedge{#}", false, k), 0);
		auto wi4 = new WeightInherit();
		wi4->combination_type = WeightInherit::multiplicative;
		auto wa4 = Ex_from_string("label=all, type=multiplicative", false, k);
		k->inject_property(wi4, Ex_from_string("\\wedge{#}", false, k), wa4);

		k->inject_property(new IndexInherit(), Ex_from_string("\\sum{#}", false, k), 0);
		k->inject_property(new CommutingAsSum(), Ex_from_string("\\sum{#}", false, k), 0);
		k->inject_property(new DependsInherit(), Ex_from_string("\\sum{#}", false, k), 0);
		auto wi = new WeightInherit();
		auto wa = Ex_from_string("label=all, type=additive", false, k);
		k->inject_property(wi, Ex_from_string("\\sum{#}", false, k), wa);

		auto d = new Derivative();
		d->hidden(true);
		k->inject_property(d, Ex_from_string("\\cdbDerivative{#}", false, k), 0);

		k->inject_property(new Derivative(), Ex_from_string("\\commutator{#}", false, k), 0);
		k->inject_property(new IndexInherit(), Ex_from_string("\\commutator{#}", false, k), 0);

		k->inject_property(new Derivative(), Ex_from_string("\\anticommutator{#}", false, k), 0);
		k->inject_property(new IndexInherit(), Ex_from_string("\\anticommutator{#}", false, k), 0);

		k->inject_property(new Distributable(), Ex_from_string("\\indexbracket{#}", false, k), 0);
		k->inject_property(new IndexInherit(), Ex_from_string("\\indexbracket{#}", false, k), 0);

		k->inject_property(new DependsInherit(), Ex_from_string("\\pow{#}", false, k), 0);
		auto wi3 = new WeightInherit();
		auto wa3 = Ex_from_string("label=all, type=power", false, k);
		k->inject_property(wi3, Ex_from_string("\\pow{#}", false, k), wa3);

		k->inject_property(new NumericalFlat(), Ex_from_string("\\int{#}", false, k), 0);
		k->inject_property(new IndexInherit(), Ex_from_string("\\int{#}", false, k), 0);

		// Hidden nodes.
		k->inject_property(new Accent(), Ex_from_string("\\ldots{#}", false, k), 0);

		// Accents, necessary for proper display.
		k->inject_property(new Accent(), Ex_from_string("\\hat{#}", false, k), 0);
		k->inject_property(new Accent(), Ex_from_string("\\bar{#}", false, k), 0);
		k->inject_property(new Accent(), Ex_from_string("\\overline{#}", false, k), 0);
		k->inject_property(new Accent(), Ex_from_string("\\tilde{#}", false, k), 0);

		post_process_enabled = true;
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

		pybind11::class_<Kernel>(m, "Kernel", pybind11::dynamic_attr())
		.def(pybind11::init<>())
		.def_readonly("scalar_backend", &Kernel::scalar_backend);

		Kernel* kernel = create_scope();
		m.attr("__cdbkernel__") = pybind11::cast(kernel);

		m.def("kernel", [](pybind11::kwargs dict) {
			Kernel *k = get_kernel_from_scope();
			for (auto& item : dict) {
				std::string key = item.first.cast<std::string>();
				std::string val = item.second.cast<std::string>();
				if (key == "scalar_backend") {
					if (val == "sympy")            k->scalar_backend = Kernel::scalar_backend_t::sympy;
					else if (val == "mathematica") k->scalar_backend = Kernel::scalar_backend_t::mathematica;
					else throw ArgumentException("scalar_backend must be 'sympy' or 'mathematica'.");
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

		}

	}