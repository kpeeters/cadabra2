
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
	/// Object to store pre-parsing intermediate results. Necessary
	/// to keep things tidy but also in order to avoid the fact that
	/// we cannot pass strings by reference between C++ and Python.
	class ConvertData {
		public:
			ConvertData();
			ConvertData(const std::string&, const std::string&, const std::string&, const std::string&);
			
			std::string lhs, rhs, op, indent;
	};
	
	/// \ingroup files
   /// Detect Cadabra expression statements and rewrite to Python form.
   ///  
   /// Lines containing ':=' are interpreted as expression declarations.
   /// Lines containing '::' are interpreted as property declarations.
   /// 
   /// These need to end on '.', ':' or ';'. If not, keep track of the
   /// input so far and store that in self.convert_data.lhs, self.convert_data.op, self.convert_data.rhs, and
   /// then return an empty string.
   /// 
   /// TODO: make ';' at the end of '::' line result the print statement printing 
   /// property objects using their readable form; addresses one issue report).

	std::string convert_line(const std::string&, ConvertData& cv, bool display); //std::string& lhs, std::string& rhs, std::string& op, std::string& indent, bool display);

	/// \ingroup files
	/// Convert a Cadabra notebook file to pure Python. This gets
	/// called on-the-fly when importing Cadabra notebooks written by
	/// users, and at install time for all system-supplied packages.
	/// If for_standalone is false, this will not make ';' characters 
	/// generate 'display' statements (used in the conversion of
	/// notebooks to python packages), and it will not convert
	/// any cells which have their `ignore_on_import` flag set.

	#ifndef CDBPYTHON_NO_NOTEBOOK

	std::string cnb2python(const std::string&, bool for_standalone);

	#endif
	}
