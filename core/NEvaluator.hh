
#pragma once

#include "Storage.hh"
#include "NTensor.hh"

namespace cadabra {

	/// Functionality to numerically evaluate a scalar expression,
	/// give the values of its building blocks.
	///
	/// The input is a map which relates subtrees in the expression
	/// to numerical values. The `evaluate` function will then do a
	/// post-order iteration over the tree, collecting and combining
	/// the values thus obtained.
	///
	/// Currently needs the leaf nodes to have an entry in the
	/// `subtree_values` map; may be extended later to cover values
	/// of non-elementary subtrees.

	class NEvaluator {
		public:
			/// If we know the value of a subtree explicitly as a number,
			/// it is stored in this map. These are computed nodes.
			std::map<Ex::iterator, NTensor, Ex::iterator_base_less> subtree_values;

			/// If we know the value of a subtree is equal to another subtree
			/// (either in the same expression or another one), it is stored
			/// in the map below. This then still needs a lookup in the
			/// `subtree_values` map.
			std::map<Ex::iterator, Ex::iterator, Ex::iterator_base_less> subtree_equalities;

			/// The expression will get evaluated for a range of values for
			/// each unknown sub-expression (variable). These are set in
			/// the map below.
			std::map<Ex, NTensor> expression_values;

			/// Set the range of values which we want to insert into the
			/// indicated variable.
			void    set_variable(const Ex&, const NTensor& val);

			/// Evaluate the expression, using the variable values set in
			/// `set_variable`.
			NTensor evaluate(const Ex&);

	};

};
