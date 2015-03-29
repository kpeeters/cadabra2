
#pragma once

#include "Algorithm.hh"

// Handle the entire preclean stage, which turns a parsed expression
// into an expression which satisfies the conventions described in the
// 'doc/developers/conventions.tex' document.

void pre_clean_dispatch(Kernel& k, exptree&, exptree::iterator& it);
void pre_clean_dispatch_deep(Kernel& k, exptree&);

// Cleanup for individual node types.  These are not needed during any
// later stages of the manipulation of an expression (and are hence
// defined here, not in 'Cleanup.hh').

void cleanup_rational(Kernel& k, exptree&, exptree::iterator& it);
void cleanup_frac(Kernel& k, exptree&, exptree::iterator& it);
void cleanup_sub(Kernel& k, exptree&, exptree::iterator& it);
