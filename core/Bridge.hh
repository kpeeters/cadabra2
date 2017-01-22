
#pragma once

#include "Storage.hh"

/// \ingroup pythoncore
///
/// Replace any objects of the form '@(...)' in the expression tree by the
/// python expression '...' if it exists. Rename dummies to avoid clashes.

void pull_in(std::shared_ptr<cadabra::Ex> ex);
