
#include <boost/python.hpp>
#include "Functional.hh"
#include "SympyCdb.hh"
#include "PreClean.hh"
#include "Cleanup.hh"
#include "Parser.hh"
#include "Kernel.hh"
#include "DisplaySympy.hh"

Ex::iterator sympy::apply(Kernel& kernel, Ex& ex, Ex::iterator& it, const std::string& head, const std::string& args)
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

	//ex.print_recursive_treeform(std::cerr, it);
	std::cerr << "feeding " << str.str() << std::endl;

	auto module = boost::python::import("sympy.parsing.sympy_parser");
	auto parse  = module.attr("parse_expr");
	boost::python::object obj = parse(str.str());
	// std::cerr << "converting result to string" << std::endl;
	auto __str__ = obj.attr("__str__");
	boost::python::object res = __str__();
	std::string result = boost::python::extract<std::string>(res);
	//std::cerr << result << std::endl;
	

   // After that, we construct a new sub-expression from this string by using our
   // own parser, and replace the original.

	auto ptr = std::make_shared<Ex>();
	Parser parser(ptr);
	std::stringstream istr(result);
	istr >> parser;

	pre_clean_dispatch_deep(kernel, *parser.tree);
   cleanup_dispatch_deep(kernel, *parser.tree);

	//parser.tree->print_recursive_treeform(std::cerr, parser.tree->begin());

	ds.import(*parser.tree);

	Ex::iterator first=parser.tree->begin(parser.tree->begin());
   it = ex.move_ontop(it, first);

	return it;
	}

Ex sympy::invert_matrix(Kernel& kernel, Ex& ex, Ex& rules)
	{
	// check that object has two children only.
	if(ex.number_of_children(ex.begin())!=2) {
		throw ConsistencyException("Object should have exactly two indices.");
		}

	Ex::iterator ind1=ex.child(ex.begin(), 0);
	Ex::iterator ind2=ex.child(ex.begin(), 1);

	Ex ret;

	// Get Indices property and from there Coordinates.

	const Indices *prop1 = kernel.properties.get<Indices>(ind1);
	const Indices *prop2 = kernel.properties.get<Indices>(ind2);
	
	if(prop1!=prop2 || prop1==0) 
		throw ConsistencyException("Need the indices of object to be declared with Indices property.");

	// Run over all values of Coordinates, construct matrix.
	std::cerr << "number of coordinates: " << prop1->values.size()  << std::endl;

	std::ostringstream str;
	str << "Matrix(";
	for(int c1=0; c1<prop1->values.size(); ++c1) {
		for(int c2=0; c2<prop1->values.size(); ++c2) {
			
			}
		}
	str << ")";

	return ret;
	}
