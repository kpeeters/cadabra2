
#pragma once

#include "Algorithm.hh"
#include "properties/WeightInherit.hh"
#include "properties/Weight.hh"
#include <string>

class drop_keep_weight : public Algorithm {
	public:
		drop_keep_weight(const Kernel&, Ex&, Ex&);

		virtual bool can_apply(iterator) override;
		result_t     do_apply(iterator&, bool keepthem);

	protected:
		Ex                   arg;
		const WeightInherit *gmn;
		const Weight        *wgh;
		std::string          label;
		multiplier_t         weight;
};

class drop_weight : public drop_keep_weight {
	public:
		drop_weight(const Kernel&, Ex&, Ex&);

		virtual result_t apply(iterator&) override;		
};

class keep_weight : public drop_keep_weight {
	public:
		keep_weight(const Kernel&, Ex&, Ex&);

		virtual result_t apply(iterator&) override;		
};

