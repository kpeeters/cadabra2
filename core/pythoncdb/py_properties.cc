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
#include "properties/Diagonal.hh"
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
#include "properties/TableauInherit.hh"
#include "properties/TableauSymmetry.hh"
#include "properties/Trace.hh"
#include "properties/Traceless.hh"
#include "properties/Vielbein.hh"
#include "properties/Weight.hh"
#include "properties/WeightInherit.hh"
#include "properties/WeylTensor.hh"


namespace cadabra {
	namespace py = pybind11;

	BoundPropertyBase::BoundPropertyBase()
		: prop(nullptr)
		, for_obj(nullptr)
	{

	}

	BoundPropertyBase::BoundPropertyBase(const property* prop, Ex_ptr for_obj)
		: prop(prop)
		, for_obj(for_obj)
	{

	}

	BoundPropertyBase::~BoundPropertyBase()
	{

	}

	std::string BoundPropertyBase::str_() const
	{
		std::ostringstream str;
		str << "Attached property ";
//		std::cerr << "going to print" << std::endl;
		prop->latex(str); // FIXME: this should call 'str' on the property, which does not exist yet
		str << " to " + Ex_as_str(for_obj) + ".";
		return str.str();
	}

	std::string BoundPropertyBase::latex_() const
	{
		std::ostringstream str;

		//	HERE: this text should go away, property should just print itself in a python form,
		//   the decorating text should be printed in a separate place.

		str << "\\text{Attached property ";
		prop->latex(str);
		std::string bare = Ex_as_latex(for_obj);

		if (dynamic_cast<const LaTeXForm*>(prop)) {
			bare = std::regex_replace(bare, std::regex(R"(\\)"), "$\\backslash{}$}");
			bare = std::regex_replace(bare, std::regex(R"(#)"), "\\#");
			str << " to {\\tt " + bare + "}.";
		}
		else {
			str << " to~}" + bare + ".";
		}

		return str.str();
	}

	std::string BoundPropertyBase::repr_() const
	{
		// FIXME: this needs work, it does not output things which can be fed back into python.
		return "Property::repr: " + prop->name();
	}

	Kernel& BoundPropertyBase::get_kernel()
	{
		return *get_kernel_from_scope();
	}

	Properties& BoundPropertyBase::get_props()
	{
		return get_kernel_from_scope()->properties;
	}

	Ex& BoundPropertyBase::get_ex() const
	{
		return *for_obj;
	}

	Ex::iterator BoundPropertyBase::get_it() const
	{
		return for_obj->begin();
	}


	template <typename PropT, typename... ParentTs>
	BoundProperty<PropT, ParentTs...>::BoundProperty()
		: BoundPropertyBase()
	{

	}

	template <typename PropT, typename... ParentTs>
	BoundProperty<PropT, ParentTs...>::BoundProperty(const PropT* prop, Ex_ptr for_obj)
		: BoundPropertyBase(prop, for_obj)
	{

	}

	template <typename PropT, typename... ParentTs>
	BoundProperty<PropT, ParentTs...>::BoundProperty(Ex_ptr ex, Ex_ptr param)
		: BoundPropertyBase(nullptr, ex)
	{
		auto new_prop = new cpp_type();
		get_kernel_from_scope()->inject_property(new_prop, ex, param);
		BoundPropertyBase::prop = new_prop;
	}


	template <typename PropT, typename... ParentTs>
	std::shared_ptr<BoundProperty<PropT, ParentTs...>> BoundProperty<PropT, ParentTs...>::get_from_kernel(Ex::iterator it, const std::string& label, bool ignore_parent_rel)
		
	{
		int tmp;
		auto res = get_kernel_from_scope()->properties.get_with_pattern<PropT>(
			it, tmp, label, false, ignore_parent_rel);

		if (res.first) {
			return std::make_shared<BoundProperty<PropT, ParentTs...>>(
				res.first,
				res.second ? std::make_shared<Ex>(res.second->obj) : nullptr);
		}
		else {
			return nullptr;
		}
	}


	template <typename PropT, typename... ParentTs>
	const PropT* BoundProperty<PropT, ParentTs...>::get_prop() const
	{
	return dynamic_cast<const PropT*>(BoundPropertyBase::prop);
	}

	template <typename PropT, typename... ParentTs>
	std::string BoundProperty<PropT, ParentTs...>::str_() const
		{
		return BoundPropertyBase::str_();
		}
	
	template <typename PropT, typename... ParentTs>
	std::string BoundProperty<PropT, ParentTs...>::latex_() const
		{
		return BoundPropertyBase::latex_();
		}
	
	template <typename PropT, typename... ParentTs>
	std::string BoundProperty<PropT, ParentTs...>::repr_() const
		{
		return BoundPropertyBase::str_();
		}
	
	template <typename BoundPropT>
	typename BoundPropT::py_type def_abstract_prop(pybind11::module& m, const std::string& name)
	{
		using base_type = BoundPropT;
//		using cpp_type = typename base_type::cpp_type;
		using py_type = typename base_type::py_type;

		return py_type(m, name.c_str(), py::multiple_inheritance())
			.def_static("get", [](Ex_ptr ex, const std::string& label, bool ipr) { return base_type::get_from_kernel(ex->begin(), label, ipr); }, py::arg("ex"), py::arg("label") = "", py::arg("ignore_parent_rel") = false)
			.def_static("get", [](ExNode node, const std::string& label, bool ipr) { return base_type::get_from_kernel(node.it, label, ipr); }, py::arg("exnode"), py::arg("label") = "", py::arg("ignore_parent_rel") = false)
			.def("__str__", &BoundPropT::str_)
			.def("__repr__", &BoundPropT::repr_)
			.def("_latex_", &BoundPropT::latex_);
	}

	template <typename BoundPropT>
	typename BoundPropT::py_type def_prop(pybind11::module& m)
	{
		using base_type = BoundPropT;
		using cpp_type = typename base_type::cpp_type;
		using py_type = typename base_type::py_type;

		return py_type(m, std::make_shared<cpp_type>()->name().c_str(), py::multiple_inheritance())
			.def(py::init<Ex_ptr, Ex_ptr>(), py::arg("ex"), py::arg("param")=Ex{})
			.def_static("get", [](Ex_ptr ex, const std::string& label, bool ipr) { return base_type::get_from_kernel(ex->begin(), label, ipr); }, py::arg("ex"), py::arg("label") = "", py::arg("ignore_parent_rel") = false)
			.def_static("get", [](ExNode node, const std::string& label, bool ipr) { return base_type::get_from_kernel(node.it, label, ipr); }, py::arg("exnode"), py::arg("label") = "", py::arg("ignore_parent_rel") = false)
			.def("__str__", &BoundPropT::str_)
			.def("__repr__", &BoundPropT::repr_)
			.def("_latex_", &BoundPropT::latex_);
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
		
		m.def("properties", &list_properties);

		py::class_<BoundPropertyBase, std::shared_ptr<BoundPropertyBase>>(m, "Property")
			.def_property_readonly("for_obj", &BoundPropertyBase::get_ex);


		// Abstract base types = these are visible in Python but cannot be injected into the Kernel
		using Py_list_property = BoundProperty<list_property, BoundPropertyBase>;
		using Py_labelled_property = BoundProperty<labelled_property, BoundPropertyBase>;
		using Py_CommutingBehaviour = BoundProperty<CommutingBehaviour, BoundPropertyBase>;
		using Py_SelfCommutingBehaviour = BoundProperty<SelfCommutingBehaviour, BoundPropertyBase>;
		using Py_TableauBase = BoundProperty<TableauBase, BoundPropertyBase>;
		using Py_DependsBase = BoundProperty<DependsBase, BoundPropertyBase>;
		using Py_WeightBase = BoundProperty<WeightBase, Py_labelled_property>;
		using Py_DifferentialFormBase = BoundProperty<DifferentialFormBase, BoundPropertyBase>;

		def_abstract_prop<Py_list_property>(m, "list_property");
		def_abstract_prop<Py_labelled_property>(m, "labelled_property")
			.def_property_readonly("label", [](const Py_labelled_property & p) { return p.get_prop()->label; });
		def_abstract_prop<Py_CommutingBehaviour>(m, "CommutingBehaviour")
			.def("sign", [](const Py_CommutingBehaviour & p) { return p.get_prop()->sign(); });
		def_abstract_prop<Py_SelfCommutingBehaviour>(m, "SelfCommutingBehaviour")
			.def("sign", [](const Py_SelfCommutingBehaviour & p) { return p.get_prop()->sign(); });
		def_abstract_prop<Py_TableauBase>(m, "TableauBase")
			.def("size", [](const Py_TableauBase & p) { return p.get_prop()->size(p.get_props(), p.get_ex(), p.get_it()); })
			.def("get_tab", [](const Py_TableauBase & p, unsigned int num) { return p.get_prop()->get_tab(p.get_props(), p.get_ex(), p.get_it(), num); })
			.def("only_column_exchange", [](const Py_TableauBase & p) { return p.get_prop()->only_column_exchange(); })
			.def("get_indexgroup", [](const Py_TableauBase & p, int group) { return p.get_prop()->get_indexgroup(p.get_props(), p.get_ex(), p.get_it(), group); })
			.def("is_simple_symmetry", [](const Py_TableauBase & p) { return p.get_prop()->is_simple_symmetry(p.get_props(), p.get_ex(), p.get_it()); });
		def_abstract_prop<Py_DependsBase>(m, "DependsBase")
			.def("dependencies", [](const Py_DependsBase & p) { return p.get_prop()->dependencies(p.get_kernel(), p.get_it()); });
		def_abstract_prop<Py_WeightBase>(m, "WeightBase")
			.def("value", [](const Py_WeightBase & p, const std::string& forcedLabel) { return p.get_prop()->value(p.get_kernel(), p.get_it(), forcedLabel); });
		def_abstract_prop<Py_DifferentialFormBase>(m, "DifferentialFormBase")
			.def("degree", [](const Py_DifferentialFormBase & p) { return p.get_prop()->degree(p.get_props(), p.get_it()); });


		// Base types - inherit only from BoundPropertyBase, list_property or labelled_property
		using Py_IndexInherit = BoundProperty<IndexInherit, BoundPropertyBase>;
		using Py_NumericalFlat = BoundProperty<NumericalFlat, BoundPropertyBase>;
		using Py_Traceless = BoundProperty<Traceless, BoundPropertyBase>;
		using Py_Coordinate = BoundProperty<Coordinate, BoundPropertyBase>;
		using Py_CommutingAsProduct = BoundProperty<CommutingAsProduct, BoundPropertyBase>;
		using Py_CommutingAsSum = BoundProperty<CommutingAsSum, BoundPropertyBase>;
		using Py_Distributable = BoundProperty<Distributable, BoundPropertyBase>;
		using Py_Determinant = BoundProperty<Determinant, BoundPropertyBase>;
		using Py_FilledTableau = BoundProperty<FilledTableau, BoundPropertyBase>;
		using Py_ImplicitIndex = BoundProperty<ImplicitIndex, BoundPropertyBase>;
		using Py_ImaginaryI = BoundProperty<ImaginaryI, BoundPropertyBase>;
		using Py_Indices = BoundProperty<Indices, Py_list_property>;
		using Py_Integer = BoundProperty<Integer, BoundPropertyBase>;
		using Py_LaTeXForm = BoundProperty<LaTeXForm, BoundPropertyBase>;
		using Py_SortOrder = BoundProperty<SortOrder, Py_list_property>;
		using Py_Symbol = BoundProperty<Symbol, BoundPropertyBase>;
		using Py_Tableau = BoundProperty<Tableau, BoundPropertyBase>;
		using Py_TableauInherit = BoundProperty<TableauInherit, BoundPropertyBase>;
		using Py_Vielbein = BoundProperty<Vielbein, BoundPropertyBase>;
		using Py_InverseVielbein = BoundProperty<InverseVielbein, BoundPropertyBase>;

		def_prop<Py_IndexInherit>(m);
		def_prop<Py_NumericalFlat>(m);
		def_prop<Py_Traceless>(m);
		def_prop<Py_Coordinate>(m);
		def_prop<Py_CommutingAsProduct>(m);
		def_prop<Py_CommutingAsSum>(m);
		def_prop<Py_Distributable>(m);
		def_prop<Py_Determinant>(m)
			.def_property_readonly("obj", [](const Py_Determinant & p) { return p.get_prop()->obj; });
		def_prop<Py_FilledTableau>(m)
			.def_property_readonly("dimension", [](const Py_FilledTableau & p) { return p.get_prop()->dimension; });
		def_prop<Py_ImplicitIndex>(m)
			.def_property_readonly("explicit_form", [](const Py_ImplicitIndex & p) { return p.get_prop()->explicit_form; });
		def_prop<Py_ImaginaryI>(m);
		auto py_indices = def_prop<Py_Indices>(m)
			.def_property_readonly("set_name", [](const Py_Indices & p) { return p.get_prop()->set_name; })
			.def_property_readonly("parent_name", [](const Py_Indices & p) { return p.get_prop()->parent_name; })
			.def_property_readonly("values", [](const Py_Indices & p) { return p.get_prop()->values; });
		def_prop<Py_Integer>(m)
			.def_property_readonly("from", [](const Py_Integer & p) { return p.get_prop()->from; })
			.def_property_readonly("to", [](const Py_Integer & p) { return p.get_prop()->to; })
			.def_property_readonly("difference", [](const Py_Integer & p) { return p.get_prop()->difference; });
		def_prop<Py_LaTeXForm>(m)
			.def("latex_form", [](const Py_LaTeXForm & p) { return p.get_prop()->latex_form(); });
		def_prop<Py_SortOrder>(m);
		def_prop<Py_Symbol>(m);
		def_prop<Py_Tableau>(m)
			.def_property_readonly("dimension", [](const Py_Tableau & p) { return p.get_prop()->dimension; });
		def_prop<Py_TableauInherit>(m);
		def_prop<Py_Vielbein>(m);
		def_prop<Py_InverseVielbein>(m);

		py::enum_<Indices::position_t>(py_indices, "position_t")
			.value("free", Indices::free)
			.value("fixed", Indices::fixed)
			.value("independent", Indices::independent)
			.export_values();


		// Derived types
		using Py_Accent = BoundProperty<Accent, Py_IndexInherit, Py_NumericalFlat>;
		using Py_AntiCommuting = BoundProperty<AntiCommuting, Py_CommutingBehaviour>;
		using Py_Commuting = BoundProperty<Commuting, Py_CommutingBehaviour>;
		using Py_AntiSymmetric = BoundProperty<AntiSymmetric, Py_TableauBase, Py_Traceless>;
		using Py_DAntiSymmetric = BoundProperty<DAntiSymmetric, Py_TableauBase>;
		using Py_Depends = BoundProperty<Depends, Py_DependsBase>;
		using Py_Derivative = BoundProperty<Derivative, Py_IndexInherit, Py_CommutingAsProduct, Py_NumericalFlat, Py_TableauBase, Py_Distributable, Py_WeightBase>;
		using Py_Symmetric = BoundProperty<Symmetric, Py_TableauBase>;
		using Py_DifferentialForm = BoundProperty<DifferentialForm, Py_IndexInherit, Py_DifferentialFormBase>;
		using Py_DiracBar = BoundProperty<DiracBar, Py_Accent, Py_Distributable>;
		using Py_EpsilonTensor = BoundProperty<EpsilonTensor, Py_AntiSymmetric>;
		using Py_ExteriorDerivative = BoundProperty<ExteriorDerivative, Py_Derivative, Py_DifferentialFormBase>;
		using Py_Matrix = BoundProperty<Matrix, Py_ImplicitIndex>;
		using Py_GammaMatrix = BoundProperty<GammaMatrix, Py_AntiSymmetric, Py_Matrix>;
		using Py_TableauSymmetry = BoundProperty<TableauSymmetry, Py_TableauBase>;
		using Py_InverseMetric = BoundProperty<InverseMetric, Py_TableauSymmetry>;
		using Py_KroneckerDelta = BoundProperty<KroneckerDelta, Py_TableauBase>;
		using Py_Metric = BoundProperty<Metric, Py_Symmetric>;
		using Py_NonCommuting = BoundProperty<NonCommuting, Py_CommutingBehaviour>;
		using Py_PartialDerivative = BoundProperty<PartialDerivative, Py_Derivative>;
		using Py_RiemannTensor = BoundProperty<RiemannTensor, Py_TableauSymmetry>;
		using Py_SatisfiesBianchi = BoundProperty<SatisfiesBianchi, Py_TableauBase>;
		using Py_SelfAntiCommuting = BoundProperty<SelfAntiCommuting, Py_SelfCommutingBehaviour>;
		using Py_SelfCommuting = BoundProperty<SelfCommuting, Py_SelfCommutingBehaviour>;
		using Py_SelfNonCommuting = BoundProperty<SelfNonCommuting, Py_SelfCommutingBehaviour>;
		using Py_Spinor = BoundProperty<Spinor, Py_ImplicitIndex>;
		using Py_Trace = BoundProperty<Trace, Py_Distributable>;
		using Py_Weight = BoundProperty<Weight, Py_WeightBase>;
		using Py_WeightInherit = BoundProperty<WeightInherit, Py_WeightBase>;
		using Py_WeylTensor = BoundProperty<WeylTensor, Py_TableauSymmetry, Py_Traceless>;

		def_prop<Py_Accent>(m);
		def_prop<Py_AntiCommuting>(m);
		def_prop<Py_Commuting>(m);
		def_prop<Py_AntiSymmetric>(m);
		def_prop<Py_DAntiSymmetric>(m);
		def_prop<Py_Depends>(m);
		def_prop<Py_Derivative>(m);
		def_prop<Py_Symmetric>(m);
		def_prop<Py_DifferentialForm>(m);
		def_prop<Py_DiracBar>(m);
		def_prop<Py_EpsilonTensor>(m)
			.def_property_readonly("metric", [](const Py_EpsilonTensor& p) { return p.get_prop()->metric; })
			.def_property_readonly("krdelta", [](const Py_EpsilonTensor& p) { return p.get_prop()->krdelta; });
		def_prop<Py_ExteriorDerivative>(m);
		def_prop<Py_Matrix>(m);
		def_prop<Py_GammaMatrix>(m)
			.def_property_readonly("metric", [](const Py_GammaMatrix& p) { return p.get_prop()->metric; });
		def_prop<Py_TableauSymmetry>(m);
		def_prop<Py_InverseMetric>(m)
			.def_property_readonly("signature", [](const Py_InverseMetric& p) { return p.get_prop()->signature; });
		def_prop<Py_KroneckerDelta>(m);
		def_prop<Py_Metric>(m)
			.def_property_readonly("signature", [](const Py_Metric& p) { return p.get_prop()->signature; });;
		def_prop<Py_NonCommuting>(m);
		def_prop<Py_PartialDerivative>(m);
		def_prop<Py_RiemannTensor>(m);
		def_prop<Py_SatisfiesBianchi>(m);
		def_prop<Py_SelfAntiCommuting>(m);
		def_prop<Py_SelfCommuting>(m);
		def_prop<Py_SelfNonCommuting>(m);

		using Py_Diagonal = BoundProperty<Diagonal, Py_Symmetric>;
		def_prop<Py_Diagonal>(m);


		auto py_spinor = def_prop<Py_Spinor>(m)
			.def_property_readonly("dimension", [](const Py_Spinor& p) { return p.get_prop()->dimension; })
			.def_property_readonly("weyl", [](const Py_Spinor& p) { return p.get_prop()->weyl; })
			.def_property_readonly("chirality", [](const Py_Spinor& p) { return p.get_prop()->chirality; })
			.def_property_readonly("majorana", [](const Py_Spinor& p) { return p.get_prop()->majorana; });
		def_prop<Py_Trace>(m)
			.def_property_readonly("obj", [](const Py_Trace& p) { return p.get_prop()->obj; })
			.def_property_readonly("index_set_name", [](const Py_Trace& p) { return p.get_prop()->index_set_name; });
		def_prop<Py_Weight>(m);
		auto py_weight_inherit = def_prop<Py_WeightInherit>(m)
			.def("combination_type", [](const Py_WeightInherit& p) { return p.get_prop()->combination_type; });
		def_prop<Py_WeylTensor>(m);

		py::enum_<Spinor::Chirality>(py_spinor, "Chirality")
			.value("positive", Spinor::positive)
			.value("negative", Spinor::negative)
			.export_values();

		py::enum_<WeightInherit::CombinationType>(py_weight_inherit, "CombinationType")
			.value("multiplicative", WeightInherit::multiplicative)
			.value("additive", WeightInherit::additive)
			.value("power", WeightInherit::power)
			.export_values();


		}
	}
