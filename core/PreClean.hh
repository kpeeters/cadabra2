
#pragma once

// Routines to cleanup input expression when it is passed to the system
// for the very first time. This includes doing things like moving 
// numerical factors into multiplier fields.
//
// Defines a number of algorithms which are not needed during any later
// stages of the manipulation of an expression.


#include "Algorithm.hh"

// Handle the entire preclean stage.

void pre_clean_dispatch(Kernel& k, exptree&, exptree::iterator& it);
void pre_clean_dispatch_deep(Kernel& k, exptree&);

// Cleanup for individual node types.

void cleanup_rational(Kernel& k, exptree&, exptree::iterator& it);
void cleanup_frac(Kernel& k, exptree&, exptree::iterator& it);
void cleanup_sub(Kernel& k, exptree&, exptree::iterator& it);
