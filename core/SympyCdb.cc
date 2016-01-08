
#include <boost/python.hpp>
#include "SympyCdb.hh"
#include "PreClean.hh"
#include "Cleanup.hh"
#include "Parser.hh"
#include "Kernel.hh"
#include "DisplaySympy.hh"

Ex::iterator apply_sympy(Kernel& kernel, Ex& ex, Ex::iterator& it, const std::string& head, const std::string& args)
	{
	// We first need to print the sub-expression using DisplaySympy,
	// optionally with the head wrapped around it and the args added
	// (if present).

	std::ostringstream str;

	if(head.size()>0)
		str << head << "(";

	DisplaySympy ds(kernel.properties, ex);
	ds.output(str, it);

	if(head.size()>0)
		if(args.size()>0) 
			str << ", " << args << ")";

	// We then execute the expression in Python.

	std::cerr << "feeding " << str.str() << std::endl;

	auto module = boost::python::import("sympy.parsing.sympy_parser");
	auto parse  = module.attr("parse_expr");
	boost::python::object obj = parse(str.str());
	// std::cerr << "converting result to string" << std::endl;
	auto __str__ = obj.attr("__str__");
	boost::python::object res = __str__();
	std::string result = boost::python::extract<std::string>(res);
	std::cerr << result << std::endl;
	

   // After that, we construct a new sub-expression from this string by using our
   // own parser, and replace the original.

	auto ptr = std::make_shared<Ex>();
	Parser parser(ptr);
	std::stringstream istr(result);
	istr >> parser;

	pre_clean_dispatch_deep(kernel, *parser.tree);
   cleanup_dispatch_deep(kernel, *parser.tree);

	parser.tree->print_recursive_treeform(std::cerr, parser.tree->begin());

	Ex::iterator first=parser.tree->begin(parser.tree->begin());
   it = ex.move_ontop(it, first);

	return it;
	}
