#include "py_properties.hh"
#include "py_kernel.hh"

#include "properties/Accent.hh"
#include "properties/AntiCommuting.hh"
#include "properties/AntiSymmetric.hh"
#include "properties/Commuting.hh"
#include "properties/Coordinate.hh"
#include "properties/Depends.hh"
#include "properties/DependsInherit.hh"
#include "properties/Derivative.hh"
#include "properties/Determinant.hh"
#include "properties/DifferentialForm.hh"
#include "properties/DiracBar.hh"
#include "properties/GammaMatrix.hh"
#include "properties/CommutingAsProduct.hh"
#include "properties/CommutingAsSum.hh"
#include "properties/DAntiSymmetric.hh"
#include "properties/Diagonal.hh"
#include "properties/Distributable.hh"
#include "properties/EpsilonTensor.hh"
#include "properties/ExteriorDerivative.hh"
#include "properties/FilledTableau.hh"
#include "properties/ImaginaryI.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/Indices.hh"
#include "properties/IndexInherit.hh"
#include "properties/Integer.hh"
#include "properties/InverseMetric.hh"
#include "properties/KroneckerDelta.hh"
#include "properties/LaTeXForm.hh"
#include "properties/Matrix.hh"
#include "properties/Metric.hh"
#include "properties/NonCommuting.hh"
#include "properties/NumericalFlat.hh"
#include "properties/PartialDerivative.hh"
#include "properties/RiemannTensor.hh"
#include "properties/SatisfiesBianchi.hh"
#include "properties/SelfAntiCommuting.hh"
#include "properties/SelfCommuting.hh"
#include "properties/SelfNonCommuting.hh"
#include "properties/SortOrder.hh"
#include "properties/Spinor.hh"
#include "properties/Symbol.hh"
#include "properties/Symmetric.hh"
#include "properties/Tableau.hh"
#include "properties/TableauSymmetry.hh"
#include "properties/Trace.hh"
#include "properties/Traceless.hh"
#include "properties/Vielbein.hh"
#include "properties/Weight.hh"
#include "properties/WeightInherit.hh"
#include "properties/WeylTensor.hh"


namespace cadabra {
	namespace py = pybind11;

	template <typename PropT, typename... ParentTs>
	BoundProperty<PropT, ParentTs...>::BoundProperty()
		: prop(nullptr)
		, for_obj(nullptr)
	{

	}

	template <typename PropT, typename... ParentTs>
	BoundProperty<PropT, ParentTs...>::BoundProperty(Ex_ptr ex, Ex_ptr param)
	{
		Kernel* kernel = get_kernel_from_scope();
		auto prop_ = new PropT(); // we keep a pointer, but the kernel owns it.
		//	std::cerr << "Declaring property " << prop->name() << " in kernel " << kernel << std::endl;
		kernel->inject_property(prop_, ex, param);

		prop = prop_;
		for_obj = ex;
	}

	template <typename PropT, typename... ParentTs>
	std::string BoundProperty<PropT, ParentTs...>::str_() const
	{
		std::ostringstream str;
		str << "Attached property ";
		prop->latex(str); // FIXME: this should call 'str' on the property, which does not exist yet
		str << " to " + Ex_as_str(for_obj) + ".";
		return str.str();
	}

	template <typename PropT, typename... ParentTs>
	std::string BoundProperty<PropT, ParentTs...>::latex_() const
	{
		std::ostringstream str;

		//	HERE: this text should go away, property should just print itself in a python form,
		//   the decorating text should be printed in a separate place.

		str << "\\text{Attached property ";
		prop->latex(str);
		std::string bare = Ex_as_latex(for_obj);
		str << " to~}" + bare + ".";
		return str.str();
	}

	template <>
	std::string BoundProperty<LaTeXForm>::latex_() const;

	template <typename PropT, typename... ParentTs>
	std::string BoundProperty<PropT, ParentTs...>::repr_() const
	{
		// FIXME: this needs work, it does not output things which can be fed back into python.
		return "Property::repr: " + prop->name();
	}


	template<>
	std::string BoundProperty<LaTeXForm>::latex_() const
		{
		std::ostringstream str;
		str << "\\text{Attached property ";
		prop->latex(str);
		std::string bare = Ex_as_str(for_obj);
		bare = std::regex_replace(bare, std::regex(R"(\\)"), "$\\backslash{}$}");
		bare = std::regex_replace(bare, std::regex(R"(#)"), "\\#");
		str << " to {\\tt " + bare + "}.";
		return str.str();
		}

	template <typename PropT, typename... ParentTs>
	BoundProperty<PropT, ParentTs...> BoundProperty<PropT, ParentTs...>::get_from_it(Ex::iterator it, bool ignore_parent_rel)
	{
		int tmp;
		auto res = get_kernel_from_scope()->properties.get_with_pattern<PropT>(
			it,
			tmp,
			false,
			ignore_parent_rel
			);

		BoundProperty ret;
		ret.prop = res.first;
		if (res.second)
			ret.for_obj = std::make_shared<Ex>(res.second->obj);
		return ret;
	}

	template <typename PropT, typename... ParentTs>
	BoundProperty<PropT, ParentTs...> BoundProperty<PropT, ParentTs...>::get_from_ex(Ex_ptr ex, bool ignore_parent_rel)
	{
		return get_from_it(ex->begin(), ignore_parent_rel);
	}

	template <typename PropT, typename... ParentTs>
	BoundProperty<PropT, ParentTs...> BoundProperty<PropT, ParentTs...>::get_from_exnode(ExNode exnode, bool ignore_parent_rel)
	{
		return get_from_it(exnode.it, ignore_parent_rel);
	}

	template <typename PropT, typename... ParentTs>
	PyProperty<PropT, ParentTs...> def_prop(pybind11::module& m, const char* docstring)
	{
		using namespace pybind11;

		return PyProperty<PropT, ParentTs...>(m, std::make_shared<PropT>()->name().c_str())
			.def(
				init<std::shared_ptr<Ex>, std::shared_ptr<Ex>>(),
				arg("ex"),
				arg("param"),	
				docstring
			)
			.def("__str__", &BoundProperty<PropT, ParentTs...>::str_)
			.def("__repr__", &BoundProperty<PropT, ParentTs...>::repr_)
			.def("_latex_", &BoundProperty<PropT, ParentTs...>::latex_)
			.def_static("get", &BoundProperty<PropT, ParentTs...>::get_from_ex, py::arg("ex"), py::arg("ignore_parent_rel") = false)
			.def_static("get", &BoundProperty<PropT, ParentTs...>::get_from_exnode, py::arg("exnode"), py::arg("ignore_parent_rel") = false);
	}



	pybind11::list list_properties()
		{
		//	std::cout << "listing properties" << std::endl;
		Kernel *kernel = get_kernel_from_scope();
		Properties& props = kernel->properties;

		pybind11::list ret;
		std::string res;
		bool multi = false;
		for (auto it = props.pats.begin(); it != props.pats.end(); ++it) {
			if (it->first->hidden()) continue;

			// print the property name if we are at the end or if the next entry is for
			// a different property.
			decltype(it) nxt = it;
			++nxt;
			if (res == "" && (nxt != props.pats.end() && it->first == nxt->first)) {
				res += "{";
				multi = true;
				}


			//std::cerr << Ex(it->second->obj) << std::endl;
			//		DisplayTeX dt(*get_kernel_from_scope(), it->second->obj);
			std::ostringstream str;
			// std::cerr << "displaying" << std::endl;
			//		dt.output(str);

			str << it->second->obj;

			// std::cerr << "displayed " << str.str() << std::endl;
			res += str.str();

			if (nxt == props.pats.end() || it->first != nxt->first) {
				if (multi)
					res += "}";
				multi = false;
				res += "::";
				res += (*it).first->name();
				ret.append(res);
				res = "";
				}
			else {
				res += ", ";
				}
			}

		return ret;
		}


	void init_properties(py::module& m)
		{
		py::class_<BoundPropertyBase, std::shared_ptr<BoundPropertyBase>>(m, "Property");

		m.def("properties", &list_properties);


		def_prop<Accent>(m);
		def_prop<AntiCommuting>(m);
		def_prop<AntiSymmetric>(m);
		def_prop<Coordinate>(m);
		def_prop<Commuting>(m);
		def_prop<CommutingAsProduct>(m);
		def_prop<CommutingAsSum>(m);
		def_prop<DAntiSymmetric>(m);
		def_prop<Depends>(m);
		def_prop<Derivative>(m);
		def_prop<Determinant>(m);
		def_prop<Diagonal>(m);
		def_prop<DifferentialForm>(m);
		def_prop<Distributable>(m);
		def_prop<DiracBar>(m);
		def_prop<EpsilonTensor>(m);
		def_prop<ExteriorDerivative>(m);
		def_prop<FilledTableau>(m);
		def_prop<GammaMatrix>(m);
		def_prop<ImaginaryI>(m);
		def_prop<ImplicitIndex>(m);
		def_prop<IndexInherit>(m);
		def_prop<Indices>(m);
		def_prop<Integer>(m);
		def_prop<InverseMetric>(m);
		def_prop<KroneckerDelta>(m);
		def_prop<LaTeXForm>(m);
		def_prop<Matrix>(m);
		def_prop<Metric>(m);
		def_prop<NonCommuting>(m);
		def_prop<NumericalFlat>(m);
		def_prop<PartialDerivative>(m);
		def_prop<RiemannTensor>(m);
		def_prop<SatisfiesBianchi>(m);
		def_prop<SelfAntiCommuting>(m);
		def_prop<SelfCommuting>(m);
		def_prop<SelfNonCommuting>(m);
		def_prop<SortOrder>(m);
		def_prop<Spinor>(m);
		def_prop<Symbol>(m);
		def_prop<Symmetric>(m);
		def_prop<Tableau>(m);
		def_prop<TableauSymmetry>(m);
		def_prop<Trace>(m);
		def_prop<Traceless>(m);
		def_prop<Vielbein>(m);
		def_prop<InverseVielbein>(m);
		def_prop<Weight>(m);
		def_prop<WeightInherit>(m);
		def_prop<WeylTensor>(m);

		}
	}
