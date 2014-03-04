
#pragma once

#include "Algorithm.hh"

class rename_dummies : public Algorithm {
	public:
		rename_dummies(Kernel&, exptree&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);
};
