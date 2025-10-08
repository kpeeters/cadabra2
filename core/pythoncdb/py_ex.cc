
#include "Config.hh"

#include <pybind11/stl.h>

#include "py_ex.hh"
#include "py_helpers.hh"
#include "py_globals.hh"
#include "py_kernel.hh"
#include "py_algorithms.hh"

#include "algorithms/map_sympy.hh"
#ifdef MATHEMATICA_FOUND
#include "algorithms/map_mma.hh"
#endif

#include "Exceptions.hh"
#include "Parser.hh"
#include "PreClean.hh"
#include "Cleanup.hh"
#include "Bridge.hh"
#include "SympyCdb.hh"
#include "NTensor.hh"
#include "NInterpolatingFunction.hh"

// Includes for display routines
#include <sstream>
#include "DisplayMMA.hh"
#include "DisplayTeX.hh"
#include "DisplaySympy.hh"
#include "DisplayTerminal.hh"

#include <ios>
#include <iomanip>

// #define DEBUG 1

namespace cadabra {

	namespace py = pybind11;

	bool Ex_compare(Ex_ptr one, Ex_ptr other)
		{
		return tree_equal(&(get_kernel_from_scope()->properties), *one, *other);
		}

	bool Ex_compare(Ex_ptr one, int other)
		{
		auto ex = std::make_shared<Ex>(other);
		return Ex_compare(one, ex);
		}

	Ex_ptr Ex_add(const Ex_ptr ex1, const ExNode ex2)
		{
		return Ex_add(ex1, ex2.ex, ex2.it);

		}

	Ex_ptr Ex_add(const Ex_ptr ex1, const Ex_ptr ex2)
		{
		return Ex_add(ex1, ex2, ex2->begin());
		}

	Ex_ptr Ex_add(const Ex_ptr ex1, const Ex_ptr ex2, Ex::iterator top2)
		{
		if (ex1->size() == 0) return ex2;
		if (ex2->size() == 0) return ex1;

		// Using '+' for list addition is very illogical; it should add
		// componentwise.
//		bool comma1 = (*ex1->begin()->name == "\\comma");
//		bool comma2 = (*ex2->begin()->name == "\\comma");

//		if (comma1 || comma2) {
//			if (comma1) {
//				auto ret = std::make_shared<Ex>(*ex1);
//				auto loc = ret->append_child(ret->begin(), top2);
//				if (comma2)
//					ret->flatten_and_erase(loc);
//				return ret;
//				}
//			else {
//				auto ret = std::make_shared<Ex>(top2);
//				auto loc = ret->prepend_child(ret->begin(), ex1->begin());
//				if (comma1)
//					ret->flatten_and_erase(loc);
//				return ret;
//				}
//			}
//		else {
			auto ret = std::make_shared<Ex>(*ex1);
			if (*ret->begin()->name != "\\sum")
				ret->wrap(ret->begin(), str_node("\\sum"));
			ret->append_child(ret->begin(), top2);

			auto it = ret->begin();
			cleanup_dispatch(*get_kernel_from_scope(), *ret, it);
			return ret;
//			}
		}

   Ex_ptr Ex_join(const Ex_ptr ex1, const Ex_ptr ex2)
		{
		// DEPRECATED: no longer in use, can be removed
		if (ex1->size() == 0) return ex2;
		if (ex2->size() == 0) return ex1;

		bool comma1 = (*ex1->begin()->name == "\\comma");
		bool comma2 = (*ex2->begin()->name == "\\comma");

		if(comma1 || comma2) {
			if (comma1) {
				auto ret = std::make_shared<Ex>(*ex1);
				auto loc = ret->append_child(ret->begin(), ex2->begin());
				if (comma2)
					ret->flatten_and_erase(loc);
				return ret;
				}
			else {
				auto ret = std::make_shared<Ex>(ex2->begin());
				auto loc = ret->prepend_child(ret->begin(), ex1->begin());
				if (comma1)
					ret->flatten_and_erase(loc);
				return ret;
				}
			}
		else {
			auto ret = std::make_shared<Ex>(*ex1);
			if (*ret->begin()->name != "\\comma")
				ret->wrap(ret->begin(), str_node("\\comma"));
			ret->append_child(ret->begin(), ex2->begin());

			auto it = ret->begin();
			cleanup_dispatch(*get_kernel_from_scope(), *ret, it);
			return ret;
			}
		}

   Ex_ptr Ex_join(const std::vector<Ex_ptr>& exs)
		{
		auto ret = std::make_shared<Ex>("\\comma");

		for(const Ex_ptr& ex: exs) {
			// skip ex if empty
			if (ex->size() > 0) {
				auto loc = ret->append_child(ret->begin(), ex->begin());
				if(*ex->begin()->name=="\\comma") 
					ret->flatten_and_erase(loc);
				}
			}
		// Note: ret could still \comma{} or \comma{x}
		auto it = ret->begin();
		if (ret->number_of_children(it) < 2) {
			ret->flatten_and_erase(it);
			}
		return ret;
		}
	
	Ex_ptr Ex_mul(const Ex_ptr ex1, const Ex_ptr ex2)
		{
		return Ex_mul(ex1, ex2, ex2->begin());
		}


	Ex_ptr Ex_mul(const Ex_ptr ex1, const Ex_ptr ex2, Ex::iterator top2)
		{
		if (ex1->size() == 0) return ex2;
		if (ex2->size() == 0) return ex1;

		auto ret = std::make_shared<Ex>(*ex1);
		if (*ret->begin()->name != "\\prod")
			ret->wrap(ret->begin(), str_node("\\prod"));
		ret->append_child(ret->begin(), top2);

		auto it = ret->begin();
		cleanup_dispatch(*get_kernel_from_scope(), *ret, it);
		return ret;
		}

	Ex_ptr Ex_sub(const Ex_ptr ex1, const ExNode ex2)
		{
		return Ex_sub(ex1, ex2.ex, ex2.it);

		}

	Ex_ptr Ex_sub(const Ex_ptr ex1, const Ex_ptr ex2)
		{
		return Ex_sub(ex1, ex2, ex2->begin());
		}

	Ex_ptr Ex_sub(const Ex_ptr ex1, const Ex_ptr ex2, Ex::iterator top2)
		{
		if (ex1->size() == 0) {
			if (ex2->size() != 0) {
				auto ret = std::make_shared<Ex>(*ex2);
				multiply(ex2->begin()->multiplier, -1);
				auto it = ret->begin();
				cleanup_dispatch(*get_kernel_from_scope(), *ret, it);
				return ret;
				}
			else return ex2;
			}
		if (ex2->size() == 0) return ex1;

		auto ret = std::make_shared<Ex>(*ex1);
		if (*ret->begin()->name != "\\sum")
			ret->wrap(ret->begin(), str_node("\\sum"));
		auto it = ret->append_child(ret->begin(), top2);
		multiply(it->multiplier, -1);
		cleanup_dispatch(*get_kernel_from_scope(), *ret, it);

		it = ret->begin();
		cleanup_dispatch(*get_kernel_from_scope(), *ret, it);

		return ret;
		}

	Ex_ptr fetch_from_python(const std::string& nm)
		{
		auto locals = get_locals();
		auto ret = fetch_from_python(nm, locals);
		if (!ret) {
			auto globals = get_globals();
			ret = fetch_from_python(nm, globals);
			}
		return ret;
		}

	Ex_ptr fetch_from_python(const std::string& nm, pybind11::object scope)
		{
		if( !scope_has(scope, nm) ) {
			return 0;
			}
		auto obj = scope[nm.c_str()];

		// Try 'Ex'
		try {
			return obj.cast<Ex_ptr>();
			}
		catch (const pybind11::cast_error& e) {
			}

		// Try 'ExNode'
		try {
			auto exnode = obj.cast<ExNode>();
			auto ret = std::make_shared<Ex>(exnode.it);
			return ret;
			}
		catch (const pybind11::cast_error& e) {
			}

		// Try float or int.
		if(pybind11::isinstance<py::int_>(obj)) {
			auto ret = std::make_shared<Ex>(pybind11::cast<int>(obj));
			return ret;
			}
		if(pybind11::isinstance<py::float_>(obj)) {
			auto ret = std::make_shared<Ex>(pybind11::cast<float>(obj));
			return ret;
			}
			
		return 0;
		}

	std::string Ex_as_str(Ex_ptr ex)
		{
		std::ostringstream str;
		//
		// //	if(state()==Algorithm::result_t::l_no_action)
		// //		str << "(unchanged)" << std::endl;
		// 	DisplayTeX dt(get_kernel_from_scope()->properties, ex);

		DisplayTerminal dt(*get_kernel_from_scope(), *ex, true);
		dt.output(str);

		return str.str();
		}

	std::string Ex_as_repr(Ex_ptr ex)
		{
		if (!ex) return "";
		if (ex->begin() == ex->end()) return "";

		//		Ex::iterator it = ex->begin();
		std::ostringstream str;
		ex->print_python(str, ex->begin());
		return str.str();
		}

	std::string Ex_as_latex(Ex_ptr ex)
		{
		if (!ex) return "";
		std::ostringstream str;
		DisplayTeX dt(*get_kernel_from_scope(), *ex);
		dt.output(str);
		return str.str();
		}

	pybind11::object Ex_as_sympy(Ex_ptr ex)
		{
		// Generate a string which can be parsed by Sympy.
		std::string txt = Ex_as_sympy_string(ex);

		// Call sympify on a sympy-parseable  textual representation.
		pybind11::module sympy_parser = pybind11::module::import("sympy.parsing.sympy_parser");
		auto parse = sympy_parser.attr("parse_expr");
#ifdef DEBUG
		std::cerr << "Feeding sympy: " << txt << std::endl;
#endif
		pybind11::object ret = parse(txt);
		return ret;
		}

	pybind11::object ExNode_get_multiplier(const ExNode& ex)
		{
		if(!ex.is_valid())
			throw ConsistencyException("Cannot get the multiplier of an iterator before the first 'next'.");

		if(ex.it->multiplier->is_rational()) {
			pybind11::object mpq = pybind11::module::import("gmpy2").attr("mpq");
			auto m = ex.it->multiplier->get_rational();
			pybind11::object mult = mpq(m.get_num().get_si(), m.get_den().get_si());
			return mult;
			}
		else {
			return pybind11::cast(ex.it->multiplier->get_double());
			}
		}
	
	void ExNode_set_multiplier(ExNode& ex, pybind11::object mult)
		{
		if(!ex.is_valid())
			throw ConsistencyException("Cannot set the multiplier of an iterator before the first 'next'.");

		// FIXME: this assumes the python object is a rational, but it could be a double now.
		set(ex.it->multiplier, multiplier_t(mult.attr("numerator").cast<long>(),
														mult.attr("denominator").cast<long>()) );
		}

	pybind11::object ExNode_as_sympy(const ExNode& exnode)
		{
		return Ex_as_sympy(exnode.ex);
		}

	std::string Ex_as_sympy_string(Ex_ptr ex)
		{
		// Check to see if the expression is a scalar without dummy indices.
		//	Algorithm::index_map_t ind_free, ind_dummy;
		//	Algorithm::classify_indices(ex.begin(), ind_free, ind_dummy);
		//	if(ind_dummy.size()>0)
		//		throw NonScalarException("Expression contains dummy indices.");
		//	if(ind_free.size()>0)
		//		throw NonScalarException("Expression contains free indices.");

		if (!ex) return "";
		std::ostringstream str;
		DisplaySympy dt(*get_kernel_from_scope(), *ex);
		dt.output(str);
		return str.str();
		}

	std::string Ex_as_input(Ex_ptr ex)
		{
		std::ostringstream str;
		//
		// //	if(state()==Algorithm::result_t::l_no_action)
		// //		str << "(unchanged)" << std::endl;
		// 	DisplayTeX dt(get_kernel_from_scope()->properties, ex);

		DisplayTerminal dt(*get_kernel_from_scope(), *ex, false);
		dt.output(str);

		return str.str();
		}

	std::string Ex_as_MMA(Ex_ptr ex, bool use_unicode)
		{
		// Check to see if the expression is a scalar without dummy indices.
		//	Algorithm::index_map_t ind_free, ind_dummy;
		//	Algorithm::classify_indices(ex.begin(), ind_free, ind_dummy);
		//	if(ind_dummy.size()>0)
		//		throw NonScalarException("Expression contains dummy indices.");
		//	if(ind_free.size()>0)
		//		throw NonScalarException("Expression contains free indices.");

		std::ostringstream str;
		DisplayMMA dt(*get_kernel_from_scope(), *ex, use_unicode);
		dt.output(str);

		return str.str();
		}

	Ex_comparator::match_t ExNode_compare(const ExNode& lhs, const ExNode& rhs, const std::string& use_props, bool ignore_parent_rel)
	{
		Ex_comparator comp(get_kernel_from_scope()->properties);
		Ex_comparator::useprops_t up = Ex_comparator::useprops_t::always;
		if (use_props == "not_at_top")
			up = Ex_comparator::useprops_t::not_at_top;
		else if (use_props == "never")
			up = Ex_comparator::useprops_t::never;
		return comp.equal_subtree(lhs.it, rhs.it, up, ignore_parent_rel);
	}

	std::string print_tree(Ex *ex)
		{
		std::ostringstream str;
		ex->print_entire_tree(str);
		return str.str();
		}

	Ex lhs(Ex_ptr ex)
		{
		auto it = ex->begin();
		if (it == ex->end())
			throw ArgumentException("Empty expression passed to 'lhs'.");

		if (*it->name != "\\equals")
			throw ArgumentException("Cannot take 'lhs' of expression which is not an equation.");

		return Ex(ex->begin(ex->begin()));
		}

	Ex rhs(Ex_ptr ex)
		{
		auto it = ex->begin();
		if (it == ex->end())
			throw ArgumentException("Empty expression passed to 'rhs'.");

		if (*it->name != "\\equals")
			throw ArgumentException("Cannot take 'rhs' of expression which is not an equation.");

		auto sib = ex->begin(ex->begin());
		++sib;
		return Ex(sib);
		}

	Ex Ex_getslice(Ex_ptr ex, pybind11::slice slice)
		{
		Ex result;

		pybind11::size_t start, stop, step, length;
		// std::cerr << "SLICE object has " << ex->number_of_children(ex->begin()) << " elements" << std::endl;
		slice.compute(ex->number_of_children(ex->begin()), &start, &stop, &step, &length);
		// std::cerr << "SLICE: " << start << ", " << stop << ", " << step << ", " << length << std::endl;
		if (length == 0)
			return result;

		// Set head
		auto it = result.set_head(*ex->begin());

		// Iterate over fully-closed range.
		for (; start < stop; start += step) {
			Ex::sibling_iterator toadd=ex->begin(ex->begin());
			std::advance(toadd, start);
			result.append_child(it, Ex::iterator(toadd));
			}
		return result;
		}

	Ex Ex_getitem(Ex &ex, int index)
		{
		Ex::iterator it = ex.begin();

		size_t num = ex.number_of_children(it);
		if (index >= 0 && (size_t)index < num)
			return Ex(ex.child(it, index));
		else {
			//		if(num==0 && index==0) {
			//			std::cerr << "returning " << ex << std::endl;
			//			return Ex(ex);
			//			}
			//		else
			throw ArgumentException("index " + std::to_string(index) + " out of range, must be smaller than " + std::to_string(num));
			}
		}

	void Ex_setitem(Ex_ptr ex, int index, Ex val)
		{
		Ex::iterator it = ex->begin();

		size_t num = ex->number_of_children(it);
		if (index >= 0 && (size_t)index < num)
			ex->replace(ex->child(it, index), val.begin());
		else
			throw ArgumentException("index " + std::to_string(index) + " out of range, must be smaller than " + std::to_string(num));
		}

	void Ex_setitem_iterator(Ex_ptr ex, ExNode en, Ex_ptr val)
		{
		Ex::iterator use;
		if (en.ex != ex) {
			//		std::cerr << "Setitem need to convert iterator of" << std::endl;
			//		std::cerr << en.it << std::endl;
			//		std::cerr << "in " << en.topit << std::endl;
			//		std::cerr << "of " << en.ex->begin() << std::endl;
			auto path = en.ex->path_from_iterator(en.it, en.topit);
			//		for(auto v: path)
			//			std::cerr << v << std::endl;
			//		std::cerr << "for " << ex->begin() << std::endl;
			use = ex->iterator_from_path(path, ex->begin());
			//		std::cerr << "which is " << use << std::endl;
			}
		else {
			use = en.it;
			}

		Ex::iterator top = val->begin();
		if (*top->name == "")
			top = val->begin(top);

		ex->replace_index(use, top, true);
		}

	size_t Ex_len(Ex_ptr ex)
		{
		Ex::iterator it = ex->begin();

		return ex->number_of_children(it);
		}

	long Ex_int_cast(Ex_ptr ex)
		{
		return ex->to_integer(); // this will throw an exception if the object is not integer
		}

	std::string Ex_head(Ex_ptr ex)
		{
		if (ex->begin() == ex->end())
			throw ArgumentException("Expression is empty, no head.");
		return *ex->begin()->name;
		}

	pybind11::object Ex_get_mult(Ex_ptr ex)
		{
		if (ex->begin() == ex->end())
			throw ArgumentException("Expression is empty, no head.");
		auto m = *ex->begin()->multiplier;
		//	return mpq(2,3);

		if(m.is_rational()) {
			pybind11::object mpq = pybind11::module::import("gmpy2").attr("mpq");
			pybind11::object mult = mpq(m.get_rational().get_num().get_si(), m.get_rational().get_den().get_si());
			return mult;
			}
		else return pybind11::cast(m.get_double());
		}

	pybind11::list terms(Ex_ptr ex)
		{
		Ex::iterator it = ex->begin();

		if (*it->name != "\\sum")
			throw ArgumentException("terms() expected a sum expression.");

		pybind11::list ret;

		auto sib = ex->begin(it);
		while (sib != ex->end(it)) {
			ret.append(Ex(sib));
			++sib;
			}

		return ret;
		}

	std::shared_ptr<sympy::SympyBridge> SympyBridge_init(std::shared_ptr<Ex> ex)
		{
		auto sb = std::make_shared<sympy::SympyBridge>(*get_kernel_from_scope(), ex);
		return sb;
		}

	Ex_ptr Ex_from_string(const std::string& ex_, bool, Kernel *kernel)
		{
		if (kernel == nullptr)
			kernel = get_kernel_from_scope();

		auto ptr = std::make_shared<Ex>();
		// Parse the string expression.
		Parser parser(ptr);
		std::stringstream str(ex_);

		try {
			str >> parser;
			}
		catch (std::exception& except) {
			for(size_t i=0; i<ex_.size(); ++i)
				std::cerr << static_cast<int>(static_cast<uint8_t>(ex_[i])) << " ";
			std::cerr << std::endl;
			throw ParseException(std::string("Cannot parse: ")+except.what());
			}
		parser.finalise();

		// First pull in any expressions referred to with @(...) notation, because the
		// full expression may not have consistent indices otherwise.
		pull_in(ptr, kernel);
		//	std::cerr << "pulled in" << std::endl;

		// Basic cleanup of rationals and subtractions, followed by
		// cleanup of nested sums and products.
		pre_clean_dispatch_deep(*kernel, *ptr);
		cleanup_dispatch_deep(*kernel, *ptr);
		check_index_consistency(*kernel, *ptr, (*ptr).begin());
		call_post_process(*kernel, ptr);
		//	std::cerr << "cleaned up" << std::endl;

		// Now run all embedded python functions.
		run_python_functions(ptr, kernel);

		return ptr;
		}

	Ex_ptr Ex_from_int(int num, bool)
		{
		auto ptr = std::make_shared<Ex>(num);
		return ptr;
		}

	Ex_ptr Ex_from_float(double num, bool)
		{
		auto ptr = std::make_shared<Ex>(num);
		return ptr;
		}

	void call_post_process(Kernel& kernel, Ex_ptr ex)
		{
		// Find the 'post_process' function, and if found, turn off
		// post-processing, then call the function on the current Ex.
		if (post_process_enabled) {
			if (ex->number_of_children(ex->begin()) == 0)
				return;

			post_process_enabled = false;
			pybind11::object post_process;

			auto locals = get_locals();
			if (scope_has(locals, "post_process")) {
				post_process = locals["post_process"];
				}
			else {
				auto globals = get_globals();
				if (scope_has(globals, "post_process"))
					post_process = globals["post_process"];
				}
			if (post_process) {
				// std::cerr << "calling post-process" << std::endl;
				post_process(std::ref(kernel), ex);
				}
			post_process_enabled = true;
			}
		}

	void Ex_cleanup(Ex_ptr ex)
	{
		Kernel* kernel = get_kernel_from_scope();
		pre_clean_dispatch_deep(*kernel, *ex);
		cleanup_dispatch_deep(*kernel, *ex);
		check_index_consistency(*kernel, *ex, (*ex).begin());
		call_post_process(*kernel, ex);
	}

	Ex_ptr map_sympy_wrapper(Ex_ptr ex, std::string head, pybind11::args args)
		{
		std::vector<std::string> av;
		for (auto& arg : args)
			av.push_back(arg.cast<std::string>());
		return apply_algo_preorder<map_sympy, std::string, std::vector<std::string>>(ex, head, av, false, false, 0);
		}

	void init_ex(py::module& m)
		{
		pybind11::enum_<str_node::parent_rel_t>(m, "parent_rel_t")
		.value("sub", str_node::parent_rel_t::p_sub)
		.value("super", str_node::parent_rel_t::p_super)
		.value("none", str_node::parent_rel_t::p_none)
		.export_values()
		;

		pybind11::enum_<Ex_comparator::match_t>(m, "match_t")
			.value("node_match", Ex_comparator::match_t::node_match)
			.value("subtree_match", Ex_comparator::match_t::subtree_match)
			.value("match_index_less", Ex_comparator::match_t::match_index_less)
			.value("match_index_greater", Ex_comparator::match_t::match_index_greater)
			.value("no_match_indexpos_less", Ex_comparator::match_t::no_match_indexpos_less)
			.value("no_match_indexpos_greater", Ex_comparator::match_t::no_match_indexpos_greater)
			.value("no_match_less", Ex_comparator::match_t::no_match_less)
			.value("no_match_greater", Ex_comparator::match_t::no_match_greater);

		pybind11::class_<Ex, Ex_ptr >(m, "Ex", py::buffer_protocol())
		.def(py::init(&Ex_from_string), py::arg("input_form"), py::arg("make_ref") = true, py::arg("kernel") = nullptr)
		.def(py::init(&Ex_from_int), py::arg("num"), py::arg("make_ref") = true)
		.def(py::init(&Ex_from_float), py::arg("num"), py::arg("make_ref") = true)
		.def("__str__", &Ex_as_str)
		.def("_latex_", &Ex_as_latex)
		.def("__repr__", &Ex_as_repr)
		.def("__eq__", static_cast<bool(*)(Ex_ptr, Ex_ptr)>(&Ex_compare))
		.def("__eq__", static_cast<bool(*)(Ex_ptr, int)>(&Ex_compare))
		.def("_sympy_", &Ex_as_sympy)
		.def("sympy_form", &Ex_as_sympy_string)
		.def("mma_form", &Ex_as_MMA, pybind11::arg("unicode") = true)    // standardize on this
		.def("input_form", &Ex_as_input)
		.def("__getitem__", &Ex_getitem)
		.def("__getitem__", &Ex_getitem_string)
		.def("__getitem__", &Ex_getitem_iterator)
		.def("__getitem__", &Ex_getslice)
		.def("__setitem__", &Ex_setitem)
		.def("__setitem__", &Ex_setitem_iterator)
		.def("__len__", &Ex_len)
		.def("__int__", &Ex_int_cast)
		.def("head", &Ex_head)
		.def("mult", &Ex_get_mult)
		.def("__iter__", &Ex_iter)
		.def("top", &Ex_top)
		.def("matches", &Ex_matches)
		.def("matches", &Ex_matches_Ex)
		.def("state", &Ex::state)
		.def("reset", &Ex::reset_state)
		.def("copy", [](const Ex& ex) { return std::make_shared<Ex>(ex); })
		.def("size", [](const Ex& ex) { return ex.size(); })
		.def("changed", &Ex::changed_state)
		.def("cleanup", &Ex_cleanup)
		.def("__array__", [](Ex& ex) {
			if(std::holds_alternative<std::shared_ptr<NTensor>>(ex.begin()->content)) {
				std::shared_ptr<NTensor> ntp = std::get<std::shared_ptr<NTensor>>(ex.begin()->content);
            py::object py_ntensor = py::cast(*ntp);

				if(!py::isinstance<py::buffer>(py_ntensor)) {
					throw std::runtime_error("NTensor does not implement buffer protocol");
					}
				
				py::buffer  py_buffer  = py_ntensor.cast<py::buffer>();
				py::module_ np = py::module_::import("numpy");
            
            // Create a numpy array view of the buffer (no copy)
            py::object array = np.attr("asarray")(py_ntensor);
				return array;
 				}
			else {
				throw ConsistencyException("Expression cannot be converted to numerical array.");
				}
			})
//		.def_buffer([](Ex &ex) -> py::buffer_info {
//			throw std::runtime_error("Help?");
//			})
//		.def("__floordiv__", [](const Ex& self, py::function func) {
//			   Ex expr_copy(self);
//            py::object result = func(expr_copy);
//            return result.cast<Ex>();
//			})
		.def("__hash__", [](const Ex& ex) { return ex.calc_hash(ex.begin()); })
		.def("__add__", static_cast<Ex_ptr(*)(const Ex_ptr, const ExNode)>(&Ex_add), py::is_operator{})
		.def("__add__", static_cast<Ex_ptr(*)(const Ex_ptr, const Ex_ptr)>(&Ex_add), py::is_operator{})
		.def("__sub__", static_cast<Ex_ptr(*)(const Ex_ptr, const ExNode)>(&Ex_sub), py::is_operator{})
		.def("__sub__", static_cast<Ex_ptr(*)(const Ex_ptr, const Ex_ptr)>(&Ex_sub), py::is_operator{})
		.def("__mul__", static_cast<Ex_ptr(*)(const Ex_ptr, const Ex_ptr)>(&Ex_mul), py::is_operator{})
			.def(py::pickle(
			[](const Ex_ptr& ex) { return py::make_tuple(Ex_as_input(ex)); },
			[](py::tuple t) { return Ex_from_string(t[0].cast<std::string>(), true, get_kernel_from_scope()); }
		));

		pybind11::class_<ExNode>(m, "ExNode", "Iterator over elements of an Ex mathematical expression.", py::buffer_protocol())
			.def("__iter__", &ExNode::iter)
			.def("__next__", &ExNode::next, pybind11::return_value_policy::reference_internal)
			.def("input_form",  &ExNode::input_form)
			.def("__getitem__", &ExNode::getitem_string)
			.def("__getitem__", &ExNode::getitem_iterator)
			.def("__setitem__", &ExNode::setitem_string)
			.def("__setitem__", &ExNode::setitem_iterator)
			.def("_latex_", &ExNode::_latex_)
			.def("__str__", &ExNode::__str__)
			.def("_sympy_", &ExNode_as_sympy)
			.def("terms", &ExNode::terms, "Return an ExNode iterator over all terms at the level of the current ExNode.")
			.def("components", &ExNode::components, "Returns an ExNode iterator over all components of the tensor at the given ExNode.")
			.def("factors", &ExNode::factors, "Return an ExNode iterator over all factors at the level of the current ExNode.")
			.def("own_indices", &ExNode::own_indices, "Return an ExNode iterator over all indices which are not inherited from child nodes.")
			.def("indices", &ExNode::indices, "Return an ExNode iterator over all indices.")
			.def("free_indices", &ExNode::free_indices, "Return an ExNode iterator over all free indices.")
			.def("args", &ExNode::args)
			.def("children", &ExNode::children)
			.def("replace", &ExNode::replace, "Replace the node pointed to by ExNode with the given Ex.")
			.def("insert", &ExNode::insert, "Insert the given Ex in front of the node pointed to by the ExNode.")
			.def("insert", &ExNode::insert_it, "Insert the node pointed to by the given ExNode in front of the ExNode.")
			.def("append_child", &ExNode::append_child, "Append the given Ex as a child of the node pointed to by the ExNode.")
			.def("append_child", &ExNode::append_child_it, "Append the node pointed to by the given ExNode as a child of the node pointed to by the ExNode.")
			.def("erase", &ExNode::erase, "Erase the node pointed to by the ExNode.")
			.def("ex", &ExNode::get_ex, "Obtain a copy of the node pointed to by the ExNode.")
			.def("compare", &ExNode_compare,
				py::arg("other"), py::arg("use_props") = "always", py::arg("ignore_parent_rel") = false)
			.def_property("name", &ExNode::get_name, &ExNode::set_name, "Set the name property of the node pointed to by the ExNode.")
			.def_property("parent_rel", &ExNode::get_parent_rel, &ExNode::set_parent_rel)
			.def_property("multiplier", &ExNode_get_multiplier, &ExNode_set_multiplier)
			.def_buffer([](ExNode &exnode) -> py::buffer_info {
				throw std::range_error("Do not call this on ExNode");
				})
			.def("__add__", [](ExNode a, Ex_ptr b) {
								 return a.add_ex(b);
								 }, pybind11::is_operator{});

		pybind11::class_<sympy::SympyBridge, std::shared_ptr<sympy::SympyBridge> >(m, "SympyBridge")
		.def(py::init(&SympyBridge_init))
		.def("to_sympy", &sympy::SympyBridge::export_ex)
		.def("from_sympy", &sympy::SympyBridge::import_ex)
//		.def("from_sympy_new", &sympy::SympyBridge::import_ex_new)			
//		.def("from_sympy_new", &sympy::SympyBridge::import_ex_new)
		;

		m.def("join", [](const Ex_ptr ex1, const Ex_ptr ex2, py::args args) {
			std::vector<Ex_ptr> ex = {ex1, ex2};
        	for (const auto& arg : args) {
				ex.push_back(arg.cast<Ex_ptr>());
				}
        	return Ex_join(ex);
    		});

		m.def("tree", &print_tree);

		m.def("function_domain", &cadabra::function_domain);
		
		m.def("map_sympy", &map_sympy_wrapper,
		      pybind11::arg("ex"),
		      pybind11::arg("function") = "",
		      pybind11::return_value_policy::reference_internal);
#ifdef MATHEMATICA_FOUND
		def_algo<map_mma, std::string>(m, "map_mma", false, false, 0, pybind11::arg("function") = "");
#endif

		m.def("terms", &terms);

		m.def("lhs", &lhs);
		m.def("rhs", &rhs);

		}

	}
