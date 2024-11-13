
#pragma once

#include "Algorithm.hh"
#include "algorithms/sort_product.hh"
#include "lru_cache.hh"

namespace cadabra {

	/// \ingroup algorithms
	///
	/// Generic substitution algorithm.

	class substitute : public Algorithm {
		public:
			substitute(const Kernel&, Ex& tr, Ex& args, bool partial=true);

			/// Match the lhs of the replacement rule to the subtree 'st' under consideration.
			/// This will fill the replacement_map giving a map from objects appearing in the
			/// _rule_ to what they matched to.
			///
			/// For F_{m n} -> G_{m n}, position-free indices and the
			/// expression F^{m n}, we would get a replacement map
			///
			///      _m  -> ^m
			///      _n  -> ^n
			///
			/// (Note how it is important that despite these indices being position-free,
			/// the rules in the replacement map do not ignore sub/superscript information).
			/// In the apply member, we then replace all of 'st' with the rhs of the rule,
			/// and scan through the rule (with a basic replacement_map.find based on
			/// simple tree comparison logic, not pattern matching) for replacements to be made.

			virtual bool     can_apply(iterator st);

			virtual result_t apply(iterator&);

			Ex_comparator comparator;

		private:
			Ex&     args;

			iterator      use_rule;
			iterator      conditions;

			std::map<iterator, bool> lhs_contains_dummies, rhs_contains_dummies;

			// For object swap testing routines:
			sort_product    sort_product_;
			bool            partial;
			
			// Rules is a class for caching properties of substitution
			// rules to avoid processing them in subsequent calls.

			class Rules {

				public:
					Rules(size_t max_size_=1000, size_t cleanup_threshold_=100) 
						: properties(max_size_), max_size(max_size_), cleanup_threshold(cleanup_threshold_) {}

					// Associate rule properties with a specific object
					void store(Ex& rules,
								  std::map<iterator, bool>& lhs_contains_dummies,
								  std::map<iterator, bool>& rhs_contains_dummies);
					
					// Check if rules are present
					bool is_present(Ex& rules) const;
					
					// Retrieve properties from rules
					void retrieve(Ex& rules,
									  std::map<iterator, bool>& lhs_contains_dummies,
									  std::map<iterator, bool>& rhs_contains_dummies) const;
					
					// Count number of rules
					int size() const;
					
					// Eliminate rules that are expired
					void cleanup();

				private:
					// Map storing weak pointers to `Ex` and pairs of lhs/rhs maps as values
					mutable LRUcache<std::weak_ptr<Ex>, 
										std::pair< std::map<iterator, bool>, std::map<iterator, bool> >,
										std::owner_less<std::weak_ptr<Ex>>
										> properties;
					// Max size of the rules list
					size_t max_size;
					// Initial size threshold to trigger cleanup_rules
					size_t cleanup_threshold;


				};

			// Shared instance of all replacement rules.
			static Rules    replacement_rules;
			static size_t   cache_hits, cache_misses;
			
	};


	}


