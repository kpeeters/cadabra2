
#pragma once

#include "algorithms/young_project.hh"
#include "properties/TableauBase.hh"
#include "properties/Indices.hh"

namespace cadabra {

/// \ingroup algorithms
///
/// Decompose a product of tensors by applying Young projectors.

class decompose_product : public Algorithm {
	public:
		decompose_product(const Kernel&, Ex& tr);

		virtual bool     can_apply(iterator) override;
		virtual result_t apply(iterator&) override;

	private:
		typedef young_project::name_tab_t  sibtab_t;
		typedef yngtab::tableaux<sibtab_t> sibtabs_t;
		typedef young_project::pos_tab_t   numtab_t;
		typedef yngtab::tableaux<numtab_t> numtabs_t;

		/// Test that all indices on the product are equivalent, that is, have
		/// the same Indices property attached to them. Return this property.
		const Indices *indices_equivalent(iterator it) const;

		void fill_asym_ranges(TableauBase::tab_t& tab, int offset, combin::range_vector_t&);
		void project_onto_initial_symmetries(Ex& rep, iterator rr, young_project& yp,
														 const TableauBase *tt, iterator ff, 
														 int offset, const TableauBase::tab_t& thetab,
														 bool remove_traces);

		iterator               f1, f2;
		const TableauBase     *t1, *t2;
		TableauBase::tab_t     t1tab, t2tab;
		const Indices         *ind1, *ind2;
		unsigned int dim;
		yngtab::filled_tableau<iterator> nt1, nt2;

		combin::range_vector_t asym_ranges;
};

}
