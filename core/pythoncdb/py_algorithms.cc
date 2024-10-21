#include "py_algorithms.hh"

#include <pybind11/stl.h>

#include "../Algorithm.hh"
#include "../NEvaluator.hh"

#include "../algorithms/canonicalise.hh"
#include "../algorithms/collect_components.hh"
#include "../algorithms/collect_factors.hh"
#include "../algorithms/collect_terms.hh"
#include "../algorithms/combine.hh"
#include "../algorithms/complete.hh"
#include "../algorithms/decompose.hh"
#include "../algorithms/decompose_product.hh"
#include "../algorithms/distribute.hh"
#include "../algorithms/drop_weight.hh"
#include "../algorithms/einsteinify.hh"
#include "../algorithms/eliminate_kronecker.hh"
#include "../algorithms/eliminate_metric.hh"
#include "../algorithms/eliminate_vielbein.hh"
#include "../algorithms/epsilon_to_delta.hh"
#include "../algorithms/evaluate.hh"
#include "../algorithms/expand.hh"
#include "../algorithms/expand_delta.hh"
#include "../algorithms/expand_diracbar.hh"
#include "../algorithms/expand_dummies.hh"
#include "../algorithms/expand_power.hh"
#include "../algorithms/explicit_indices.hh"
#include "../algorithms/factor_in.hh"
#include "../algorithms/factor_out.hh"
#include "../algorithms/fierz.hh"
#include "../algorithms/flatten_sum.hh"
#include "../algorithms/indexsort.hh"
#include "../algorithms/integrate_by_parts.hh"
#include "../algorithms/join_gamma.hh"
#include "../algorithms/keep_terms.hh"
#include "../algorithms/lower_free_indices.hh"
#include "../algorithms/lr_tensor.hh"
#ifdef MATHEMATICA_FOUND
#include "../algorithms/map_mma.hh"
#endif
#include "../algorithms/map_sympy.hh"
#include "../algorithms/meld.hh"
#include "../algorithms/nevaluate.hh"
#include "../algorithms/order.hh"
#include "../algorithms/product_rule.hh"
#include "../algorithms/reduce_delta.hh"
#include "../algorithms/rename_dummies.hh"
#include "../algorithms/replace_match.hh"
#include "../algorithms/rewrite_indices.hh"
#include "../algorithms/simplify.hh"
#include "../algorithms/sort_product.hh"
#include "../algorithms/sort_spinors.hh"
#include "../algorithms/sort_sum.hh"
#include "../algorithms/split_gamma.hh"
#include "../algorithms/split_index.hh"
#include "../algorithms/substitute.hh"
#include "../algorithms/sym.hh"
#include "../algorithms/tab_dimension.hh"
#include "../algorithms/take_match.hh"
#include "../algorithms/unwrap.hh"
#include "../algorithms/unzoom.hh"
#include "../algorithms/untrace.hh"
#include "../algorithms/vary.hh"
#include "../algorithms/young_project.hh"
#include "../algorithms/young_project_product.hh"
#include "../algorithms/young_project_tensor.hh"
#include "../algorithms/zoom.hh"

namespace cadabra {
	namespace py = pybind11;

	void init_algorithms(py::module& m)
		{
		pybind11::enum_<Algorithm::result_t>(m, "result_t")
		.value("checkpointed", Algorithm::result_t::l_checkpointed)
		.value("changed", Algorithm::result_t::l_applied)
		.value("unchanged", Algorithm::result_t::l_no_action)
		.value("error", Algorithm::result_t::l_error)
		.export_values();

		def_algo<canonicalise>(m, "canonicalise", true, false, 0);
		def_algo<collect_components>(m, "collect_components", true, false, 0);
		def_algo<collect_factors>(m, "collect_factors", true, false, 0);
		def_algo<collect_terms>(m, "collect_terms", true, false, 0);
		def_algo<decompose_product>(m, "decompose_product", true, false, 0);
		def_algo<distribute>(m, "distribute", true, false, 0);
		def_algo<eliminate_kronecker>(m, "eliminate_kronecker", true, false, 0);
		def_algo<expand>(m, "expand", true, false, 0);
		def_algo<expand_delta>(m, "expand_delta", true, false, 0);
		def_algo<expand_diracbar>(m, "expand_diracbar", true, false, 0);
		def_algo<expand_dummies, const Ex*, bool>(m, "expand_dummies", true, false, 0, py::arg("components") = nullptr, py::arg("zero_missing_components") = true);
		def_algo<expand_power>(m, "expand_power", true, false, 0);
		def_algo<explicit_indices>(m, "explicit_indices", true, false, 0);
		def_algo<flatten_sum>(m, "flatten_sum", true, false, 0);
		def_algo<indexsort>(m, "indexsort", true, false, 0);
		def_algo<lr_tensor>(m, "lr_tensor", true, false, 0);
		def_algo<product_rule>(m, "product_rule", true, false, 0);
		def_algo<reduce_delta>(m, "reduce_delta", true, false, 0);
		def_algo<sort_product>(m, "sort_product", true, false, 0);
		def_algo<sort_spinors>(m, "sort_spinors", true, false, 0);
		def_algo<sort_sum>(m, "sort_sum", true, false, 0);
		def_algo<tabdimension>(m, "tab_dimension", true, false, 0);
		def_algo<young_project_product>(m, "young_project_product", true, false, 0);
		def_algo<combine, Ex>(m, "combine", true, false, 0, py::arg("trace_op") = Ex{});
		def_algo<complete, Ex>(m, "complete", false, false, 0, py::arg("add"));
		def_algo<decompose, Ex>(m, "decompose", false, false, 0, py::arg("basis"));
		def_algo<drop_weight, Ex>(m, "drop_weight", false, false, 0, py::arg("condition") = Ex{});
		def_algo<eliminate_metric, Ex,bool>(m, "eliminate_metric", true, false, 0, py::arg("preferred") = Ex{}, py::arg("redundant") = false);
		def_algo<eliminate_vielbein, Ex,bool>(m, "eliminate_vielbein", true, false, 0, py::arg("preferred") = Ex{},py::arg("redundant")= false);
		def_algo<keep_weight, Ex>(m, "keep_weight", false, false, 0, py::arg("condition"));
		def_algo<lower_free_indices, bool>(m, "lower_free_indices", true, false, 0, py::arg("lower") = true);
		def_algo<lower_free_indices, bool>(m, "raise_free_indices", true, false, 0, py::arg("lower") = false);
		def_algo<integrate_by_parts, Ex>(m, "integrate_by_parts", true, false, 0, py::arg("away_from"));
		def_algo<young_project_tensor, bool>(m, "young_project_tensor", true, false, 0, py::arg("modulo_monoterm") = false);
		def_algo<join_gamma, bool, bool>(m, "join_gamma", true, false, 0, py::arg("expand") = true, py::arg("use_gendelta") = false);
		def_algo<einsteinify, Ex>(m, "einsteinify", true, false, 0, py::arg("metric") = Ex{});
		def_algo<evaluate, Ex, bool, bool>(m, "evaluate", false, false, 0, py::arg("components") = Ex{}, py::arg("rhsonly") = false, py::arg("simplify") = true);
		def_algo<keep_terms, std::vector<int>>(m, "keep_terms", true, false, 0, py::arg("terms"));
		def_algo<young_project, std::vector<int>, std::vector<int>>(m, "young_project", true, false, 0, py::arg("shape"), py::arg("indices"));
		def_algo<simplify>(m, "simplify", false, false, 0);
		def_algo<order, Ex, bool>(m, "order", true, false, 0, py::arg("factors"), py::arg("anticommuting") = false);
		def_algo<epsilon_to_delta, bool>(m, "epsilon_to_delta", true, false, 0, py::arg("reduce") = true);
		def_algo<rename_dummies, std::string, std::string>(m, "rename_dummies", true, false, 0, py::arg("set") = "", py::arg("to") = "");
		def_algo<sym, Ex, bool>(m, "sym", true, false, 0, py::arg("items"), py::arg("antisymmetric") = false);
		def_algo<sym, Ex, bool>(m, "asym", true, false, 0, py::arg("items"), py::arg("antisymmetric") = true);
		def_algo<sym, std::vector<unsigned int>, bool>(m, "slot_sym", true, false, 0, py::arg("items"), py::arg("antisymmetric") = false);
		def_algo<sym, std::vector<unsigned int>, bool>(m, "slot_asym", true, false, 0, py::arg("items"), py::arg("antisymmetric") = true);
		def_algo<factor_in, Ex>(m, "factor_in", true, false, 0, py::arg("factors"));
		def_algo<factor_out, Ex, bool>(m, "factor_out", true, false, 0, py::arg("factors"), py::arg("right") = false);
		def_algo<fierz, Ex>(m, "fierz", true, false, 0, py::arg("spinors"));
		def_algo<substitute, Ex, bool>(m, "substitute", true, false, 0, py::arg("rules"), py::arg("partial") = true);
		def_algo<take_match, Ex>(m, "take_match", true, false, 0, py::arg("rules"));
		def_algo<replace_match>(m, "replace_match", false, false, 0);
		def_algo<zoom, Ex, bool>(m, "zoom", true, false, 0, py::arg("rules"), py::arg("partial") = true);
		def_algo_preorder<unzoom>(m, "unzoom", true, false, 0);
		def_algo<untrace>(m, "untrace", true, false, 0);
		def_algo<rewrite_indices, Ex, Ex>(m, "rewrite_indices", true, false, 0, py::arg("preferred"), py::arg("converters"));
		def_algo_preorder<vary, Ex>(m, "vary", false, false, 0, py::arg("rules"));
		def_algo<split_gamma, bool>(m, "split_gamma", true, false, 0, py::arg("on_back"));
		def_algo<split_index, Ex>(m, "split_index", true, false, 0, py::arg("rules"));
		def_algo<unwrap, Ex>(m, "unwrap", true, false, 0, py::arg("wrapper") = Ex{});
//		def_algo_preorder<young_reduce, const Ex*>(m, "young_reduce", true, false, 0, py::arg("pattern") = nullptr);
//		def_algo_preorder<young_reduce_trace>(m, "young_reduce_trace", true, false, 0);
		def_algo_preorder<meld, bool>(m, "meld", true, false, 0, py::arg("project_as_sum") = false);

//		def_algo<nevaluate>(m, "nevaluate", true, false, 0);
		m.def("nevaluate",
				[](Ex_ptr ex, py::dict d) {
				std::vector<std::pair<Ex, NTensor>> values;
				NEvaluator ev(*ex);
				for(const auto& dv: d) {
					// std::cerr << dv.first << std::endl;
//					values.push_back(std::make_pair(py::cast<Ex>(dv.first), py::cast<std::vector<double>>(dv.second)));
					ev.set_variable(py::cast<Ex>(dv.first), py::cast<std::vector<double>>(dv.second));
					}
				auto res = ev.evaluate();
				return res;
//				nevaluate algo(*get_kernel_from_scope(), *ex, values);
//				algo.apply

				}
				);
		}
	}
