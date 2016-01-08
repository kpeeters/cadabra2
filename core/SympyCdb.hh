
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include "Kernel.hh"

/// \ingroup scalar
///
/// Functionality to act with Sympy functions on (parts of) Cadabra Ex expressions
/// and read the result back into the same Ex. This duplicates some of the 
/// logic in PythonCdb.hh, in particular make_Ex_from_string, but it is best to
/// keep these two completely separate.

Ex::iterator apply_sympy(Kernel&, Ex&, Ex::iterator&, const std::string& head, const std::string& args);
