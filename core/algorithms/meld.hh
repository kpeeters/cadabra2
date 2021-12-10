#pragma once

#include <memory>
#include <array>
#include "Algorithm.hh"
#include "Adjform.hh"
#include "properties/TableauBase.hh"

namespace cadabra {

	class meld : public Algorithm
		{
		public:
			meld(const Kernel& kernel, Ex& ex, bool project_as_sum = false);
			virtual ~meld();

			virtual bool can_apply(iterator it) override;
			virtual result_t apply(iterator& it) override;

		private:
			using tab_t = cadabra::TableauBase::tab_t;

			struct ProjectedTerm {
				ProjectedTerm(const Kernel& kernel, IndexMap& index_map, Ex& ex, Ex::iterator it);
				// Return 'true' if the tensor parts are identical up to index structure
				bool compare(const Kernel& kernel, const ProjectedTerm& other);

				Ex scalar, tensor;
				ProjectedAdjform projection;
				Adjform ident;
				Ex::iterator it;
				bool changed;
			};

			struct symmetrizer_t {
				symmetrizer_t(bool antisymmetric, bool independent) : antisymmetric(antisymmetric), independent(independent) {}
				std::vector<size_t> indices;
				bool antisymmetric, independent;
			};

			// Return the tableaux associated with the expression, offsetting
			// cells with how many indices *deep* it is 
			std::vector<tab_t> collect_tableaux(Ex& ex) const;

			// Collects all the columns and rows of the tableaux starting at node 'it' and combines
			// similar columns where possible. Returns 'true' if the symmetrizers mean the term is
			// identically 0
			bool collect_symmetries(const std::vector<tab_t>& tabs, std::vector<symmetrizer_t>& symmetrizers) const;
			bool collect_symmetries_as_product(const std::vector<tab_t>& tabs, std::vector<symmetrizer_t>& symmetrizers) const;
			bool collect_symmetries_as_sum(const std::vector<tab_t>& tabs, std::vector<symmetrizer_t>& symmetrizers) const;

			void symmetrize(ProjectedTerm& adj, const std::vector<symmetrizer_t>& symmetries);
			void symmetrize_as_product(ProjectedTerm& adj, const std::vector<symmetrizer_t>& symmetrizers);
			void symmetrize_as_sum(ProjectedTerm& adj, const std::vector<symmetrizer_t>& symmetries);
			void symmetrize_idents(ProjectedTerm& sym);

			// Remove non-diagonal terms of diagonal objects
			bool can_apply_diagonals(iterator it);
			bool apply_diagonals(iterator it);

			// Remove terms which contain contractions of traceless objects
			bool can_apply_traceless(iterator it);
			bool apply_traceless(iterator it);

			// Apply the symmetry Tr(ABC) = Tr(BCA) = Tr(CAB)
			bool can_apply_cycle_traces(iterator it);
			bool apply_cycle_traces(iterator it);

			// Compare Young projections of each term for symmetry
			bool can_apply_tableaux(iterator it);
			bool apply_tableaux(iterator it);

			//bool can_apply_side_relations(iterator it);
			//bool apply_side_relations(iterator it);
			//Ex side_relations;

			IndexMap index_map;
			bool project_as_sum;
		};

}
