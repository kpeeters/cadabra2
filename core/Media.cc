
#include "Media.hh"

LaTeXString::LaTeXString(const std::string& s)
   : content(s)
	{
	}

std::string LaTeXString::latex() const
	{
	return content;
	}
