
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	/// \ingroup algorithms
	///
	/// Complete a set of coordinate rules so that they also cover related tensors.
	/// At present this only inverts metric rules, but could do more related rules,
	/// or expanded to cover symmetry.

	class complete : public Algorithm {
		public:
			complete(const Kernel&, Ex&, Ex&);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		private:
			Ex goal;
		};

	}
