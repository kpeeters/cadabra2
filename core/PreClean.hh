
#pragma once

#include "Algorithm.hh"

/// \ingroup core
///
/// Handle the entire preclean stage, which turns a parsed expression
/// into an expression which satisfies the Cadabra conventions. This
/// does not involve the property system.
///
/// - All numerical multipliers in a product on the product node, no
///   multiplier on a sum node.
///
/// - Any '\\frac' nodes with a purely numerical denominator should be
///   rewritten as a rational multiplier for the numerator node.
///
/// - Any \\sub nodes get converted to \\sum nodes.

namespace cadabra {

	void pre_clean_dispatch(const Kernel& k, Ex&, Ex::iterator& it);
	void pre_clean_dispatch_deep(const Kernel& k, Ex&);

	/// Cleanup for individual node types.  These are not needed during any
	/// later stages of the manipulation of an expression (and are hence
	/// defined here, not in 'Cleanup.hh').

	void cleanup_updown(const Kernel& k, Ex&, Ex::iterator& it);
	void cleanup_rational(const Kernel& k, Ex&, Ex::iterator& it);
	void cleanup_frac(const Kernel& k, Ex&, Ex::iterator& it);
	void cleanup_sqrt(const Kernel& k, Ex&, Ex::iterator& it);
	void cleanup_sub(const Kernel& k, Ex&, Ex::iterator& it);

	/// Convert parser output which indicates an indexbracket to an actual
	/// indexbracket node.
	void cleanup_indexbracket(const Kernel& k, Ex&, Ex::iterator& it);

	/// Replace all occurrances of 'from' with 'to', return result (does not replace in-place).
	std::string replace_all(std::string const& original, std::string const& from, std::string const& to );

	}
