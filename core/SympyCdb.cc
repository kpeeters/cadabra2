
#include "Storage.hh"
#include <gmpxx.h>
#define PYBIND11_DETAILED_ERROR_MESSAGES

#include <pybind11/pybind11.h>
#include <sstream>
#include "Functional.hh"
#include "SympyCdb.hh"
#include "Exceptions.hh"
#include "PreClean.hh"
#include "Cleanup.hh"
#include "Parser.hh"
#include "Kernel.hh"
#include "DisplaySympy.hh"
#include "algorithms/substitute.hh"
#include "properties/AntiCommuting.hh"
#include "properties/NonCommuting.hh"
#include "properties/SelfAntiCommuting.hh"
#include "properties/SelfNonCommuting.hh"

using namespace cadabra;

// #define DEBUG 1

#ifndef NO_SYMPY

sympy::SympyBridge::SympyBridge(const Kernel& k, std::shared_ptr<Ex> ex)
	: DisplaySympy(k, *ex), ex(ex)
	{
	Ex::iterator it=(*ex).begin();
	while(it != (*ex).end()) {
		if(Algorithm::is_noncommuting(kernel.properties, it))
			throw RuntimeException("Cannot handle NonCommuting/AntiCommuting objects in the SymPy bridge.");
		
		++it;
		}
	}

sympy::SympyBridge::~SympyBridge()
	{
	}

pybind11::object sympy::SympyBridge::export_ex()
	{
	std::ostringstream str;
	output(str);
	pybind11::module sympy_parser = pybind11::module::import("sympy.parsing.sympy_parser");
	auto parse = sympy_parser.attr("parse_expr");
#ifdef DEBUG
	std::cerr << "sympy::SympyBridge::export_ex: " << str.str() << std::endl;
#endif
	pybind11::object ret = parse(str.str());
#ifdef DEBUG
	std::cerr << "sympy::SympyBridge::export_ex: succes" << std::endl;
#endif
	
	return ret;
	}

cadabra::Ex sympy::SympyBridge::convert(pybind11::handle obj)
	{
	static std::map<std::string, std::function<Ex(pybind11::handle)>> known =
		{
			{ "<class 'sympy.sets.sets.FiniteSet'>",  [this](pybind11::handle node)
					{
					Ex ex_tuple("\\comma");
					
					pybind11::object py_args = node.attr("args");
					for(auto item: py_args)
						ex_tuple.append_child(ex_tuple.begin(), convert(item).begin());

					return ex_tuple;
					}
			},
			{ "<class 'set'>",  [this](pybind11::handle node)
					{
					Ex ex_tuple("\\comma");
					
					for(auto item: node)
						ex_tuple.append_child(ex_tuple.begin(), convert(item).begin());

					return ex_tuple;
					}
			},
			{ "<class 'list'>",  [this](pybind11::handle node)
					{
					Ex ex_tuple("\\comma");
					
					for(auto item: node)
						ex_tuple.append_child(ex_tuple.begin(), convert(item).begin());

					return ex_tuple;
					}
			},
			{ "<class 'sympy.core.containers.Tuple'>", [this](pybind11::handle node)
					{
					Ex ex_tuple("\\comma");
					
					pybind11::object py_args = node.attr("args");
					for(auto item: py_args)
						ex_tuple.append_child(ex_tuple.begin(), convert(item).begin());

					
					return ex_tuple;
					}
			},
			{ "<class 'tuple'>", [this](pybind11::handle node)
					{
					Ex ex_tuple("\\comma");
					
					pybind11::tuple py_vars_tuple = pybind11::reinterpret_borrow<pybind11::tuple>(node);
					size_t num = pybind11::len(py_vars_tuple);
					for(size_t i=0; i<num; ++i) 
						ex_tuple.append_child(ex_tuple.begin(), convert(py_vars_tuple[i]).begin());

					return ex_tuple;
					}
			},
			{ "<class 'sympy.core.add.Add'>",          [this](pybind11::handle node)
					{
					Ex ex_add("\\sum");

					// The order of terms in `args` is some weird SymPy idea of canonical
					// form. We use StrPrinter()._as_ordered_terms(node) to get something
					// suitable for human consumption.

					pybind11::module sympy_printing = pybind11::module::import("sympy.printing.str");
					auto StrPrinter_class = sympy_printing.attr("StrPrinter");
					pybind11::object printer = StrPrinter_class();
					pybind11::object py_args = printer.attr("_as_ordered_terms")(node);
					
					for(auto item: py_args)
						ex_add.append_child(ex_add.begin(), convert(item).begin());

					return ex_add;
					}
			},
			{ "<class 'sympy.core.mul.Mul'>",          [this](pybind11::handle node)
					{
					Ex ex_mul("\\prod");

					// For `Mul`, SymPy uses *reverse* lexicographical ordering. That is just
					// madness.
					pybind11::tuple py_args = node.attr("args");
					size_t num = pybind11::len(py_args);
					for(; num>0; --num) 
						ex_mul.append_child(ex_mul.begin(), convert(py_args[num-1]).begin());

					return ex_mul;
					}
			},
			{ "<class 'sympy.core.power.Pow'>",          [this](pybind11::handle node)
					{
					Ex ex_pow("\\pow");
					
					pybind11::object py_base = node.attr("base");
					pybind11::object py_exp  = node.attr("exp");

					ex_pow.append_child(ex_pow.begin(), convert(py_base).begin());
					ex_pow.append_child(ex_pow.begin(), convert(py_exp).begin());					

					return ex_pow;
					}
			},
			{ "<class 'sympy.core.function.Derivative'>",          [this](pybind11::handle node)
					{
					Ex ex_der("\\partial"); // FIXME: use the original derivative!
					
					pybind11::object py_expr = node.attr("expr");
					pybind11::object py_vars = node.attr("variables");

					ex_der.append_child(ex_der.begin(), convert(py_expr).begin());

					pybind11::tuple py_vars_tuple = pybind11::reinterpret_borrow<pybind11::tuple>(py_vars);
					size_t num = pybind11::len(py_vars_tuple);
					for(size_t i=0; i<num; ++i) {
						ex_der.append_child(ex_der.begin(), convert(py_vars_tuple[i]).begin())->fl.parent_rel=str_node::p_sub;
						}

					return ex_der;
					}
			},
			{ "<class 'sympy.core.numbers.Integer'>",   [](pybind11::handle node)
					{
					// Make sure to handle large integers: convert to string, then parse back into mpz_class.
					std::string int_str = pybind11::str(node);
					Ex res(1);
					multiply(res.begin()->multiplier, Multiplier(mpz_class(int_str)));
					return res;
					}
			},
			{ "<class 'int'>",   [](pybind11::handle node)
					{
					// Make sure to handle large integers: convert to string, then parse back into mpz_class.
					std::string int_str = pybind11::str(node);
					Ex res(1);
					multiply(res.begin()->multiplier, Multiplier(mpz_class(int_str)));
					return res;
					}
			},
			{ "<class 'sympy.core.numbers.Float'>",          [](pybind11::handle node)
					{
					return Ex(node.cast<double>());
					}
			},
			{ "<class 'sympy.core.numbers.Zero'>",          [](pybind11::handle node)
					{
					return Ex(0);
					}
			},
			{ "<class 'sympy.core.numbers.One'>",          [](pybind11::handle node)
					{
					return Ex(1);
					}
			},
			{ "<class 'sympy.core.numbers.Half'>",          [](pybind11::handle node)
					{
					return Ex(1,2);
					}
			},
			{ "<class 'sympy.core.numbers.NegativeOne'>",          [](pybind11::handle node)
					{
					return Ex(-1);
					}
			},
			{ "<class 'sympy.core.numbers.NegativeOne'>",          [](pybind11::handle node)
					{
					return Ex(-1);
					}
			},
			{ "<class 'sympy.core.numbers.Pi'>",          [](pybind11::handle node)
					{
					return Ex("\\pi");
					}
			},
			{ "<class 'sympy.core.numbers.Infinity'>",          [](pybind11::handle node)
					{
					return Ex("\\infty");
					}
			},
			{ "<class 'sympy.core.numbers.NegativeInfinity'>",          [](pybind11::handle node)
					{
					Ex neginf("\\infty");
					multiply(neginf.begin()->multiplier, -1);
					return neginf;
					}
			},
			{ "<class 'sympy.series.order.Order'>",          [this](pybind11::handle node)
					{
					Ex ex_order("\\bigO");
					pybind11::object py_expr = node.attr("expr");
					pybind11::object py_vars = node.attr("variables");

					ex_order.append_child(ex_order.begin(), convert(py_expr).begin());
					return ex_order;
					}
			},
			{ "<class 'sympy.core.relational.Equality'>",          [this](pybind11::handle node)
					{
					Ex ex_equal("\\equals");
					
					pybind11::object py_lhs = node.attr("lhs");
					pybind11::object py_rhs = node.attr("rhs");

					ex_equal.append_child(ex_equal.begin(), convert(py_lhs).begin());
					ex_equal.append_child(ex_equal.begin(), convert(py_rhs).begin());					

					return ex_equal;
					}
			},
			{ "<class 'sympy.core.numbers.Rational'>",          [this](pybind11::handle node)
					{
					Ex ex_frac("\\frac");
					pybind11::object py_numer = node.attr("numerator");
					pybind11::object py_denom = node.attr("denominator");

					ex_frac.append_child(ex_frac.begin(), convert(py_numer).begin());
					ex_frac.append_child(ex_frac.begin(), convert(py_denom).begin());					
					
					return ex_frac;
					}
			},
			{ "<class 'sympy.core.symbol.Symbol'>",          [](pybind11::handle node)
					{
					pybind11::handle py_name = node.attr("name");
					return Ex(pybind11::str(py_name));
					}
			}
		};

	pybind11::handle type_obj = obj.get_type();
	std::string type_name = pybind11::str(type_obj);

	auto fun = known.find(type_name);
	if(fun != known.end()) {
		// std::cerr << type_name << std::endl;
		return (fun->second)(obj);
		}
	else {
		// SymPy represents each function as a separate class. So the type of
		// `f(x)` will be `f`, and you need to walk the inheritance chain to
		// figure out that it is f -> AppliedUndef -> Function.
		// See https://github.com/sympy/sympy/issues/18028 for details.

		pybind11::object mro = type_obj.attr("__mro__");
//		int depth=0;
		for(pybind11::handle base_class: mro) {
			std::string class_name = base_class.attr("__name__").cast<std::string>();
			if(class_name == "Function") {
				// std::cerr << " Function!" << std::endl;
				Ex ex_fun(type_name);
				pybind11::object py_args = obj.attr("args");
				for(pybind11::handle py_arg: py_args) {
					ex_fun.append_child(ex_fun.begin(), convert(py_arg).begin())->fl.bracket=str_node::b_none;
					}
				return ex_fun;
				}
//			if(++depth == 3)
//				break;
			}
		throw InternalError("SympyBridge::convert: do not know (yet) how to handle "+type_name);
		}
	}

void sympy::SympyBridge::import_ex(pybind11::object obj)
	{
	if(pybind11::isinstance<pybind11::str>(obj))
		throw ArgumentException("SympyBridge::from_sympy: passing as 'str' is now disabled, pass the sympy object instead.");
	
	Ex new_ex = convert(obj);
	pre_clean_dispatch_deep(kernel, new_ex);
	cleanup_dispatch_deep(kernel, new_ex);
	import(new_ex);
	Ex::iterator first=new_ex.begin();
	Ex::iterator orig=tree.begin();
	ex->move_ontop(orig, first);
	}

// void sympy::SympyBridge::import_ex(const std::string& s)
// 	{
// 	preparse_import(s);
// #ifdef DEBUG
// 	std::cerr << "sympy::SympyBridge::import_ex: " << s << std::endl;
// #endif
// 	auto ptr = std::make_shared<Ex>();
// 	cadabra::Parser parser(ptr);
// 	std::stringstream istr(s);
// 	istr >> parser;
// 
// 	pre_clean_dispatch_deep(kernel, *parser.tree);
// 	cleanup_dispatch_deep(kernel, *parser.tree);
// #ifdef DEBUG
// 	std::cerr << "importing " << parser.tree->begin() << std::endl;
// #endif
// 	import(*parser.tree);
// 	Ex::iterator first=parser.tree->begin();
// 	Ex::iterator orig=tree.begin();
// 	ex->move_ontop(orig, first);
// #ifdef DEBUG
// 	std::cerr << "result " << ex->begin() << std::endl;
// #endif
// 	}

#endif

Ex::iterator sympy::apply(const Kernel& kernel, Ex& ex, Ex::iterator& it, const std::vector<std::string>& wrap, std::vector<std::string> args,
                          const std::string& method)
	{
	// Safeguard against using noncommuting/anticommuting objects.
	Ex::iterator cpy = it, nxt = it;
	nxt.skip_children();
	++nxt;
	while(cpy != nxt) {
		if(Algorithm::is_noncommuting(kernel.properties, cpy))
			throw RuntimeException("Cannot handle NonCommuting/AntiCommuting objects in the SymPy bridge.");
		
		++cpy;
		}

	// We first need to print the sub-expression using DisplaySympy,
	// optionally with the head wrapped around it and the args added
	// (if present).
	std::ostringstream str;

	for(size_t i=0; i<wrap.size(); ++i) {
		str << wrap[i] << "(";
		}

	DisplaySympy ds(kernel, ex);
	ds.output(str, it);

	if(wrap.size()>0)
		if(args.size()>0) {
			for(size_t i=0; i<args.size(); ++i)
				str << ", " << args[i];
			}
	for(size_t i=1; i<wrap.size(); ++i)
		str << ")";
	str << method;

	if(wrap.size()>0)
		str << ")";

	// We then execute the expression in Python.

	//ex.print_recursive_treeform(std::cerr, it);
#ifdef DEBUG
	std::cerr << "feeding " << str.str() << std::endl;
	std::cerr << "which is " << it << std::endl;
#endif

	auto module = pybind11::module::import("sympy.parsing.sympy_parser");
	auto parse  = module.attr("parse_expr");
	pybind11::object obj = parse(str.str());
	//std::cerr << "converting result to string" << std::endl;
	auto __str__ = obj.attr("__str__");
	pybind11::object res = __str__();
	std::string result = res.cast<std::string>();
#ifdef DEBUG
	std::cerr << "result " << result << std::endl;
#endif


	// After that, we construct a new sub-expression from this string by using our
	// own parser, and replace the original.

	result = ds.preparse_import(result);

	auto ptr = std::make_shared<Ex>();
	cadabra::Parser parser(ptr);
	std::stringstream istr(result);
	istr >> parser;

	pre_clean_dispatch_deep(kernel, *parser.tree);
	cleanup_dispatch_deep(kernel, *parser.tree);

	//parser.tree->print_recursive_treeform(std::cerr, parser.tree->begin());

	ds.import(*parser.tree);


	Ex::iterator first=parser.tree->begin();
#ifdef DEBUG
	std::cerr << "reparsed " << first.node << "\n" << Ex(first) << std::endl;
	std::cerr << "before " << it.node << "\n" << Ex(it) << std::endl;
#endif
	str_node::parent_rel_t pr = it->fl.parent_rel;
	it = ex.move_ontop(it, first);
	it->fl.parent_rel = pr;
#ifdef DEBUG
	std::cerr << "after " << Ex(it) << std::endl;
	std::cerr << "top node " << it.node << std::endl;
#endif


	return it;
	}

Ex sympy::fill_matrix(const Kernel& kernel, Ex& ex, Ex& rules)
	{
	// check that object has two children only.
	if(ex.number_of_children(ex.begin())!=2) {
		throw ConsistencyException("Object should have exactly two indices.");
		}

	Ex::iterator ind1=ex.child(ex.begin(), 0);
	Ex::iterator ind2=ex.child(ex.begin(), 1);

	// Get Indices property and from there Coordinates.

	const Indices *prop1 = kernel.properties.get<Indices>(ind1);
	const Indices *prop2 = kernel.properties.get<Indices>(ind2);

	if(prop1!=prop2 || prop1==0)
		throw ConsistencyException("Need the indices of object to be declared with Indices property.");

	// Run over all values of Coordinates, construct matrix.

	Ex matrix("\\matrix");
	const auto& values = prop1->values(kernel.properties, ind1);
	auto cols=matrix.append_child(matrix.begin(), str_node("\\comma"));
	for(unsigned c1=0; c1<values.size(); ++c1) {
		auto row=matrix.append_child(cols, str_node("\\comma"));
		for(unsigned c2=0; c2<values.size(); ++c2) {
			// Generate an expression with this component, apply substitution, then stick
			// the result into the string that will go to sympy.

			Ex c(ex.begin());
			Ex::iterator cit1=c.child(c.begin(), 0);
			Ex::iterator cit2=c.child(c.begin(), 1);
			cit1=c.replace_index(cit1, values[c1].begin(), true);
			cit2=c.replace_index(cit2, values[c2].begin(), true);

			Ex::iterator cit=c.begin();
			substitute subs(kernel, c, rules);
			if(subs.can_apply(cit)) {
				subs.apply(cit);
				matrix.append_child(row, cit);
				}
			else {
				zero( matrix.append_child(row, str_node("1"))->multiplier );
				}
			}
		}

	return matrix;
	}

void sympy::invert_matrix(const Kernel& kernel, Ex& ex, Ex& rules, const Ex& tocompute)
	{
	if(ex.number_of_children(ex.begin())!=2) {
		throw ConsistencyException("Object should have exactly two indices.");
		}

	auto matrix = fill_matrix(kernel, ex, rules);

	auto top=matrix.begin();
	std::vector<std::string> wrap;
	sympy::apply(kernel, matrix, top, wrap, std::vector<std::string>(), ".inv()");
	//matrix.print_recursive_treeform(std::cerr, top);

	Ex::iterator ind1=ex.child(ex.begin(), 0);
	Ex::iterator ind2=ex.child(ex.begin(), 1);
	const Indices *prop1 = kernel.properties.get<Indices>(ind1);
	const Indices *prop2 = kernel.properties.get<Indices>(ind2);

	Ex::iterator ruleslist=rules.begin();
	const auto& values1 = prop1->values(kernel.properties, ind1);
	const auto& values2 = prop2->values(kernel.properties, ind2);	

	// Now we need to iterate over the components again and construct sparse rules.
	auto cols=matrix.begin(matrix.begin()); // outer comma
	auto row=matrix.begin(cols); // first inner comma
	for(unsigned c1=0; c1<values1.size(); ++c1) {
		auto el =matrix.begin(row);  // first element of first inner comma
		for(unsigned c2=0; c2<values2.size(); ++c2) {
			if(el->is_zero()==false) {
				Ex rule("\\equals");
				auto rit  = rule.append_child(rule.begin(), tocompute.begin());
				/* auto cvit = */ rule.append_child(rule.begin(), Ex::iterator(el));
				auto i = rule.begin(rit);
				//std::cerr << c1 << ", " << c2 << std::endl;
				i = rule.replace_index(i, values1[c1].begin(), true);
				//				i->fl.parent_rel=ind1->fl.parent_rel;
				++i;
				i = rule.replace_index(i, values2[c2].begin(), true);
				//				i->fl.parent_rel=ind1->fl.parent_rel;
				rules.append_child(ruleslist, rule.begin());
				//rule.print_recursive_treeform(std::cerr, rule.begin());
				}
			++el;
			}
		++row;
		}
	}

void sympy::determinant(const Kernel& kernel, Ex& ex, Ex& rules, const Ex& tocompute)
	{
	auto matrix = fill_matrix(kernel, ex, rules);

	auto top=matrix.begin();
	std::vector<std::string> wrap;
	sympy::apply(kernel, matrix, top, wrap, std::vector<std::string>(), ".det()");

	Ex rule("\\equals");
	rule.append_child(rule.begin(), tocompute.begin());
	rule.append_child(rule.begin(), matrix.begin());
	rules.append_child(rules.begin(), rule.begin());
	}

void sympy::trace(const Kernel& kernel, Ex& ex, Ex& rules, const Ex& tocompute)
	{
	auto matrix = fill_matrix(kernel, ex, rules);

	auto top=matrix.begin();
	std::vector<std::string> wrap;
	sympy::apply(kernel, matrix, top, wrap, std::vector<std::string>(), ".trace()");

	Ex rule("\\equals");
	rule.append_child(rule.begin(), tocompute.begin());
	rule.append_child(rule.begin(), matrix.begin());
	rules.append_child(rules.begin(), rule.begin());
	}
