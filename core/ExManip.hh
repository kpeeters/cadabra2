#pragma once

#include "Storage.hh"

namespace cadabra {

	class Kernel;

	class ExManip {
		public:
			typedef Ex::iterator_base       iterator_base;
			typedef Ex::iterator            iterator;
			typedef Ex::post_order_iterator post_order_iterator;
			typedef Ex::sibling_iterator    sibling_iterator;

			ExManip(const Kernel&, Ex&);
			
			/// Take a single non-product node in a sum and wrap it in a
			/// product node, so it can be handled on the same footing as a proper product.
			bool prod_wrap_single_term(iterator&);
			bool prod_unwrap_single_term(iterator&);
			bool sum_wrap_single_term(iterator&);
			bool sum_unwrap_single_term(iterator&);

			/// Is the indicated node a single term in an expression?
         /// Returns true if the indicated node is a single
         /// non-reserved node (non-prod, non-sum, ...)  at the top
         /// level of an expression (real top, top of equation
         /// lhs/rhs, top of integral argument, ...).
			bool  is_single_term(iterator);

			/// 
			bool  is_nonprod_factor_in_prod(iterator);

			/// Wrap a term in a product or sum in a node with indicated
			/// name, irrespective of its parent (it usually makes more
			/// sense to call the safer prod_wrap_single_term or
			/// sum_wrap_single_term above). Sets the iterator to the
			/// new node.
			void  force_node_wrap(iterator&, std::string);

		protected:
			const Kernel& kernel;
			Ex&           tr;
	};
	
};
