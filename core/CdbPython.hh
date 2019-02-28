
#pragma once

#include <string>

namespace cadabra {

	std::string escape_quotes(const std::string&);

	/// \ingroup files
	/// Convert a block of Cadabra notation into pure Python. Mimics
	/// the functionality in the python script 'cadabra2'

	std::string cdb2python(const std::string&);

	/// \ingroup files
	/// As above, but for a single line; for private use only.

	std::string convert_line(const std::string&, std::string& lhs, std::string& rhs, std::string& op, std::string& indent);	

	/// \ingroup files
	/// Convert a Cadabra notebook file to pure Python. This gets
	/// called on-the-fly when importing Cadabra notebooks written by
	/// users, and at install time for all system-supplied packages.

	std::string cnb2python(const std::string&);
}
