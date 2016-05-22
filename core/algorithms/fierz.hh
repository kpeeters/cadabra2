
#pragma once

#include "Algorithm.hh"
#include "properties/Indices.hh"
#include "properties/Spinor.hh"

class fierz : public Algorithm {
	public:
		fierz(const Kernel&, Ex&, Ex&);
		
		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	private:
		Ex              spinor_list;

		iterator        spin1, spin2, spin3, spin4;
		const Spinor   *prop1,*prop2,*prop3,*prop4;
		iterator        gam1, gam2;
		int             dim, spinordim;
		const Indices  *indprop;
};
