
#pragma once

#include "Props.hh"
#include "YoungTab.hh"

class TableauBase {
	public:
		virtual ~TableauBase() {};
		typedef yngtab::filled_tableau<unsigned int> tab_t;

		virtual unsigned int size(exptree&, exptree::iterator) const=0;
		virtual tab_t        get_tab(const Properties&, exptree&, exptree::iterator, unsigned int) const=0;

		virtual bool         only_column_exchange() const { return false; };

		// Indexgroups are groups of indices which can be sorted by application
		// of single-index monoterm symmetries. E.g. R_{m n p q} -> {m,n}:0, {p,q}:1.
		int                  get_indexgroup(const Properties&, exptree&, exptree::iterator, int) const;

		// Is the tableau either a single column or a single row, and without 
		// duality projections?
		bool                 is_simple_symmetry(const Properties&, exptree&, exptree::iterator) const;
};

