/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2014  Kasper Peeters <kasper.peeters@phi-sci.com>

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/


#pragma once

#include "Storage.hh"
#include "Kernel.hh"

// Central cleanup dispatch routine, which calls the other cleanup
// functions defined later. These algorithms clean up the tree at the
// current node and the first layer of child nodes, but do not descend
// deeper down the tree.

// IMPORTANT: as with any algorithms, the iterator pointing to the
// starting node may be changed, but these functions are not allowed
// to modify anything except the node and nodes below.

void cleanup_dispatch(Kernel& k, exptree&, exptree::iterator& it);

void cleanup_productlike(Kernel& k, exptree&, exptree::iterator& it);
void cleanup_sumlike(Kernel& k, exptree&, exptree::iterator& it);
void cleanup_expressionlike(Kernel& k, exptree&, exptree::iterator& it);
void cleanup_derivative(Kernel& k, exptree&, exptree::iterator& it);


// Walk depth-first along the entire tree and call cleanup_dispatch at
// every node.

void cleanup_dispatch_deep(Kernel& k, exptree&);


// DEPRECATED:
// Generic helper functions to cleanup expressions and bring them
// to canonical form. These tend to make use of algorithms from the 
// sub-modules and should hence not really be part of the core.
// This should be fixed later.

void cleanup_expression(exptree&);
void cleanup_expression(exptree&, exptree::iterator&); // may change the pointer!
void cleanup_sums_products(exptree&, exptree::iterator&);
void cleanup_nests(exptree&tr, exptree::iterator &it, bool ignore_bracket_types=false);
void cleanup_nests_below(exptree&tr, exptree::iterator it, bool ignore_bracket_types=false);

