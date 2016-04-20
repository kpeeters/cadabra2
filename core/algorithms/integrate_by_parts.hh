

#pragma once

#include "Algorithm.hh"

/// \ingroup algorithms
///
/// Integrate by parts away from the indicated derivative object.


class integrate_by_parts : public Algorithm {
	public:
		integrate_by_parts(const Kernel&, Ex&, Ex&);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	private:
		// Integrate by parts on a single term in the integrand. First
		// argument points to the integral, second to a term in the 
		// integrand.
		result_t handle_term(iterator, iterator&);

		// Are the given integral and derivative inverses of each-other?
		bool int_and_derivative_related(iterator int_it, iterator der_it) const;

		// Wrap the indicated range of factor nodes inside the product node in 
      // the derivative.
		Ex wrap(iterator, sibling_iterator, sibling_iterator) const;

		// Determine whether the indicated derivative acts on the 'away_from'
		// expression.
		bool derivative_acting_on_arg(iterator der_it) const;

		// Expression to move derivative away from.
		Ex away_from;
};

