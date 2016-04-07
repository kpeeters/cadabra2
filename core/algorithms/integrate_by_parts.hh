

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
		// Integrate by parts on a single term in the integrand.
		result_t handle_term(iterator&);
};

