
#pragma once

#include "properties/TableauBase.hh"

class Symmetric : public TableauBase, virtual public property  {
	public:
		virtual ~Symmetric() {};
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;

		virtual unsigned int size(exptree&, exptree::iterator) const;
		virtual tab_t        get_tab(exptree&, exptree::iterator, unsigned int) const;
};

