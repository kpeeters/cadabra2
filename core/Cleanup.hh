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

typedef void (*dispatcher_t)(const Kernel& k, Ex&, Ex::iterator& it);

/// \ingroup cleanup
///
/// Central cleanup dispatch routine, which calls the other cleanup
/// functions defined later. These algorithms clean up the tree at the
/// current node and the first layer of child nodes, but do NOT descend
/// deeper down the tree, UNLESS that would leave the tree in an 
/// inconsistent state. An example is acting at the top node of
/// \prod{4}{\sum{a}{b}}, which would push the 4 to the multiplier of the
/// sum, but that is not allowed, so it needs to go further down.
/// Sibling nodes of 'it' remain untouched as well.

void cleanup_dispatch(const Kernel& k, Ex&, Ex::iterator& it);

/// \ingroup cleanup
///
/// More general cleanup of an entire tree. Walks depth-first along the
/// entire tree and call cleanup_dispatch at every node.

void cleanup_dispatch_deep(const Kernel& k, Ex&, dispatcher_t disp=&cleanup_dispatch);

/// Individual node cleanup routines. Do not call these yourself. 
///
/// Once more, these algorithms clean up the tree at the current node
/// and the first layer of child nodes, but do NOT descend deeper down
/// the tree, except when that is necessary to ensure that the tree
/// remains consistent.  As with any algorithms, the iterator pointing
/// to the starting node may be changed, but these functions are not
/// allowed to modify anything except the node and nodes below (in
/// particular, they will leave sibling nodes untouched).

void cleanup_productlike(const Kernel& k, Ex&, Ex::iterator& it);
void cleanup_sumlike(const Kernel& k, Ex&, Ex::iterator& it);
void cleanup_expressionlike(const Kernel& k, Ex&, Ex::iterator& it);
void cleanup_derivative(const Kernel& k, Ex&, Ex::iterator& it);
void cleanup_components(const Kernel& k, Ex&, Ex::iterator& it);
void cleanup_numericalflat(const Kernel& k, Ex&, Ex::iterator& it);

/// Given a node with a non-unit multiplier, push this multiplier
/// down the tree if the node is not allowed to have a non-unit
/// multiplier. This is a recursive procedure as the node onto
/// which the multiplier gets pushed may itself not allow for
/// a non-unit multiplier.
/// Note that some nodes disallow non-unit multipliers on their
/// children, but that should be handled individually (see cleanup
/// of product nodes for an example).

void push_down_multiplier(const Kernel& k, Ex& tr, Ex::iterator it);




