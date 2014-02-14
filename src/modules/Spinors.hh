
#pragma once

class spinorsort : public algorithm {
	public:
		spinorsort(exptree&, iterator);
		
		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);

		sibling_iterator one, gammamat, two;
};

