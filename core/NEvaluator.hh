
#pragma once

#include "Storage.hh"
#include "NTensor.hh"
#include "Compare.hh"
#include <functional>

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
	/// If there are 'n' variables, then at any stage the tree will
	/// be an n-tensor, with the dimension of each index set by the
	/// length of the value array for the corresponding variable.

	class NEvaluator {
		public:
			NEvaluator();
			NEvaluator(Ex::iterator);

			/// If we know the value of a subtree explicitly as a number,
			/// it is stored in this map. These are computed nodes. Note that
			/// keys are compared as pointers, so two identical expressions at
			/// two different locations in the tree will have two entries in
			/// this map.
			std::map<Ex::iterator, NTensor, Ex::iterator_base_less> subtree_values;

			/// If we know the value of a subtree is equal to another subtree
			/// (either in the same expression or another one), it is stored
			/// in the map below. This then still needs a lookup in the
			/// `subtree_values` map.
			/// FIXME: not used right now. The idea was that if we find two subtrees
			/// which are equal symbolically, we only have to evaluate one, and can
			/// then read off the value of the other one by referring to the first.
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

			/// Set function to evaluate. Can also be passed in the constructor.
			void    set_function(Ex::iterator);
			
			/// Set the range of values which we want to insert into the
			/// indicated variable. Fills the map above.
			void    set_variable(const Ex&, const NTensor& val);

			/// Set an external function which will be used by `evaluate` to lookup
			/// values of objects as a last resort, if they do not resolve using
			/// the `variable_values` list above.
			typedef std::function<std::complex<double>(const Ex&)> lookup_function_t;
			void    set_lookup_function(lookup_function_t);
			
			/// Evaluate the expression, using the variable values set in
			/// `set_variable`.
			NTensor evaluate();

			/// PRIVATE:

			void find_variable_locations();

		private:
			Ex                ex;
			lookup_function_t lookup_function;

			// The shape of the tensor that we will produce.
			std::vector<size_t> fullshape;
	};

};
