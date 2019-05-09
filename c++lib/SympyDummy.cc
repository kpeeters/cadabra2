
#include "Parser.hh"
#include "Cleanup.hh"
#include "PreClean.hh"
#include "SympyCdb.hh"
#include "DisplaySympy.hh"
#ifdef USE_TREETRACKER
#include "treetracker.h"
#endif
#include <sstream>

cadabra::Ex::iterator sympy::apply(const cadabra::Kernel& kernel, cadabra::Ex& ex, cadabra::Ex::iterator& it,
                                   const std::vector<std::string>& head, std::vector<std::string> args,
                                   const std::string& method)
	{
	std::ostringstream str;

	if(head.size()>0)
		str << head[0] << "(";

	cadabra::DisplaySympy ds(kernel, ex);
	ds.output(str, it);

	if(head.size()>0)
		if(args.size()>0)
			str << ", " << args[0] << ")";
	str << method;

	if(head.size()>0)
		str << ")";

	//	std::cerr << "Send: " << str.str() << std::endl;

#ifdef USE_TREETRACKER

	auto res = TreeTracker::FromString(str.str());
	res.RecursiveSimplify();
	std::stringstream istr;
	res.ShowTree(istr, 0, false, true);
	//	std::cerr << "Return: " << istr.str() << std::endl;
	auto ptr = std::make_shared<cadabra::Ex>();
	cadabra::Parser parser(ptr);
	istr >> parser;
	pre_clean_dispatch_deep(kernel, *parser.tree);
	cleanup_dispatch_deep(kernel, *parser.tree);
	//parser.tree->print_recursive_treeform(std::cerr, parser.tree->begin());
	ds.import(*parser.tree);

	cadabra::Ex::iterator first=parser.tree->begin();
	it = ex.move_ontop(it, first);
#endif

	return it;
	}

void sympy::invert_matrix(const cadabra::Kernel& kernel, cadabra::Ex& ex,
								  cadabra::Ex& rules, const cadabra::Ex& tocompute)
	{
	throw std::logic_error("Not implemented: sympy::invert_matrix");
	}

void sympy::determinant(const cadabra::Kernel&, cadabra::Ex& ex, cadabra::Ex& rules, const cadabra::Ex& tocompute)
	{
	throw std::logic_error("Not implemented: sympy::determinant");
	}

void sympy::trace(const cadabra::Kernel&, cadabra::Ex& ex, cadabra::Ex& rules, const cadabra::Ex& tocompute)
	{
	throw std::logic_error("Not implemented: sympy::trace");
	}
