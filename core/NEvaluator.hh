
#pragma once

#include "Storage.hh"
#include "NTensor.hh"
#include "Compare.hh"

namespace cadabra {

	/// \ingroup numerical
	///
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
	///
	/// If the input variables are 'a' NTensors, then at any stage...

	class NEvaluator {
		public:
			NEvaluator(const Ex&);

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
			class VariableValues {
				public:
					Ex                        variable;
					NTensor                   values;
					std::vector<Ex::iterator> locations;
			};
			std::vector<VariableValues> variable_values;
			std::map<Ex, size_t, tree_exact_less_no_wildcards_obj> variable_values_locs;

			/// Set the range of values which we want to insert into the
			/// indicated variable. Fills the map above.
			void    set_variable(const Ex&, const NTensor& val);

			/// Evaluate the expression, using the variable values set in
			/// `set_variable`.
			NTensor evaluate();

			/// PRIVATE:

			void find_variable_locations();

		private:
			Ex ex;
	};

};
