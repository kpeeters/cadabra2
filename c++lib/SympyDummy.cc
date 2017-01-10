#include "SympyCdb.hh"

Ex::iterator sympy::apply(const Kernel& kernel, Ex& ex, Ex::iterator& it,
								  const std::string& head, const std::string& args, 
								  const std::string& method)
	{
	return it;
	}

Ex sympy::invert_matrix(const Kernel& kernel, Ex& ex, Ex& rules)
	{
	throw std::logic_error("Not implemented: sympy::invert_matrix");
	}
