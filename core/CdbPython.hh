
#pragma once

#include <string>

namespace cadabra {

	std::string escape_quotes(const std::string&);

	/// \ingroup files
	/// Convert a block of Cadabra notation into pure Python. Mimics
	/// the functionality in the python script 'cadabra2'
	/// If display is false, this will not make ';' characters 
	/// generate 'display' statements (used in the conversion of
	/// notebooks to python packages).

	std::string cdb2python(const std::string&, bool display);

	std::string cdb2python_string(const std::string&, bool display);	

	/// \ingroup files
	/// As above, but for a single line; for private use only.
	/// If display is false, this will not make ';' characters 
	/// generate 'display' statements (used in the conversion of
	/// notebooks to python packages).

	std::string convert_line(const std::string&, std::string& lhs, std::string& rhs, std::string& op, std::string& indent, bool display);

	/// \ingroup files
	/// Convert a Cadabra notebook file to pure Python. This gets
	/// called on-the-fly when importing Cadabra notebooks written by
	/// users, and at install time for all system-supplied packages.
	/// If for_standalone is false, this will not make ';' characters 
	/// generate 'display' statements (used in the conversion of
	/// notebooks to python packages), and it will not convert
	/// any cells which have their `ignore_on_import` flag set.

	std::string cnb2python(const std::string&, bool for_standalone);
	}
