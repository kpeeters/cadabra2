
#pragma once

// Routines to cleanup input expression when it is passed to the system
// for the very first time. This includes doing things like moving 
// numerical factors into multiplier fields.
//
// Defines a number of algorithms which are not needed during any later
// stages of the manipulation of an expression.

#include "Algorithm.hh"

// Handle the entire preclean stage.

void pre_clean(Kernel&, exptree&, exptree::iterator);

class ratrewrite : public Algorithm {
	public:
		ratrewrite(Kernel&, exptree&);

		virtual bool     can_apply(iterator);
		virtual result_t apply(iterator&);		
};


