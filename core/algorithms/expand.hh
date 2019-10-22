
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class expand : public Algorithm {
		public:
			expand(const Kernel&, Ex&);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;

		private:
			iterator       mx_first, mx_last, ii_first, ii_last;
			bool           one_index, check_pos;
			index_iterator nth_implicit_index(Ex::iterator eform, Ex::iterator iform, unsigned int n);
		};

	}
