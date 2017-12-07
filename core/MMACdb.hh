
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include "Kernel.hh"
#include "Stopwatch.hh"

#include "wstp.h"

class MMA {
	public:

		/// \ingroup scalar
		///
		/// Functionality to act with Mathematica functions on (parts of)
		/// Cadabra Ex expressions and read the result back into the same
		/// Ex. This duplicates some of the logic in PythonCdb.hh, in
		/// particular make_Ex_from_string, but it is best to keep these
		/// two completely separate.
		
		static cadabra::Ex::iterator apply_mma(const cadabra::Kernel&, cadabra::Ex&, cadabra::Ex::iterator&,
															const std::vector<std::string>& wrap, const std::string& args,
															const std::string& method);


	private:
		static WSEnvironment stdenv;
		static WSLINK        lp;

		static void setup_link();
		static void teardown_link();
};
