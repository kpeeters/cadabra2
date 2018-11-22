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
#include "properties/Traceless.hh"
#include "properties/Vielbein.hh"
#include "properties/Weight.hh"
#include "properties/WeightInherit.hh"
#include "properties/WeylTensor.hh"


namespace cadabra
	{
	namespace py = pybind11;

	template<>
	std::string Property<LaTeXForm>::latex_() const
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
		py::class_<BaseProperty, std::shared_ptr<BaseProperty>>(m, "Property");

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
		def_prop<Traceless>(m);
		def_prop<Vielbein>(m);
		def_prop<InverseVielbein>(m);
		def_prop<Weight>(m);
		def_prop<WeightInherit>(m);
		def_prop<WeylTensor>(m);

		}
	}