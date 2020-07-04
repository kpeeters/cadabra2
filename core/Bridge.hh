
#pragma once

#include "Storage.hh"
#include "Kernel.hh"

/// \ingroup pythoncore
///
/// Replace any objects of the form '@(...)' in the expression tree by the
/// python expression '...' if it exists. Rename dummies to avoid clashes.

void pull_in(std::shared_ptr<cadabra::Ex> ex, cadabra::Kernel *);


/// \ingroup pythoncore
///
/// Run all functions in the given expression which are available 
/// in the Python scope.

void run_python_functions(std::shared_ptr<cadabra::Ex> ex, cadabra::Kernel *);
