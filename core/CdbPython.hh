
#pragma once

#include <string>

namespace cadabra {

	std::string escape_quotes(const std::string&);

	// Convert a block of Cadabra notation into pure Python. Mimics
	// the functionality in the python script 'cadabra2'

	std::string cdb2python(const std::string&);

	// As above, but for a single line; for private use only.

	std::string convert_line(const std::string&, std::string& lhs, std::string& rhs, std::string& indent);	

}
