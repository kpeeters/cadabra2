
#pragma once

/// \ingroup display
///
/// Base class for all display classes.  A key difficulty with
/// printing is to figure out when to print additional brackets for
/// objects which would otherwise not render correctly. For example, a
/// sum inside a product need brackets, but it does not need brackets
/// when it is given as an argument to a function, because then the
/// brackets are already there from the function call.

class DisplayBase {
	public:


};
