
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include "Kernel.hh"
#include "Stopwatch.hh"

//namespace mma {

	/// \ingroup scalar
	///
	/// Functionality to act with Mathematica on all scalar parts of an
	/// expression, and keep the result in-place. This is a
	/// higher-level function than 'apply' below.

//	cadabra::Ex* map_mma(const cadabra::Kernel&, cadabra::Ex&,
//								const std::vector<std::string>& wrap, const std::string& args, const std::string& method);
	
   /// \ingroup scalar
   ///
   /// Functionality to act with Mathematica functions on (parts of)
   /// Cadabra Ex expressions and read the result back into the same
   /// Ex. This duplicates some of the logic in PythonCdb.hh, in
   /// particular make_Ex_from_string, but it is best to keep these
   /// two completely separate.

	cadabra::Ex::iterator apply_mma(const cadabra::Kernel&, cadabra::Ex&, cadabra::Ex::iterator&,
										 const std::vector<std::string>& wrap, const std::string& args, const std::string& method);

	
//};
