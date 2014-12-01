
#pragma once

#include "properties/TableauBase.hh"

class Symmetric : public TableauBase, virtual public property  {
	public:
		virtual ~Symmetric() {};
		virtual bool parse(exptree&, exptree::iterator, exptree::iterator, keyval_t&);
		virtual std::string name() const;

		virtual unsigned int size(const Properties&, exptree&, exptree::iterator) const override;
		virtual tab_t        get_tab(const Properties&, exptree&, exptree::iterator, unsigned int) const override;
};

