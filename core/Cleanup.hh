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

typedef void (*dispatcher_t)(Kernel& k, exptree&, exptree::iterator& it);

// Central cleanup dispatch routine, which calls the other cleanup
// functions defined later. These algorithms clean up the tree at the
// current node and the first layer of child nodes, but do NOT descend
// deeper down the tree. Sibling nodes of 'it' remain untouched as well.

void cleanup_dispatch(Kernel& k, exptree&, exptree::iterator& it);

// More general cleanup of an entire tree. Walks depth-first along the
// entire tree and call cleanup_dispatch at every node.

void cleanup_dispatch_deep(Kernel& k, exptree&, dispatcher_t disp=&cleanup_dispatch);

// Individual node cleanup routines. Once more, these algorithms clean
// up the tree at the current node and the first layer of child nodes,
// but do NOT descend deeper down the tree.
// As with any algorithms, the iterator pointing to the starting node
// may be changed, but these functions are not allowed to modify
// anything except the node and nodes below (in particular, they will
// leave sibling nodes untouched).

void cleanup_productlike(Kernel& k, exptree&, exptree::iterator& it);
void cleanup_sumlike(Kernel& k, exptree&, exptree::iterator& it);
void cleanup_expressionlike(Kernel& k, exptree&, exptree::iterator& it);
void cleanup_derivative(Kernel& k, exptree&, exptree::iterator& it);






