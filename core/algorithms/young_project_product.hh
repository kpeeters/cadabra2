
#pragma once

#include "Algorithm.hh"

class young_project_product : public Algorithm {
	public:
		young_project_product(Kernel&, exptree&);
		
		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;
};

