
#pragma once

#include "Props.hh"
#include "YoungTab.hh"

namespace cadabra {

	class TableauBase : virtual public property {
		public:
			virtual ~TableauBase() {};
			typedef yngtab::filled_tableau<unsigned int> tab_t;

			virtual std::string name() const;

			virtual unsigned int size(const Properties&, Ex&, Ex::iterator) const=0;
			virtual tab_t        get_tab(const Properties&, Ex&, Ex::iterator, unsigned int) const=0;

			virtual bool         only_column_exchange() const
				{
				return false;
				};

			// Indexgroups are groups of indices which can be sorted by application
			// of single-index monoterm symmetries. E.g. R_{m n p q} -> {m,n}:0, {p,q}:1.
			int                  get_indexgroup(const Properties&, Ex&, Ex::iterator, int) const;

			// Is the tableau either a single column or a single row, and without
			// duality projections?
			bool                 is_simple_symmetry(const Properties&, Ex&, Ex::iterator) const;
		};

	}
